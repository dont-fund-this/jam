#include "handler.h"
#include "registry.h"
#include <iostream>

handler_def control_list_with() {
    return {
        .sid = "control.list",
        .tag = "introspection",
        .fun = [](const char* /* payload */, const char* /* options */, std::string& /* err */) -> std::any {
            std::cout << "CONTROL: Listing loaded plugins" << std::endl;
            
            std::vector<LoadedPlugin>& plugins = control_get_registry();
            std::string result = R"({"success":true,"plugins":[)";
            
            for (size_t i = 0; i < plugins.size(); ++i) {
                if (i > 0) result += ",";
                result += R"({"name":")" + plugins[i].name + R"("})";
            }
            
            result += "]}";
            return result;
        }
    };
}
