```
../                         # someplace else
│
└── jam/                    # you could be anywhere, doing anything. .. and youre here.  
    │                       # ( wierdo )
    ├── apps/               # programming across languages ( pals )
    │   │                    lang       version        Est. Users
    │   ├── cjam/            C++        Clang 17.0.0    4.5M
    │   ├── gjam/            Go         1.25.1          3.0M
    │   ├── rjam/            Rust       1.89.0          2.8M
    │   ├── njam/            Node.js    24.7.0         18.0M
    │   ├── pjam/            Python     3.14.0         16.0M
    │   ├── jjam/            Java       25.0.1          9.0M
    │   ├── djam/            .NET       10.0.100        6.5M
    │   ├── kjam/            Kotlin     2.2.21          4.5M
    │   ├── sjam/            Swift      6.2             2.5M
    │   ├── ljam/            LuaJIT     2.1             1.5M
    │   ├── zjam/            Zig        0.15.2            50K
    │   ├── hjam/            Haskell    9.12.2           100K
    │   ├── ojam/            OCaml      5.4.0             50K
    │   └── vjam/            V          0.4.12            20K
    ├── libs/
    │   ├── bag/             2 handlers
    │   ├── cli/             0 handlers
    │   ├── cmd/             0 handlers
    │   ├── control/         discovery + routing
    │   ├── efs/             0 handlers
    │   ├── ege/             0 handlers
    │   ├── gui/             0 handlers
    │   ├── ipc/             0 handlers
    │   ├── llm/             0 handlers
    │   ├── log/             0 handlers
    │   ├── lua/             0 handlers
    │   ├── res/             0 handlers
    │   ├── aui/             0 handlers
    │   ├── sql/             0 handlers
    │   ├── tui/             0 handlers
    │   └── www/             0 handlers
    │
    ├── dist/
    │   │                    Language   Binary     Avg Time  vs  C++ Size     vs C++ Speed
    │   ├── cjam             C++          39.0KB       70ms                              
    │   ├── gjam             Go            2.4MB      132ms    62.0x larger      1.9x slower
    │   ├── rjam             Rust        471.0KB       88ms    12.0x larger      1.3x slower
    │   ├── njam/            Node.js      84.0MB      460ms  2154.0x larger      6.6x slower
    │   ├── pjam/            Python      788.0KB       95ms    20.0x larger      1.4x slower
    │   ├── jjam             Java          1.8MB      594ms    46.0x larger      8.5x slower
    │   ├── djam             .NET        121.0KB      505ms     3.1x larger      7.2x slower
    │   ├── kjam.jar         Kotlin        4.9MB     2449ms    126.0x larger     35.0x slower
    │   ├── sjam             Swift        56.0KB      536ms     1.4x larger      7.7x slower
    │   ├── ljam/            LuaJIT      788.0KB       78ms    20.0x larger      1.1x slower
    │   ├── zjam             Zig         230.0KB      141ms     5.9x larger      2.0x slower
    │   ├── hjam             Haskell     132.0KB      156ms     3.4x larger      2.2x slower
    │   ├── ojam             OCaml         1.1MB       69ms    28.0x larger      1.0x slower
    │   ├── vjam             V           193.0KB       68ms     4.9x larger      1.0x faster
    │   │
    │   ├── lib*.dylib       16 plugin dylibs
    │   └── (build outputs)
    │
    └── Makefile

