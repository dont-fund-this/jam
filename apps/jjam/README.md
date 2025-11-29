# Java Host (jjam)

## Requirements
- Java JDK 11 or higher
- Install from: https://www.oracle.com/java/technologies/downloads/ or `brew install openjdk`

## Build
```bash
make jjam
```

## Run
```bash
make run-jjam
```

## Pattern
Follows the same 5-step pattern as all other hosts:
1. `controlBoot()` - Discovers control plugin in same directory as JAR
2. `controlBind()` - Validates functions
3. `controlAttach()` - Attaches dispatch callback
4. `controlInvoke()` - Runs "control.run"
5. `controlDetach()` - Cleanup

## Implementation
- Uses JNA (Java Native Access) for FFI
- JAR built to `dist/jjam.jar`
- Runs from `dist/` where all `.dylib` files are located
- Same directory scanning logic as cjam/gjam/rjam
