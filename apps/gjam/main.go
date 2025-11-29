package main

/*
#cgo LDFLAGS: -ldl
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>

typedef const char* (*InvokeFn)(const char*, const char*, const char*);
typedef int (*AttachFn)(InvokeFn, char*, size_t);
typedef int (*DetachFn)(char*, size_t);

typedef struct {
    const char* plugin_type;
    const char* product;
    const char* description_long;
    const char* description_short;
    unsigned long plugin_id;
} libsinfo;

typedef int (*ReportFn)(char*, size_t, libsinfo*);

static InvokeFn call_attach(void* attach_ptr, InvokeFn invoke_fn, char* err_buf, size_t err_cap) {
    AttachFn attach = (AttachFn)attach_ptr;
    if (attach(invoke_fn, err_buf, err_cap)) {
        return invoke_fn;
    }
    return NULL;
}

static void call_detach(void* detach_ptr, char* err_buf, size_t err_cap) {
    DetachFn detach = (DetachFn)detach_ptr;
    detach(err_buf, err_cap);
}

static const char* call_invoke(void* invoke_ptr, const char* addr, const char* payload, const char* opts) {
    InvokeFn invoke = (InvokeFn)invoke_ptr;
    return invoke(addr, payload, opts);
}

static int call_report(void* report_ptr, char* err_buf, size_t err_cap, libsinfo* info) {
    ReportFn report = (ReportFn)report_ptr;
    return report(err_buf, err_cap, info);
}
*/
import "C"
import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"unsafe"
)

type ControlFns struct {
	handle    unsafe.Pointer
	attachPtr unsafe.Pointer
	detachPtr unsafe.Pointer
	invokePtr unsafe.Pointer
}

func controlBoot() *ControlFns {
	fmt.Println("HOST: Discovering control plugin...")

	exe, err := os.Executable()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error getting executable path: %v\n", err)
		return nil
	}

	exeDir, err := filepath.EvalSymlinks(filepath.Dir(exe))
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error resolving executable directory: %v\n", err)
		return nil
	}

	fmt.Printf("HOST: Scanning directory: %s\n", exeDir)

	entries, err := os.ReadDir(exeDir)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error reading directory: %v\n", err)
		return nil
	}

	libExt := ".dylib" // macOS
	if strings.Contains(strings.ToLower(os.Getenv("OS")), "windows") {
		libExt = ".dll"
	} else {
		if _, err := os.Stat("/etc/os-release"); err == nil {
			libExt = ".so" // Linux
		}
	}

	for _, entry := range entries {
		if entry.IsDir() {
			continue
		}

		filename := entry.Name()

		if !strings.HasSuffix(filename, libExt) {
			continue
		}

		if !strings.HasPrefix(filename, "lib") {
			continue
		}

		fmt.Printf("HOST: Found candidate: %s\n", filename)

		libPath := filepath.Join(exeDir, filename)
		cPath := C.CString(libPath)
		handle := C.dlopen(cPath, C.RTLD_LAZY)
		C.free(unsafe.Pointer(cPath))

		if handle == nil {
			cerr := C.dlerror()
			fmt.Printf("HOST: Failed to load %s: %s\n", filename, C.GoString(cerr))
			continue
		}

		// Check for required functions
		attachName := C.CString("Attach")
		attachPtr := C.dlsym(handle, attachName)
		C.free(unsafe.Pointer(attachName))

		detachName := C.CString("Detach")
		detachPtr := C.dlsym(handle, detachName)
		C.free(unsafe.Pointer(detachName))

		invokeName := C.CString("Invoke")
		invokePtr := C.dlsym(handle, invokeName)
		C.free(unsafe.Pointer(invokeName))

		reportName := C.CString("Report")
		reportPtr := C.dlsym(handle, reportName)
		C.free(unsafe.Pointer(reportName))

		if attachPtr == nil || detachPtr == nil || invokePtr == nil {
			fmt.Printf("HOST: %s missing required functions (Attach/Detach/Invoke)\n", filename)
			C.dlclose(handle)
			continue
		}

		fmt.Printf("HOST: %s has valid plugin interface\n", filename)

		if reportPtr == nil {
			fmt.Printf("HOST: %s missing Report function\n", filename)
			C.dlclose(handle)
			continue
		}

		// Call Report to check plugin type
		var info C.libsinfo
		errBuf := make([]byte, 256)
		result := C.call_report(reportPtr, (*C.char)(unsafe.Pointer(&errBuf[0])), C.size_t(len(errBuf)), &info)

		if result == 0 {
			fmt.Printf("HOST: %s Report failed: %s\n", filename, string(errBuf))
			C.dlclose(handle)
			continue
		}

		pluginType := C.GoString(info.plugin_type)

		if pluginType == "control" {
			fmt.Printf("HOST: %s identified as control plugin (type=%s, id=0x%x)\n",
				filename, pluginType, uint(info.plugin_id))
			return &ControlFns{
				handle:    handle,
				attachPtr: attachPtr,
				detachPtr: detachPtr,
				invokePtr: invokePtr,
			}
		}

		fmt.Printf("HOST: %s is not control plugin (type=%s)\n", filename, pluginType)
		C.dlclose(handle)
	}

	fmt.Fprintf(os.Stderr, "Error: No control plugin found in %s\n", exeDir)
	return nil
}

func controlBind(fns *ControlFns) bool {
	if fns.attachPtr == nil || fns.detachPtr == nil || fns.invokePtr == nil {
		fmt.Fprintf(os.Stderr, "Error: Control plugin missing required functions\n")
		return false
	}
	fmt.Println("Resolved control plugin functions (Attach/Detach/Invoke)")
	return true
}

func controlAttach(fns *ControlFns) bool {
	fmt.Println("Resolved control plugin functions (Attach/Detach/Invoke)")

	errBuf := make([]byte, 256)
	invokeFn := C.call_attach(fns.attachPtr, C.InvokeFn(fns.invokePtr), (*C.char)(unsafe.Pointer(&errBuf[0])), C.size_t(len(errBuf)))
	if invokeFn == nil {
		fmt.Fprintf(os.Stderr, "Error: Control plugin Attach failed: %s\n", string(errBuf))
		return false
	}
	fmt.Println("Control plugin attached successfully")
	return true
}

func controlInvoke(fns *ControlFns) {
	addr := C.CString("control.run")
	defer C.free(unsafe.Pointer(addr))
	emptyPayload := C.CString("{}")
	defer C.free(unsafe.Pointer(emptyPayload))
	emptyOptions := C.CString("{}")
	defer C.free(unsafe.Pointer(emptyOptions))

	response := C.call_invoke(fns.invokePtr, addr, emptyPayload, emptyOptions)
	fmt.Printf("Control plugin result: %s\n", C.GoString(response))
}

func controlDetach(fns *ControlFns) {
	errBuf := make([]byte, 256)
	C.call_detach(fns.detachPtr, (*C.char)(unsafe.Pointer(&errBuf[0])), C.size_t(len(errBuf)))
	if fns.handle != nil {
		C.dlclose(fns.handle)
	}
}

func main() {
	fmt.Println("=== GO HOST ===")

	fns := controlBoot()
	if fns == nil {
		os.Exit(1)
	}

	if !controlBind(fns) {
		os.Exit(1)
	}

	if !controlAttach(fns) {
		os.Exit(1)
	}

	controlInvoke(fns)
	controlDetach(fns)

	fmt.Println("=== GO HOST COMPLETE ===")
}
