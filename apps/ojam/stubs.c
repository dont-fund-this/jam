/* C stubs for OCaml FFI to dynamic library loading */
#include <dlfcn.h>
#include <string.h>
#include <stdint.h>
#include <caml/mlvalues.h>
#include <caml/memory.h>
#include <caml/alloc.h>
#include <caml/fail.h>

/* LibsInfo struct matching plugin contract */
typedef struct {
    const char* plugin_type;
    const char* product;
    const char* description_long;
    const char* description_short;
    uint64_t plugin_id;
} LibsInfo;

/* Function pointer types */
typedef const char* (*InvokeFn)(const char*, const char*, const char*);
typedef bool (*AttachFn)(InvokeFn, char*, size_t);
typedef bool (*DetachFn)(char*, size_t);
typedef bool (*ReportFn)(char*, size_t, LibsInfo*);

/* dlopen wrapper (custom names to avoid conflicts) */
CAMLprim value caml_dlopen_custom(value path, value flags) {
    CAMLparam2(path, flags);
    void* handle = dlopen(String_val(path), Int_val(flags));
    CAMLreturn(caml_copy_nativeint((intnat)handle));
}

/* dlsym wrapper */
CAMLprim value caml_dlsym_custom(value handle, value symbol) {
    CAMLparam2(handle, symbol);
    void* handle_ptr = (void*)Nativeint_val(handle);
    if (!handle_ptr) {
        CAMLreturn(caml_copy_nativeint(0));
    }
    dlerror(); // Clear any existing error
    void* ptr = dlsym(handle_ptr, String_val(symbol));
    CAMLreturn(caml_copy_nativeint((intnat)ptr));
}

/* dlclose wrapper */
CAMLprim value caml_dlclose_custom(value handle) {
    CAMLparam1(handle);
    dlclose((void*)Nativeint_val(handle));
    CAMLreturn(Val_unit);
}

/* Attach function wrapper */
CAMLprim value caml_call_attach(value fn_ptr, value invoke_ptr, value err_buf, value buf_size) {
    CAMLparam4(fn_ptr, invoke_ptr, err_buf, buf_size);
    
    AttachFn attach = (AttachFn)Nativeint_val(fn_ptr);
    InvokeFn invoke = (InvokeFn)Nativeint_val(invoke_ptr);
    
    /* Pass the control plugin's own Invoke function as dispatch */
    bool result = attach(invoke, (char*)Bytes_val(err_buf), Int_val(buf_size));
    
    CAMLreturn(Val_bool(result));
}

/* Detach function wrapper */
CAMLprim value caml_call_detach(value fn_ptr, value err_buf, value buf_size) {
    CAMLparam3(fn_ptr, err_buf, buf_size);
    
    DetachFn detach = (DetachFn)Nativeint_val(fn_ptr);
    bool result = detach((char*)Bytes_val(err_buf), Int_val(buf_size));
    
    CAMLreturn(Val_bool(result));
}

/* Invoke function wrapper */
CAMLprim value caml_call_invoke(value fn_ptr, value addr, value args, value meta) {
    CAMLparam4(fn_ptr, addr, args, meta);
    CAMLlocal1(result);
    
    InvokeFn invoke = (InvokeFn)Nativeint_val(fn_ptr);
    const char* ret = invoke(String_val(addr), String_val(args), String_val(meta));
    
    result = caml_copy_string(ret ? ret : "");
    CAMLreturn(result);
}

/* Report function wrapper */
CAMLprim value caml_call_report(value fn_ptr, value err_buf, value buf_size) {
    CAMLparam3(fn_ptr, err_buf, buf_size);
    CAMLlocal2(result, record);
    
    ReportFn report = (ReportFn)Nativeint_val(fn_ptr);
    LibsInfo info;
    memset(&info, 0, sizeof(info));
    
    bool success = report((char*)Bytes_val(err_buf), Int_val(buf_size), &info);
    
    if (!success) {
        result = Val_int(0);  /* None */
    } else {
        /* Build Some(record) */
        record = caml_alloc(5, 0);  /* 5-field record */
        Store_field(record, 0, caml_copy_string(info.plugin_type ? info.plugin_type : ""));
        Store_field(record, 1, caml_copy_string(info.product ? info.product : ""));
        Store_field(record, 2, caml_copy_string(info.description_long ? info.description_long : ""));
        Store_field(record, 3, caml_copy_string(info.description_short ? info.description_short : ""));
        Store_field(record, 4, caml_copy_int64(info.plugin_id));
        
        result = caml_alloc(1, 0);  /* Some constructor */
        Store_field(result, 0, record);
    }
    
    CAMLreturn(result);
}
