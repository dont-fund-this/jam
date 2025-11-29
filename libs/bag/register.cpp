#include "handler.h"
#include <iostream>

std::any register_handle(const char* payload, const char* options, std::string& /* err */) {
    std::cout << "BAG: register_handle called" << std::endl;
    std::cout << "BAG:   payload='" << (payload ? payload : "null") << "'" << std::endl;
    std::cout << "BAG:   options='" << (options ? options : "null") << "'" << std::endl;
    
    return std::string(R"({"success":true,"message":"registered"})");
}

handler_def register_with() {
    return {"bag.register", "registry", register_handle};
}
