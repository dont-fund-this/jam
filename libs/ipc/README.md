# IPC Plugin - Inter-Process Communication

## Overview

The IPC plugin implements request/reply and pub/sub patterns over WebSocket using the universal `address, payload, options` signature. This enables async communication between plugins, processes, and browser clients without changing the synchronous `Invoke()` contract.

## Architecture

```
┌─────────────────┐
│  Browser JS     │ ← Promise.race with timeout
│  (async/await)  │
└────────┬────────┘
         │ WebSocket {address, payload, options}
         ↓
┌─────────────────┐
│  IPC Plugin     │ ← Request/Reply routing
│  (C++ sync)     │ ← Pub/Sub management
└────────┬────────┘
         │ Invoke(address, payload, options)
         ↓
┌─────────────────┐
│  Other Plugins  │ ← LLM, SQL, GUI, etc.
│  (C++ sync)     │
└─────────────────┘
```

## Message Format

All messages use the same structure across all layers:

```typescript
{
  address: string,          // Target endpoint
  payload: any,             // Request/response data
  options: {
    type?: 'pub'|'sub'|'req'|'res',  // Message type
    req_id?: string,                  // Request correlation ID
    reply_to?: string,                // Custom reply address
    timeout?: number,                 // Timeout in milliseconds
    retry?: number,                   // Retry attempts
    backoff?: number                  // Retry backoff ms
  }
}
```

## Request/Reply Pattern

### JavaScript Client (Browser)

```javascript
const ipc = new IPCClient('ws://localhost:8080/ws');

// Simple request with auto-generated reply address
const result = await ipc.req('llm.generate', {
  prompt: 'Hello world'
}, {
  timeout: 30000
});

// Custom reply routing (send results to log instead)
await ipc.req('async/import', {
  file: 'data.csv'
}, {
  reply_to: 'log/results',  // Route results to log channel
  timeout: 60000
});

// Fire-and-forget to /dev/null
await ipc.req('batch/process', data, {
  reply_to: '/dev/null'
});
```

### Default Reply Pattern

If `reply_to` is not specified:
```
request:  "llm.generate"
reply:    "llm.generate/res/{req_id}"
```

### Custom Reply Pattern

User controls where results go:
```
request:  "llm.generate"
reply_to: "gui/results/custom"  → Results delivered to custom handler
reply_to: "log/channel"         → Results logged
reply_to: "/dev/null"           → Results discarded
```

## C++ Plugin Implementation

### IPC Plugin Invoke

```cpp
extern "C" const char* Invoke(void* instance, const char* address,
                               const char* payload, const char* options) {
    auto* state = static_cast<IpcState*>(instance);
    
    // Parse options for routing
    // options = {"type":"req", "req_id":"abc", "reply_to":"gui/results", "timeout":5000}
    
    if (strcmp(address, "ipc.request") == 0) {
        // Route to WebSocket client
        // Send {address: target, payload: data, options: {...}}
        // Wait for reply on reply_to address
        // Return result or timeout
    }
    
    if (strcmp(address, "ipc.publish") == 0) {
        // Broadcast to all subscribers
    }
    
    if (strcmp(address, "ipc.subscribe") == 0) {
        // Register handler for address pattern
    }
}
```

### Other Plugins Using IPC

```cpp
// Plugin needs async work - route through IPC
extern "C" const char* Invoke(void* instance, const char* address,
                               const char* payload, const char* options) {
    if (strcmp(address, "llm.generate") == 0) {
        // DON'T do long-running work here - blocks app thread
        
        // Instead: Queue work via IPC
        const char* ipc_options = R"({
            "timeout": 30000,
            "reply_to": "llm/results",
            "retry": 3
        })";
        
        // Call IPC plugin to handle async routing
        return invoke_other_plugin("ipc.request", payload, ipc_options);
    }
}
```

## Timeout and Retry

### JavaScript Promise.race Pattern

```javascript
req(addr, payload, opts = {}) {
    const reqId = opts.req_id || this.genId();
    const replyTo = opts.reply_to || `${addr}/res/${reqId}`;
    const timeout = opts.timeout || 5000;
    
    return Promise.race([
        // Wait for reply
        new Promise((resolve, reject) => {
            this.pending.set(reqId, {resolve, reject, replyTo});
            this.send({address: addr, payload, options: {
                type: 'req',
                req_id: reqId,
                reply_to: replyTo
            }});
        }),
        
        // Timeout
        new Promise((_, reject) => {
            setTimeout(() => {
                if (this.pending.has(reqId)) {
                    this.pending.delete(reqId);
                    this.handlers.delete(replyTo);
                    reject(new Error('timeout'));
                }
            }, timeout);
        })
    ]);
}
```

### Automatic Cleanup

- Timeout expires → pending request deleted, handler removed
- Reply arrives → promise resolves, cleanup happens
- No memory leaks from abandoned requests

## Dual-Server Architecture (jam-0 Pattern)

### Multi-Session Deployment Scenarios

```
Scenario 1: Debian Server
┌─────────────────────────┐
│  User 1: ./cjam -tui    │
│  TUI Server :1883       │
└─────────────────────────┘
           ⬇
    No GUI possible
    TUI works standalone

Scenario 2: Workstation - Both Running
┌──────────────────────────┐     ┌──────────────────────────┐
│  User 1: ./cjam -gui     │     │  User 2: ./cjam -tui     │
│  GUI Server :1884        │◀───▶│  TUI Server :1883        │
└──────────────────────────┘     └──────────────────────────┘
           ⬇                                    ⬇
    Wayland/X11/macOS               Bubble Tea terminal
    Presence announcements           Can sync with GUI
    Can sync with TUI                Can operate alone

Scenario 3: Multiple TUI Sessions
┌──────────────────────────┐
│  User 1: ./cjam -tui     │
│  TUI Server :1883        │
└──────────────────────────┘
           ⬇
┌──────────────────────────┐
│  User 2: ./cjam -tui     │
│  TUI Server :1884        │  (different port)
└──────────────────────────┘
    Both independent OR
    Could share via IPC
```

### CLI Automation Pattern (COM/OLE Heritage)

This pattern mirrors Microsoft COM Automation from the 1990s, where command-line tools could control GUI applications programmatically:

```
Classic COM Automation (Word):
┌─────────────────────────┐
│  Word.exe (GUI)         │
│  COM IDispatch Server   │ ◀─── Automation Interface
└─────────────────────────┘
           ▲
           │ COM/OLE CreateObject()
           │ Invoke methods via IDispatch
           │
┌─────────────────────────┐
│  wordcli.exe            │
│  VBScript/JScript       │
│  Command Line Control   │
└─────────────────────────┘

Example: wordcli.exe "doc.docx" --convert-to-pdf
         ↓ CreateObject("Word.Application")
         ↓ doc = word.Documents.Open("doc.docx")
         ↓ doc.ExportAsFixedFormat(Type=PDF)
         ↓ Result returned to CLI
```

**Modern Equivalent (WebSocket + JSON):**

```
┌─────────────────────────┐
│  ./cjam -gui            │
│  GUI Server :1884       │ ◀─── WebSocket IPC Server
│  Wayland/X11/macOS      │
└─────────────────────────┘
           ▲
           │ WebSocket + JSON messages
           │ address/payload/options
           │
┌─────────────────────────┐
│  ./cjam -tui            │
│  TUI Client             │
│  Bubble Tea Terminal    │
└─────────────────────────┘

Example: TUI sends gui/request/config
         ↓ GUI receives, reads window state
         ↓ GUI sends gui/response/config
         ↓ TUI receives result, displays in terminal
```

**Key Parallels:**

| COM Automation (1990s) | WebSocket IPC (2025) |
|------------------------|----------------------|
| IDispatch interface | WebSocket JSON messages |
| CreateObject("Word.Application") | Connect to GUI broker :1884 |
| Invoke methods on COM objects | Send `gui/request/*` topics |
| CLI doesn't render, delegates to GUI | TUI queries GUI state via IPC |
| GUI runs as automation server | GUI runs MQTT/WebSocket server |
| VBScript/JScript scripting | JavaScript/JSON messages |
| Process-to-process via COM | Process-to-process via WebSocket |

**Benefits of Modern Pattern:**

- **Cross-platform**: Works on Windows, Linux, macOS (COM was Windows-only)
- **Language agnostic**: Any language with WebSocket support (COM required COM-compatible languages)
- **Network transparent**: Can control GUI over network (COM required DCOM for remote)
- **Simpler**: JSON messages vs COM marshalling complexity
- **Debuggable**: WebSocket traffic easily inspected (COM required specialized tools)
- **Bidirectional**: Both sides can be client and server (COM was typically one-way)

**Usage Examples:**

```bash
# CLI controls GUI window state
./cjam -tui --send "gui/request/window-maximize"

# CLI queries GUI for current document
./cjam -tui --send "gui/request/active-document"

# CLI triggers GUI action
./cjam -tui --send "gui/request/save-all"

# GUI announces state changes to listening TUI
# (GUI publishes, TUI subscribes)
gui/event/document-modified
gui/event/window-focused
```

**Implementation Notes:**

- TUI operates standalone on headless servers (no GUI available)
- When GUI is present, TUI can delegate rendering/UI tasks via IPC
- GUI doesn't depend on TUI - runs independently
- Both can run simultaneously with mutual automation
- No registry or COM registration required
- No DLL hell or version conflicts

### GUI and TUI Both Running

When both GUI and TUI are active, they discover each other via presence announcements:

```
┌─────────────┐                    ┌─────────────┐
│  TUI Server │                    │  GUI Server │
│  :1883      │                    │  :1884      │
└──────┬──────┘                    └──────┬──────┘
       │                                  │
       │  1. Connect as client ───────────▶
       │     to GUI broker                │
       │                                  │
       │  2. Publish presence             │
       │     "gui/system/tui-presence"    │
       │                                  │
       │                                  │  3. Receive presence
       │                                  │     Subscribe trigger
       │                                  │
       │  4. Connect as client ◀───────────
       │     to TUI broker                │
       │                                  │
       │  5. Publish presence             │
       │     "tui/system/gui-presence"    │
       │                                  │
       │  ✅ Bidirectional ready          │
       │                                  │
       │  Request/Reply ◀──────────────▶  │
       │  Heartbeat ◀──────────────────▶  │
       │  Pub/Sub ◀────────────────────▶  │
```

### Presence Announcement Pattern

```go
// TUI Server subscribes to presence on its own broker
tuiServer.Subscribe("tui/system/gui-presence", func(payload) {
    // GUI connected to TUI broker
    // Now TUI connects back to GUI broker
    go ipc.NewClient(container, "gui")
})

// TUI Client announces to GUI broker when connecting
tuiClient.Publish("gui/system/tui-presence", []byte("tui-connected"))

// GUI Server subscribes to presence on its own broker  
guiServer.Subscribe("gui/system/tui-presence", func(payload) {
    // TUI connected to GUI broker
    // Now GUI connects back to TUI broker
    go ipc.NewClient(container, "tui")
})

// GUI Client announces to TUI broker when connecting
guiClient.Publish("tui/system/gui-presence", []byte("gui-connected"))
```

### Request/Reply Topics

Each mode exposes request handlers on its own broker:

```
TUI Server Topics:
  tui/request/config   → tui/response/config
  tui/request/defs     → tui/response/defs
  tui/request/assets   → tui/response/assets
  tui/system/heartbeat (periodic)

GUI Server Topics:
  gui/request/config   → gui/response/config
  gui/request/defs     → gui/response/defs  
  gui/request/assets   → gui/response/assets
  gui/system/heartbeat (periodic)
```

### Implementation Notes

- Each mode runs its own MQTT/WebSocket server on different ports
- Presence announcements trigger automatic reverse connection
- Heartbeats maintain connection state
- Request/reply enables state synchronization
- No hardcoded coupling - discovery via presence protocol

## Routing Between Processes

Based on the `_other_/pico` project architecture:

### Inproc vs Remote

```typescript
// CoreClientInprocService - same process via WebSocket
await inproc.signal('db.query', {sql: 'SELECT ...'}, {});

// CoreClientRemoteService - different process/server
await remote.signal('external/api', {data}, {timeout: 10000});

// Both use same signature: address, payload, options
```

### Route Execution Pattern

```typescript
class RouteBpmnGetAll extends RouteBase {
    public async exec(message: {
        address: string,
        payload: any,
        options: any
    }): Promise<any> {
        // Execute SQL stored procedure
        const retval = await this.trunk.boss.executeOn('etl', `
            SELECT * FROM etl_bpmn.isp_get_all();
        `);
        return retval.rows;
    }
}
```

### DAG Execution via SQL

Routes use `exec({address, payload, options})` to run SQL DAGs:
- Address routes to correct stored procedure
- Payload contains query parameters
- Options control timeout, retry, result routing
- SQL DAG handles complex multi-step workflows

## Plugin Design Principles

### ✅ DO: Keep Invoke Fast

```cpp
// Good - returns immediately
const char* result = R"({"queued":true,"job_id":"abc123"})";
return result;
```

### ❌ DON'T: Block in Invoke

```cpp
// Bad - blocks app thread for 30 seconds
std::this_thread::sleep_for(std::chrono::seconds(30));
return result;
```

### ✅ DO: Use IPC for Async Work

```cpp
// Queue long work via IPC, return immediately
Invoke("ipc.request", payload, R"({"timeout":30000,"reply_to":"results"})");
return R"({"status":"queued"})";
```

## Transport Agnostic

The `address, payload, options` pattern works over:
- **WebSocket**: Real-time bidirectional (current focus)
- **HTTP**: Request/response (same pattern)
- **MQTT**: Pub/sub (same pattern)
- **Unix Sockets**: Local IPC (same pattern)
- **Named Pipes**: Windows IPC (same pattern)

## Reference Implementation

### Lineage: jam-0 → jam-2 → jam-1 → pico → mono

**jam-2** is the spiritual parent of mono. The architecture evolved:
1. **jam-0**: Production dual-server system (GUI + TUI, mutual discovery)
2. **jam-2**: Discovered the `address/payload/options` pattern via `efx` DI system
3. **jam-1**: Refined WebSocket request/reply with Promise.race
4. **pico**: Production TypeScript + DAG/SQL routing
5. **mono**: C++ plugin system with same signature

### Source Documents

**jam-0 (production dual-server)**:
- **TUI/GUI IPC**: `jam-0/assets/docs/tui-gui-ipc-details.md`
  - Both GUI and TUI run their own MQTT/WebSocket servers
  - Each mode announces presence when connecting
  - Bidirectional handshake: TUI → GUI broker, GUI → TUI broker
  - Request/reply pattern: `{mode}/request/{resource}` → `{mode}/response/{resource}`
  - Heartbeat: `tui/system/heartbeat`, `gui/system/heartbeat`
- **Presence Protocol**: `jam-0/internal/tech/ipc/`
  - TUI publishes: `gui/system/tui-presence` when connecting to GUI broker
  - GUI publishes: `tui/system/gui-presence` when connecting to TUI broker
  - Servers subscribe to presence topics, trigger reverse connection on announcement
  - Enables fully bidirectional communication when both are running
- **Message Topics**:
  - Config: `tui/request/config` → `tui/response/config`
  - Defs: `tui/request/defs` → `tui/response/defs`
  - Assets: `tui/request/assets` → `tui/response/assets`
  - Same pattern for GUI: `gui/request/*` → `gui/response/*`

**jam-2 (spiritual parent)**:
- **Core Philosophy**: `jam-2/refs/docs/GUIDE.md`
  - "decompose → refactor → DRY → repeat until nothing meaningful to refactor"
  - Large set of uncoupled stateless functions
  - Replace if/else with `efx.Once` + `efx.Many`
  - One function per file, one reason to change
- **IPC Signature**: `jam-2/refs/docs/ipc.md`
  - Uniform `{address, payload, options}` across all IPC implementations
  - WebSocket → messages → rxgo → sqlite
  - MQTT → messages → rxgo → sqlite
- **Message Design**: `jam-2/refs/docs/archive/WS_SIGNATURE_DESIGN.md`
  - Detailed Options structure (retry, backoff, timeout, reply_to)
  - Request/reply correlation via req_id
  - QoS features (persist, ack)
- **SQL Execute**: `jam-2/internal/tech/dag/archive/sql-execute-analysis.md`
  - `ipcRouteProxy(address, payload, options)` pattern
  - Routes between inproc and remote servers
  - Text selection → execute → IPC → SQL DAG

**jam-1 (refinement)**:
- **WebSocket Design**: `jam-1/refs/docs/archive/WS_DESIGN.md`
  - Promise.race timeout pattern
  - Custom reply routing
  - Hub/Conn architecture

**pico (production TypeScript)**:
- **Route Pattern**: `_other_/pico/module-server/src/route/.base.ts`
  - `exec({address, payload, options})` in every route
  - SQL DAG execution via stored procedures
- **Inproc IPC**: `_other_/pico/client-web/src/app/app-core/core-client-inproc-ipc.service.ts`
  - `signal(address, payload, options)` for WebSocket
  - Sanity checks, reconnect handling

### Key Philosophy from jam-2

**Core Principle:**
> "decompose everything into stateless functions, eliminate branches through composition, repeat until nothing meaningful remains to refactor"

**Applied to mono:**
- Each plugin = stateless function (Invoke)
- Control plugin = efx.Many discovery (finds plugins by pattern)
- No if/else for routing = address-based dispatch
- One function per plugin = single responsibility
- Uniform signature = composability

**efx Pattern → Plugin Pattern:**
```go
// jam-2: efx discovery
handlers, _ := efx.Many[Handler](bag, "handler")
for _, h := range handlers {
    if h.Matches(msg) {
        return h.Handle(msg)
    }
}
```

```cpp
// mono: plugin discovery
for (auto& plugin : plugins) {
    if (plugin.matches(address)) {
        return plugin.invoke(instance, address, payload, options);
    }
}
```

### Key Features Proven Across All Versions
- ✅ Dual-server architecture (jam-0: GUI + TUI both running)
- ✅ Presence announcement protocol (jam-0: mutual discovery)
- ✅ Bidirectional handshake (jam-0: each connects to other's broker)
- ✅ Request/reply with timeout (jam-0, jam-1, pico)
- ✅ Custom reply routing via `reply_to` (jam-1, pico)
- ✅ Retry with backoff (jam-2 WS_SIGNATURE_DESIGN.md)
- ✅ Pub/sub pattern (jam-0, jam-2, jam-1)
- ✅ Multi-server routing (jam-0 dual server, pico inproc vs remote)
- ✅ DAG execution via SQL (pico)
- ✅ Browser Promise.race integration (jam-1, pico)
- ✅ Heartbeat system (jam-0: `{mode}/system/heartbeat`)
- ✅ No event loop blocking (all versions)
- ✅ Stateless composition (jam-2 philosophy)
- ✅ Uniform signature across languages (Go, TypeScript, C++)

## Benefits

1. **Simple Synchronous Plugins**: No async complexity in plugin code
2. **Flexible Routing**: Results go where you want via `reply_to`
3. **Timeout Built-in**: No hanging requests
4. **Transport Agnostic**: Same pattern works everywhere
5. **JavaScript Integration**: Promise.race prevents blocking
6. **Multi-Process**: Route between services seamlessly
7. **Proven Pattern**: Used in production (_other_/pico)

## Current Status

- ✅ Plugin structure created (`libs/ipc/`)
- ✅ 4-function pattern implemented (Attach/Detach/Invoke/Report)
- ⏸️ WebSocket server implementation (TODO)
- ⏸️ Request/reply routing (TODO)
- ⏸️ Pub/sub management (TODO)
- ⏸️ Timeout handling (TODO)

## Next Steps

1. Implement WebSocket server in IPC plugin
2. Add request/reply state management
3. Implement pub/sub topic routing
4. Add timeout and retry logic
5. Create JavaScript client library
6. Test with browser frontend
7. Document multi-server routing patterns
