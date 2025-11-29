import Foundation

// C function pointer types matching plugin contract
typealias InvokeFn = @convention(c) (UnsafePointer<CChar>?, UnsafePointer<CChar>?, UnsafePointer<CChar>?) -> UnsafePointer<CChar>?
typealias AttachFn = @convention(c) (InvokeFn?, UnsafeMutablePointer<CChar>?, Int) -> Bool
typealias DetachFn = @convention(c) (UnsafeMutablePointer<CChar>?, Int) -> Bool
typealias ReportFn = @convention(c) (UnsafeMutablePointer<CChar>, Int, UnsafeMutableRawPointer) -> Bool

struct ControlFns {
    let handle: UnsafeMutableRawPointer
    let attach: AttachFn
    let detach: DetachFn
    let invoke: InvokeFn
}

func controlBoot() -> ControlFns? {
    print("HOST: Discovering control plugin...")
    
    let exePath = CommandLine.arguments[0]
    let exeDir = URL(fileURLWithPath: exePath).deletingLastPathComponent().path
    
    print("HOST: Scanning directory: \(exeDir)")
    
    #if os(macOS)
    let libExt = ".dylib"
    #elseif os(Linux)
    let libExt = ".so"
    #else
    let libExt = ".dll"
    #endif
    
    guard let files = try? FileManager.default.contentsOfDirectory(atPath: exeDir) else {
        fputs("Error: Failed to read directory\n", stderr)
        return nil
    }
    
    for filename in files {
        if !filename.hasSuffix(libExt) { continue }
        if !filename.hasPrefix("lib") { continue }
        
        print("HOST: Found candidate: \(filename)")
        
        let libPath = "\(exeDir)/\(filename)"
        
        guard let handle = dlopen(libPath, RTLD_LAZY) else {
            if let error = dlerror() {
                print("HOST: Failed to load \(filename): \(String(cString: error))")
            }
            continue
        }
        
        guard let attachPtr = dlsym(handle, "Attach"),
              let detachPtr = dlsym(handle, "Detach"),
              let invokePtr = dlsym(handle, "Invoke") else {
            print("HOST: \(filename) missing required functions (Attach/Detach/Invoke)")
            dlclose(handle)
            continue
        }
        
        print("HOST: \(filename) has valid plugin interface")
        
        guard let reportPtr = dlsym(handle, "Report") else {
            print("HOST: \(filename) missing Report function")
            dlclose(handle)
            continue
        }
        
        let report = unsafeBitCast(reportPtr, to: ReportFn.self)
        
        // Allocate buffer for LibsInfo struct (5 pointers + 1 uint64)
        let infoSize = MemoryLayout<UnsafeRawPointer>.size * 4 + MemoryLayout<UInt64>.size
        let infoBuffer = UnsafeMutableRawPointer.allocate(byteCount: infoSize, alignment: 8)
        defer { infoBuffer.deallocate() }
        
        var errBuf = [CChar](repeating: 0, count: 256)
        
        let result = errBuf.withUnsafeMutableBufferPointer { errPtr in
            report(errPtr.baseAddress!, 256, infoBuffer)
        }
        
        if !result {
            let errMsg = String(cString: errBuf)
            print("HOST: \(filename) Report failed: \(errMsg)")
            dlclose(handle)
            continue
        }
        
        // Read plugin_type from first pointer in struct
        let pluginTypePtr = infoBuffer.load(as: UnsafePointer<CChar>?.self)
        
        guard let pluginTypePtrUnwrapped = pluginTypePtr else {
            print("HOST: \(filename) returned null plugin_type")
            dlclose(handle)
            continue
        }
        
        let pluginType = String(cString: pluginTypePtrUnwrapped)
        
        // Read plugin_id (at offset: 4 pointers)
        let pluginId = infoBuffer.load(fromByteOffset: MemoryLayout<UnsafeRawPointer>.size * 4, as: UInt64.self)
        
        if pluginType == "control" {
            print("HOST: \(filename) identified as control plugin (type=\(pluginType), id=0x\(String(pluginId, radix: 16)))")
            
            let attach = unsafeBitCast(attachPtr, to: AttachFn.self)
            let detach = unsafeBitCast(detachPtr, to: DetachFn.self)
            let invoke = unsafeBitCast(invokePtr, to: InvokeFn.self)
            
            return ControlFns(handle: handle, attach: attach, detach: detach, invoke: invoke)
        }
        
        print("HOST: \(filename) is not control plugin (type=\(pluginType))")
        dlclose(handle)
    }
    
    fputs("Error: No control plugin found in \(exeDir)\n", stderr)
    return nil
}

func controlBind(_ fns: ControlFns) -> Bool {
    print("Resolved control plugin functions (Attach/Detach/Invoke)")
    return true
}

func controlAttach(_ fns: ControlFns) -> Bool {
    print("Resolved control plugin functions (Attach/Detach/Invoke)")
    
    var errBuf = [CChar](repeating: 0, count: 256)
    let result = errBuf.withUnsafeMutableBufferPointer { errPtr in
        fns.attach(fns.invoke, errPtr.baseAddress, errPtr.count)
    }
    
    if !result {
        let errMsg = String(cString: errBuf)
        fputs("Error: Control plugin Attach failed: \(errMsg)\n", stderr)
        return false
    }
    
    print("Control plugin attached successfully")
    return true
}

func controlInvoke(_ fns: ControlFns) {
    let addr = "control.run"
    let payload = "{}"
    let options = "{}"
    
    let response = addr.withCString { addrPtr in
        payload.withCString { payloadPtr in
            options.withCString { optionsPtr in
                fns.invoke(addrPtr, payloadPtr, optionsPtr)
            }
        }
    }
    
    if let responsePtr = response {
        let responseStr = String(cString: responsePtr)
        print("Control plugin result: \(responseStr)")
    }
}

func controlDetach(_ fns: ControlFns) {
    var errBuf = [CChar](repeating: 0, count: 256)
    _ = errBuf.withUnsafeMutableBufferPointer { errPtr in
        fns.detach(errPtr.baseAddress, errPtr.count)
    }
    dlclose(fns.handle)
}

print("=== SWIFT HOST ===")

guard let fns = controlBoot() else {
    exit(1)
}

guard controlBind(fns) else {
    exit(1)
}

guard controlAttach(fns) else {
    exit(1)
}

controlInvoke(fns)
controlDetach(fns)

print("=== SWIFT HOST COMPLETE ===")
