use libloading::{Library, Symbol};
use std::ffi::{CStr, CString};
use std::fs;
use std::ops::Deref;
use std::os::raw::{c_char, c_int};

// C function pointer types matching plugin contract
type DispatchFn = unsafe extern "C" fn(*const c_char, *const c_char, *const c_char) -> *const c_char;
type AttachFn = unsafe extern "C" fn(DispatchFn, *mut c_char, usize) -> c_int;
type DetachFn = unsafe extern "C" fn(*mut c_char, usize) -> c_int;
type InvokeFn = unsafe extern "C" fn(*const c_char, *const c_char, *const c_char) -> *const c_char;

#[repr(C)]
struct LibsInfo {
    plugin_type: *const c_char,
    product: *const c_char,
    description_long: *const c_char,
    description_short: *const c_char,
    plugin_id: usize,
}

type ReportFn = unsafe extern "C" fn(*mut c_char, usize, *mut LibsInfo) -> c_int;

struct ControlFns {
    attach: AttachFn,
    detach: DetachFn,
    invoke: InvokeFn,
}

fn control_boot(argv0: &str) -> Option<Library> {
    println!("HOST: Discovering control plugin...");
    
    let exe_path = std::fs::canonicalize(argv0).expect("Failed to resolve executable path");
    let exe_dir = exe_path.parent().expect("Failed to get executable directory");
    
    println!("HOST: Scanning directory: {}", exe_dir.display());
    
    #[cfg(target_os = "macos")]
    let lib_ext = ".dylib";
    #[cfg(target_os = "linux")]
    let lib_ext = ".so";
    #[cfg(target_os = "windows")]
    let lib_ext = ".dll";
    
    let entries = match fs::read_dir(exe_dir) {
        Ok(entries) => entries,
        Err(e) => {
            eprintln!("Error: Failed to read directory: {}", e);
            return None;
        }
    };
    
    for entry in entries {
        let entry = match entry {
            Ok(e) => e,
            Err(_) => continue,
        };
        
        let path = entry.path();
        if !path.is_file() {
            continue;
        }
        
        let filename = match path.file_name() {
            Some(name) => name.to_string_lossy(),
            None => continue,
        };
        
        // Check extension
        if !filename.ends_with(lib_ext) {
            continue;
        }
        
        // Only consider libs starting with "lib"
        if !filename.starts_with("lib") {
            continue;
        }
        
        println!("HOST: Found candidate: {}", filename);
        
        // Try to load it
        let lib = match unsafe { Library::new(&path) } {
            Ok(lib) => lib,
            Err(e) => {
                println!("HOST: Failed to load {}: {}", filename, e);
                continue;
            }
        };
        
        // Check for required functions
        let attach: Result<Symbol<AttachFn>, _> = unsafe { lib.get(b"Attach") };
        let detach: Result<Symbol<DetachFn>, _> = unsafe { lib.get(b"Detach") };
        let invoke: Result<Symbol<InvokeFn>, _> = unsafe { lib.get(b"Invoke") };
        let report: Result<Symbol<ReportFn>, _> = unsafe { lib.get(b"Report") };
        
        if attach.is_err() || detach.is_err() || invoke.is_err() {
            println!("HOST: {} missing required functions (Attach/Detach/Invoke)", filename);
            continue;
        }
        
        println!("HOST: {} has valid plugin interface", filename);
        
        // Call Report to check if it's control plugin
        let report = match report {
            Ok(r) => r,
            Err(_) => {
                println!("HOST: {} missing Report function", filename);
                continue;
            }
        };
        
        let mut desc = LibsInfo {
            plugin_type: std::ptr::null(),
            product: std::ptr::null(),
            description_long: std::ptr::null(),
            description_short: std::ptr::null(),
            plugin_id: 0,
        };
        let mut err_buf = vec![0u8; 256];
        
        let result = unsafe {
            report(err_buf.as_mut_ptr() as *mut c_char, err_buf.len(), &mut desc)
        };
        
        if result == 0 {
            let err_msg = unsafe {
                CStr::from_ptr(err_buf.as_ptr() as *const c_char).to_string_lossy()
            };
            println!("HOST: {} Report failed: {}", filename, err_msg);
            continue;
        }
        
        // Check if it's the control plugin
        if desc.plugin_type.is_null() {
            println!("HOST: {} returned null plugin_type", filename);
            continue;
        }
        
        let plugin_type = unsafe {
            CStr::from_ptr(desc.plugin_type).to_string_lossy()
        };
        
        if plugin_type == "control" {
            println!(
                "HOST: {} identified as control plugin (type={}, id=0x{:x})",
                filename, plugin_type, desc.plugin_id
            );
            return Some(lib);
        }
        
        println!("HOST: {} is not control plugin (type={})", filename, plugin_type);
    }
    
    eprintln!("Error: No control plugin found in {}", exe_dir.display());
    None
}

fn control_bind(lib: &Library) -> Option<ControlFns> {
    let attach: Symbol<AttachFn> = unsafe {
        lib.get(b"Attach").ok()?
    };
    let detach: Symbol<DetachFn> = unsafe {
        lib.get(b"Detach").ok()?
    };
    let invoke: Symbol<InvokeFn> = unsafe {
        lib.get(b"Invoke").ok()?
    };
    
    println!("Resolved control plugin functions (Attach/Detach/Invoke)");
    
    Some(ControlFns {
        attach: *attach.deref(),
        detach: *detach.deref(),
        invoke: *invoke.deref(),
    })
}

fn control_attach(fns: &ControlFns) -> bool {
    println!("Resolved control plugin functions (Attach/Detach/Invoke)");
    
    // Get raw function pointer for invoke to pass as dispatch
    let invoke_ptr: DispatchFn = fns.invoke;
    
    // Call Attach - pass control's own Invoke as dispatch function
    let mut err_buf = vec![0u8; 256];
    let result = unsafe {
        (fns.attach)(invoke_ptr, err_buf.as_mut_ptr() as *mut c_char, err_buf.len())
    };
    
    if result == 0 {
        let err_msg = unsafe {
            CStr::from_ptr(err_buf.as_ptr() as *const c_char).to_string_lossy()
        };
        eprintln!("Error: Control plugin Attach failed: {}", err_msg);
        return false;
    }
    
    println!("Control plugin attached successfully");
    true
}

fn control_invoke(fns: &ControlFns) -> String {
    let addr = CString::new("control.run").unwrap();
    let empty_payload = CString::new("{}").unwrap();
    let empty_options = CString::new("{}").unwrap();
    
    let response = unsafe {
        (fns.invoke)(addr.as_ptr(), empty_payload.as_ptr(), empty_options.as_ptr())
    };
    
    let response_str = unsafe {
        CStr::from_ptr(response).to_string_lossy().into_owned()
    };
    
    println!("Control plugin result: {}", response_str);
    response_str
}

fn control_detach(fns: &ControlFns) {
    let mut err_buf = vec![0u8; 256];
    let result = unsafe {
        (fns.detach)(err_buf.as_mut_ptr() as *mut c_char, err_buf.len())
    };
    
    if result == 0 {
        let err_msg = unsafe {
            CStr::from_ptr(err_buf.as_ptr() as *const c_char).to_string_lossy()
        };
        eprintln!("Warning: Control plugin Detach failed: {}", err_msg);
    }
}

fn main() {
    println!("=== RUST HOST ===");
    
    let argv0 = std::env::args().next().expect("Failed to get argv[0]");
    
    let lib = match control_boot(&argv0) {
        Some(lib) => lib,
        None => std::process::exit(1),
    };
    
    let fns = match control_bind(&lib) {
        Some(fns) => fns,
        None => {
            eprintln!("Error: Failed to bind control plugin functions");
            std::process::exit(1);
        }
    };
    
    if !control_attach(&fns) {
        std::process::exit(1);
    }
    
    control_invoke(&fns);
    control_detach(&fns);
    
    println!("=== RUST HOST COMPLETE ===");
}
