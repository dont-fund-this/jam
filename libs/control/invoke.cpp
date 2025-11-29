#include "contract.h"
#include "handler.h"
#include "registry.h"
#include <iostream>
#include <string>

extern DispatchFn g_dispatch;

handler_list control_with();

extern "C" const char* Invoke(const char* address, 
                              const char* payload, 
                              const char* options) {

    std::cout << "CONTROL: Invoke called" << std::endl;
    std::cout << "CONTROL: address='" << (address ? address : "null") << "'" << std::endl;
    std::cout << "CONTROL: payload='" << (payload ? payload : "null") << "'" << std::endl;
    std::cout << "CONTROL: pptions='" << (options ? options : "null") << "'" << std::endl;
    
    handler_list handlers = control_with();
    
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
    
    // Broadcast to all loaded plugins - let them decide if they handle it
    std::vector<LoadedPlugin>& plugins = control_get_registry();
    
    for (auto& plugin : plugins) {
        const char* result = plugin.invoke(address, payload, options);
        
        // Check if plugin handled it (any non-null response)
        if (result != nullptr) {
            std::cout << "CONTROL: Routed to " << plugin.name << std::endl;
            return result;
        }
    }
    
    // No plugin handled it
    std::cout << "CONTROL: No plugin handled address '" << address << "'" << std::endl;
    return R"({"success":false,"error":"no plugin handled address"})";
}
