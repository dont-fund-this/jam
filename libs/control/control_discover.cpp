#include "handler.h"
#include "registry.h"
#include <iostream>

extern DispatchFn g_dispatch;

handler_def control_discover_with() {
    return {
        .sid = "control.discover",
        .tag = "discovery",
        .fun = [](const char* /* payload */, const char* /* options */, std::string& /* err */) -> std::any {
            std::cout << "CONTROL: Discovering plugins" << std::endl;
            control_discover_and_load(g_dispatch);
            std::vector<LoadedPlugin>& plugins = control_get_registry();
            return std::string(R"({"success":true,"discovered":)" + std::to_string(plugins.size()) + "}");
        }
    };
}
