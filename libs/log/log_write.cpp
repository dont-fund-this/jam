#include "handler.h"
#include <iostream>

handler_def log_write_with() {
    return {
        .sid = "log.write",
        .tag = "logging",
        .fun = [](const char* payload, const char* /* options */, std::string& /* err */) -> std::any {
            std::cout << "LOG: " << payload << std::endl;
            return std::string(R"({"success":true,"logged":true})");
        }
    };
}
