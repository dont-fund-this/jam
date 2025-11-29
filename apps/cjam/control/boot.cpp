#include "boot.h"
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

#if defined(__APPLE__)
    #include <dlfcn.h>
    #define LIB_EXT ".dylib"
    #define LIB_LOAD(path) dlopen(path, RTLD_LAZY)
    #define LIB_SYM(handle, name) dlsym(handle, name)
    #define LIB_ERROR() dlerror()
    #define LIB_CLOSE(handle) dlclose(handle)
#elif defined(__linux__)
    #include <dlfcn.h>
    #define LIB_EXT ".so"
    #define LIB_LOAD(path) dlopen(path, RTLD_LAZY)
    #define LIB_SYM(handle, name) dlsym(handle, name)
    #define LIB_ERROR() dlerror()
    #define LIB_CLOSE(handle) dlclose(handle)
#elif defined(_WIN32)
    #include <windows.h>
    #define LIB_EXT ".dll"
    #define LIB_LOAD(path) LoadLibraryA(path)
    #define LIB_SYM(handle, name) GetProcAddress((HMODULE)handle, name)
    #define LIB_ERROR() "Windows LoadLibrary failed"
    #define LIB_CLOSE(handle) FreeLibrary((HMODULE)handle)
#endif

using DispatchFn = const char* (*)(const char* address, const char* payload, const char* options);
using AttachFn = bool (*)(DispatchFn dispatch, char* err_buf, std::size_t err_cap);
using DetachFn = bool (*)(char* err_buf, std::size_t err_cap);
using InvokeFn = const char* (*)(const char* address, const char* payload, const char* options);

struct libsinfo {
    const char* plugin_type;
    const char* product;
    const char* description_long;
    const char* description_short;
    unsigned long plugin_id;
};

using ReportFn = bool (*)(char* err_buf, std::size_t err_cap, libsinfo* out);

void* control_boot(char** argv) {
    std::filesystem::path exe_path = std::filesystem::canonical(argv[0]);
    std::filesystem::path exe_dir = exe_path.parent_path();

    std::cout << "HOST: Discovering control plugin..." << std::endl;
    std::cout << "HOST: Scanning directory: " << exe_dir << std::endl;
    
    for (const auto& entry : std::filesystem::directory_iterator(exe_dir)) {
        if (!entry.is_regular_file()) continue;
        
        std::string filename = entry.path().filename().string();
        std::string extension = entry.path().extension().string();
        
        // Check if it's a dylib/so/dll
        if (extension != LIB_EXT) continue;
        
        // Only consider libs starting with "lib"
        if (filename.find("lib") != 0) continue;
        
        std::cout << "HOST: Found candidate: " << filename << std::endl;
        
        // Try to load it
        void* handle = LIB_LOAD(entry.path().string().c_str());
        if (!handle) {
            std::cout << "HOST: Failed to load " << filename << ": " << LIB_ERROR() << std::endl;
            continue;
        }
        
        // Check for required functions
        AttachFn attach = (AttachFn)LIB_SYM(handle, "Attach");
        DetachFn detach = (DetachFn)LIB_SYM(handle, "Detach");
        InvokeFn invoke = (InvokeFn)LIB_SYM(handle, "Invoke");
        ReportFn report = (ReportFn)LIB_SYM(handle, "Report");
        
        if (!attach || !detach || !invoke) {
            std::cout << "HOST: " << filename << " missing required functions (Attach/Detach/Invoke)" << std::endl;
            LIB_CLOSE(handle);
            continue;
        }
        
        std::cout << "HOST: " << filename << " has valid plugin interface" << std::endl;
        
        // Call Report to check if it's control plugin
        if (!report) {
            std::cout << "HOST: " << filename << " missing Report function" << std::endl;
            LIB_CLOSE(handle);
            continue;
        }
        
        libsinfo desc = {};
        char err_buf[256] = {0};
        if (!report(err_buf, sizeof(err_buf), &desc)) {
            std::cout << "HOST: " << filename << " Report failed: " << err_buf << std::endl;
            LIB_CLOSE(handle);
            continue;
        }
        
        // Check if it's the control plugin
        if (desc.plugin_type && strcmp(desc.plugin_type, "control") == 0) {
            std::cout << "HOST: " << filename << " identified as control plugin"
                      << " (type=" << desc.plugin_type
                      << ", id=0x" << std::hex << desc.plugin_id << std::dec << ")" << std::endl;
            return handle;
        }
        
        std::cout << "HOST: " << filename << " is not control plugin (type=" << desc.plugin_type << ")" << std::endl;
        LIB_CLOSE(handle);
    }
    
    std::cerr << "Error: No control plugin found in " << exe_dir << std::endl;
    return nullptr;
}
