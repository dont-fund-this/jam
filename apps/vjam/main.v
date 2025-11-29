// V host implementation using C FFI for dynamic library loading
module main

import os
import dl

#flag darwin -Wl,-rpath,@executable_path
#flag -I .
#include "plugin.h"

// V struct declaration (references the C typedef in plugin.h)
struct C.LibsInfo {
	plugin_type       &char
	product           &char
	description_long  &char
	description_short &char
	plugin_id         u64
}

// Function pointer types matching plugin contract
type InvokeFn = fn (&char, &char, &char) &char
type AttachFn = fn (InvokeFn, &char, int) bool
type DetachFn = fn (&char, int) bool
type ReportFn = fn (&char, int, &C.LibsInfo) bool

struct ControlFns {
	handle     voidptr
	attach_ptr voidptr
	detach_ptr voidptr
	invoke_ptr voidptr
}

// Step 1: Control Boot - Discover control plugin
fn control_boot() ?ControlFns {
	println('HOST: Discovering control plugin...')
	
	exe_path := os.executable()
	exe_dir := os.dir(exe_path)
	println('HOST: Scanning directory: ${exe_dir}')
	
	lib_ext := '.dylib' // macOS
	
	files := os.ls(exe_dir) or { return none }
	
	for filename in files {
		// Only consider libs starting with "lib" and ending with extension
		if !filename.starts_with('lib') || !filename.ends_with(lib_ext) {
			continue
		}
		
		println('HOST: Found candidate: ${filename}')
		
		lib_path := os.join_path(exe_dir, filename)
		handle := dl.open(lib_path, dl.rtld_lazy)
		if handle == voidptr(0) {
			println('HOST: Failed to load ${filename}')
			continue
		}
		
		// Check for required functions
		attach_ptr := dl.sym(handle, 'Attach')
		if attach_ptr == voidptr(0) {
			println('HOST: ${filename} missing Attach function')
			dl.close(handle)
			continue
		}
		detach_ptr := dl.sym(handle, 'Detach')
		if detach_ptr == voidptr(0) {
			println('HOST: ${filename} missing Detach function')
			dl.close(handle)
			continue
		}
		invoke_ptr := dl.sym(handle, 'Invoke')
		if invoke_ptr == voidptr(0) {
			println('HOST: ${filename} missing Invoke function')
			dl.close(handle)
			continue
		}
		
		println('HOST: ${filename} has valid plugin interface')
		
		// Check for Report function
		report_ptr := dl.sym(handle, 'Report')
		if report_ptr == voidptr(0) {
			println('HOST: ${filename} missing Report function')
			dl.close(handle)
			continue
		}
		
		// Call Report to check if it's control plugin
		mut err_buf := [256]u8{}
		
		// Allocate info struct on stack using C
		info_ptr := unsafe { C.malloc(int(sizeof(C.LibsInfo))) }
		defer { unsafe { C.free(info_ptr) } }
		
		report_fn := ReportFn(report_ptr)
		if !report_fn(&char(&err_buf[0]), 256, &C.LibsInfo(info_ptr)) {
			err_str := unsafe { cstring_to_vstring(&char(&err_buf[0])) }
			println('HOST: ${filename} Report failed: ${err_str}')
			dl.close(handle)
			continue
		}
		
		// Check if it's the control plugin
		info := &C.LibsInfo(info_ptr)
		if unsafe { info.plugin_type != 0 } {
			plugin_type := unsafe { cstring_to_vstring(info.plugin_type) }
			
			if plugin_type == 'control' {
				println('HOST: ${filename} identified as control plugin (type=${plugin_type}, id=0x${info.plugin_id:x})')
				return ControlFns{
					handle:     handle
					attach_ptr: attach_ptr
					detach_ptr: detach_ptr
					invoke_ptr: invoke_ptr
				}
			}
			
			println('HOST: ${filename} is not control plugin (type=${plugin_type})')
		}
		
		dl.close(handle)
	}
	
	eprintln('Error: No control plugin found in ${exe_dir}')
	return none
}

// Step 2: Control Bind - Validate plugin functions
fn control_bind(fns ControlFns) bool {
	if fns.attach_ptr == unsafe { nil } || fns.detach_ptr == unsafe { nil } || fns.invoke_ptr == unsafe { nil } {
		eprintln('Error: Control plugin missing required functions')
		return false
	}
	
	println('Resolved control plugin functions (Attach/Detach/Invoke)')
	return true
}

// Step 3: Control Attach - Pass control's own Invoke as dispatch
fn control_attach(fns ControlFns) bool {
	mut err_buf := [256]u8{}
	
	attach_fn := AttachFn(fns.attach_ptr)
	invoke_fn := InvokeFn(fns.invoke_ptr)
	
	if !attach_fn(invoke_fn, &char(&err_buf[0]), 256) {
		err_str := unsafe { cstring_to_vstring(&char(&err_buf[0])) }
		eprintln('Error: Control plugin Attach failed: ${err_str}')
		return false
	}
	
	println('Control plugin attached successfully')
	return true
}

// Step 4: Control Invoke - Execute plugin functionality
fn control_invoke(fns ControlFns) {
	invoke_fn := InvokeFn(fns.invoke_ptr)
	
	addr := c'control.run'
	payload := c'{}'
	opts := c'{}'
	
	result := invoke_fn(addr, payload, opts)
	result_str := unsafe { cstring_to_vstring(result) }
	println('Control plugin result: ${result_str}')
}

// Step 5: Control Detach - Cleanup
fn control_detach(fns ControlFns) {
	mut err_buf := [256]u8{}
	
	detach_fn := DetachFn(fns.detach_ptr)
	_ := detach_fn(&char(&err_buf[0]), 256)
	
	dl.close(fns.handle)
}

fn main() {
	println('=== V HOST ===')
	
	fns := control_boot() or {
		exit(1)
	}
	
	if !control_bind(fns) {
		exit(1)
	}
	
	if !control_attach(fns) {
		exit(1)
	}
	
	control_invoke(fns)
	control_detach(fns)
	
	println('=== V HOST COMPLETE ===')
}
