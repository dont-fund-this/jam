#include "handler.h"
#include "llm_types.h"
#include <map>

// Global context registry
std::map<std::string, LLMContext*> g_llm_contexts;

// Forward declarations
handler_def llm_config_with();
handler_def llm_load_with();
handler_def llm_query_with();

handler_list llm_with() {
    return {
        llm_config_with(),
        llm_load_with(),
        llm_query_with(),
    };
}
