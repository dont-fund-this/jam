#include "contract.h"
#include "handler.h"
#include <iostream>

handler_list log_with();

extern "C" const char* Invoke(const char* address, 
                              const char* payload, 
                              const char* options) {

    std::cout << "LOG: Invoke called" << std::endl;
    std::cout << "LOG: address='" << (address ? address : "null") << "'" << std::endl;
    std::cout << "LOG: payload='" << (payload ? payload : "null") << "'" << std::endl;
    std::cout << "LOG: pptions='" << (options ? options : "null") << "'" << std::endl;
    
    handler_list handlers = log_with();
    
    for (const auto& handler : handlers) {
        if (handler.sid == address) {
            std::string err;
            try {
                std::any result = handler.fun(payload, options, err);
                static std::string response;
                response = std::any_cast<std::string>(result);
                return response.c_str();
            } catch (const std::exception& ex) {
                static std::string response = R"({"success":false,"error":")" + std::string(ex.what()) + R"("})";
                return response.c_str();
            }
        }
    }
    
    return R"({"success":false,"error":"handler not found"})";
}
