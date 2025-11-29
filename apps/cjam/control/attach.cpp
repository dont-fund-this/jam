#include "attach.h"
#include "bind.h"
#include <cstddef>
#include <iostream>

#if defined(__APPLE__)
    #include <dlfcn.h>
    #define LIB_CLOSE(handle) dlclose(handle)
#elif defined(__linux__)
    #include <dlfcn.h>
    #define LIB_CLOSE(handle) dlclose(handle)
#elif defined(_WIN32)
    #include <windows.h>
    #define LIB_CLOSE(handle) FreeLibrary((HMODULE)handle)
#endif

bool control_attach(void* /* handle */, const ControlFns& fns) {
    std::cout << "Resolved control plugin functions (Attach/Detach/Invoke)" << std::endl;

    // Pass control's own Invoke as the dispatch function
    // Plugins call dispatch(addr, payload, opts) → control routes to appropriate plugin
    char err_buf[256] = {0};
    if (!fns.attach(fns.invoke, err_buf, sizeof(err_buf))) {
        std::cerr << "Error: Control plugin Attach failed: " << err_buf << std::endl;
        return false;
    }
    
    std::cout << "Control plugin attached successfully" << std::endl;
    return true;
}
