#include "registry.h"
#include <iostream>
#include <filesystem>

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

// Global plugin registry
static std::vector<LoadedPlugin> g_plugin_registry;

std::vector<LoadedPlugin>& control_get_registry() {
    return g_plugin_registry;
}

void control_cleanup_registry() {
    if (g_plugin_registry.empty()) return;
    
    std::cout << "CONTROL: Cleaning up " << g_plugin_registry.size() << " plugins..." << std::endl;
    for (auto& plugin : g_plugin_registry) {
        std::cout << "CONTROL: Detaching " << plugin.name << std::endl;
        char err_buf[256] = {0};
        if (plugin.detach && !plugin.detach(err_buf, sizeof(err_buf))) {
            std::cout << "CONTROL: " << plugin.name << " Detach failed: " << err_buf << std::endl;
        }
        if (plugin.handle) {
            LIB_CLOSE(plugin.handle);
        }
    }
    g_plugin_registry.clear();
}

bool control_discover_and_load(DispatchFn dispatch) {
    std::cout << "CONTROL: Discovering plugins..." << std::endl;
    
    std::filesystem::path exe_dir = std::filesystem::current_path();
    std::cout << "CONTROL: Scanning directory: " << exe_dir << std::endl;
    
    std::vector<std::filesystem::path> lib_paths;
    
    for (const auto& entry : std::filesystem::directory_iterator(exe_dir)) {
        if (!entry.is_regular_file()) continue;
        
        std::string filename = entry.path().filename().string();
        std::string extension = entry.path().extension().string();
        
        if (extension != LIB_EXT) continue;
        if (filename.find("libcontrol") == 0) {
            std::cout << "CONTROL: Skipping self: " << filename << std::endl;
            continue;
        }
        if (filename.find("lib") != 0) continue;
        
        lib_paths.push_back(entry.path());
        std::cout << "CONTROL: Found candidate: " << filename << std::endl;
    }
    
    std::cout << "CONTROL: Found " << lib_paths.size() << " plugin candidates" << std::endl;
    
    for (const auto& lib_path : lib_paths) {
        std::string filename = lib_path.filename().string();
        std::cout << "CONTROL: Loading " << filename << "..." << std::endl;
        
        void* handle = LIB_LOAD(lib_path.string().c_str());
        if (!handle) {
            std::cout << "CONTROL: Failed to load " << filename << ": " << LIB_ERROR() << std::endl;
            continue;
        }
        
        AttachFn attach = (AttachFn)LIB_SYM(handle, "Attach");
        DetachFn detach = (DetachFn)LIB_SYM(handle, "Detach");
        InvokeFn invoke = (InvokeFn)LIB_SYM(handle, "Invoke");
        ReportFn report = (ReportFn)LIB_SYM(handle, "Report");
        
        if (!attach || !detach || !invoke || !report) {
            std::cout << "CONTROL: " << filename << " missing required functions" << std::endl;
            LIB_CLOSE(handle);
            continue;
        }
        
        std::cout << "CONTROL: " << filename << " has valid plugin interface" << std::endl;
        
        char err_buf[256] = {0};
        if (!attach(dispatch, err_buf, sizeof(err_buf))) {
            std::cout << "CONTROL: " << filename << " Attach failed: " << err_buf << std::endl;
            LIB_CLOSE(handle);
            continue;
        }
        
        std::cout << "CONTROL: " << filename << " attached successfully" << std::endl;
        
        LoadedPlugin plugin;
        plugin.name = filename;
        plugin.handle = handle;
        plugin.attach = attach;
        plugin.detach = detach;
        plugin.invoke = invoke;
        plugin.report = report;
        
        g_plugin_registry.push_back(plugin);
    }
    
    return !g_plugin_registry.empty();
}
