#include "contract.h"
#include "handler.h"
#include <iostream>

handler_list sql_with();

extern "C" const char* Invoke(const char* address, 
                              const char* payload, 
                              const char* options) {

    std::cout << "SQL: Invoke called" << std::endl;
    std::cout << "SQL: address='" << (address ? address : "null") << "'" << std::endl;
    std::cout << "SQL: payload='" << (payload ? payload : "null") << "'" << std::endl;
    std::cout << "SQL: pptions='" << (options ? options : "null") << "'" << std::endl;
    
    handler_list handlers = sql_with();
    
    for (const auto& handler : handlers) {
        if (handler.sid == address) {
            std::string err;
            try {
                std::any result = handler.fun(payload, options, err);
                static std::string response;
                response = std::any_cast<std::string>(result);
                return response.c_str();
            } catch (const std::exception& ex) {
                static std::string response = R"({"success":false,"error":"})" + std::string(ex.what()) + R"("})";
                return response.c_str();
            }
        }
    }
    
    return R"({"success":false,"error":"handler not found"})";
}
