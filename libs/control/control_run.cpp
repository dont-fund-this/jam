#include "handler.h"
#include "registry.h"
#include "contract.h"
#include <iostream>

extern DispatchFn g_dispatch;

handler_def control_run_with() {
    return {
        .sid = "control.run",
        .tag = "bootstrap",
        .fun = [](const char* /* payload */, const char* /* options */, std::string& /* err */) -> std::any {
            std::cout << "CONTROL: Running main application logic" << std::endl;
            
            // Load all plugins
            if (!control_discover_and_load(g_dispatch)) {
                std::cout << "CONTROL: No plugins discovered" << std::endl;
                return std::string(R"({"success":true,"message":"no plugins found"})");
            }
            
            std::vector<LoadedPlugin>& plugins = control_get_registry();
            std::cout << "CONTROL: Successfully loaded " << plugins.size() << " plugins" << std::endl;
            
            // Print plugin list
            std::cout << "\n=== DISCOVERED PLUGINS ===" << std::endl;
            for (size_t i = 0; i < plugins.size(); ++i) {
                auto& plugin = plugins[i];
                std::cout << (i + 1) << ". " << plugin.name;
                
                if (plugin.report) {
                    libsinfo desc = {};
                    char err_buf[256] = {0};
                    if (plugin.report(err_buf, sizeof(err_buf), &desc)) {
                        std::cout << " - " << desc.description_short
                                  << " (type=" << desc.plugin_type << ")";
                    }
                }
                std::cout << std::endl;
            }
            std::cout << "==========================\n" << std::endl;
            
            return std::string(R"({"success":true,"plugins":)" + std::to_string(plugins.size()) + "}");
        }
    };
}
