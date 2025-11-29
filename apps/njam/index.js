import koffi from 'koffi';
import path from 'path';
import fs from 'fs';
import { fileURLToPath } from 'url';
import { dirname } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Define libsinfo struct
const libsinfo = koffi.struct('libsinfo', {
    plugin_type: 'const char *',
    product: 'const char *',
    description_long: 'const char *',
    description_short: 'const char *',
    plugin_id: 'unsigned long'
});

function controlBoot() {
    console.log("HOST: Discovering control plugin...");
    
    // Running from apps/njam/, scan current directory
    const exeDir = fs.realpathSync(__dirname);
    console.log(`HOST: Scanning directory: ${exeDir}`);
    
    const libExt = process.platform === 'darwin' ? '.dylib' : 
                   process.platform === 'win32' ? '.dll' : '.so';
    
    const entries = fs.readdirSync(exeDir);
    
    for (const filename of entries) {
        const filepath = path.join(exeDir, filename);
        const stat = fs.statSync(filepath);
        
        if (!stat.isFile()) {
            continue;
        }
        
        if (!filename.endsWith(libExt)) {
            continue;
        }
        
        if (!filename.startsWith('lib')) {
            continue;
        }
        
        console.log(`HOST: Found candidate: ${filename}`);
        
        let lib;
        try {
            lib = koffi.load(filepath);
        } catch (err) {
            console.log(`HOST: Failed to load ${filename}: ${err.message}`);
            continue;
        }
        
        // Check for required functions
        let Attach, Detach, Invoke, Report;
        try {
            Attach = lib.func('Attach', 'int', ['void *', 'char *', 'size_t']);
            Detach = lib.func('Detach', 'int', ['char *', 'size_t']);
            Invoke = lib.func('Invoke', 'const char *', ['const char *', 'const char *', 'const char *']);
            Report = lib.func('Report', 'int', ['char *', 'size_t', koffi.pointer(libsinfo)]);
        } catch (err) {
            console.log(`HOST: ${filename} missing required functions (Attach/Detach/Invoke/Report)`);
            continue;
        }
        
        console.log(`HOST: ${filename} has valid plugin interface`);
        
        // Call Report to check plugin type
        const errBuf = Buffer.alloc(256);
        const infoPtr = koffi.alloc(libsinfo, 1);
        const result = Report(errBuf, errBuf.length, infoPtr);
        
        if (result === 0) {
            const errMsg = errBuf.toString('utf8').split('\0')[0];
            console.log(`HOST: ${filename} Report failed: ${errMsg}`);
            continue;
        }
        
        const info = koffi.decode(infoPtr, libsinfo);
        const pluginType = info.plugin_type || '';
        
        if (pluginType === 'control') {
            console.log(`HOST: ${filename} identified as control plugin (type=${pluginType}, id=0x${info.plugin_id.toString(16)})`);
            return { lib, Attach, Detach, Invoke };
        }
        
        console.log(`HOST: ${filename} is not control plugin (type=${pluginType})`);
    }
    
    console.error(`Error: No control plugin found in ${exeDir}`);
    return null;
}

function controlBind(fns) {
    if (!fns.Attach || !fns.Detach || !fns.Invoke) {
        console.error("Error: Control plugin missing required functions");
        return false;
    }
    console.log("Resolved control plugin functions (Attach/Detach/Invoke)");
    return true;
}

function controlAttach(fns) {
    console.log("Resolved control plugin functions (Attach/Detach/Invoke)");
    
    // Create callback wrapper
    const InvokeCallback = koffi.pointer(koffi.proto('const char *InvokeCallback(const char *, const char *, const char *)'));
    const invokePtr = koffi.register(function(addr, payload, opts) {
        return fns.Invoke(addr, payload, opts);
    }, InvokeCallback);
    
    // Call Attach
    const errBuf = Buffer.alloc(256);
    const attachResult = fns.Attach(invokePtr, errBuf, errBuf.length);
    
    if (attachResult === 0) {
        const errMsg = errBuf.toString('utf8').split('\0')[0];
        console.error(`Error: Control plugin Attach failed: ${errMsg}`);
        return false;
    }
    
    console.log("Control plugin attached successfully");
    return true;
}

function controlInvoke(fns) {
    const response = fns.Invoke("control.run", "{}", "{}");
    console.log(`Control plugin result: ${response}`);
}

function controlDetach(fns) {
    const errBuf = Buffer.alloc(256);
    fns.Detach(errBuf, errBuf.length);
}

console.log("=== NODE.JS HOST ===");

const fns = controlBoot();
if (!fns) {
    process.exit(1);
}

if (!controlBind(fns)) {
    process.exit(1);
}

if (!controlAttach(fns)) {
    process.exit(1);
}

controlInvoke(fns);
controlDetach(fns);

console.log("=== NODE.JS HOST COMPLETE ===");
