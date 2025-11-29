#include "handler.h"
#include <iostream>

std::any list_handle(const char* payload, const char* options, std::string& /* err */) {
    std::cout << "BAG: list_handle called" << std::endl;
    std::cout << "BAG:   payload='" << (payload ? payload : "null") << "'" << std::endl;
    std::cout << "BAG:   options='" << (options ? options : "null") << "'" << std::endl;
    
    return std::string(R"({"success":true,"message":"listed handlers"})");
}

handler_def list_with() {
    return {"bag.list", "registry", list_handle};
}
