#include "handler.h"
#include <iostream>

handler_def control_load_with() {
    return {
        .sid = "control.load",
        .tag = "loading",
        .fun = [](const char* /* payload */, const char* /* options */, std::string& /* err */) -> std::any {
            std::cout << "CONTROL: Load specific plugin (not yet implemented)" << std::endl;
            return std::string(R"({"success":true,"message":"load not yet implemented"})");
        }
    };
}
