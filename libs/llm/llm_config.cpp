#include "handler.h"
#include "llm_types.h"
#include "contract.h"
#include <iostream>
#include <fstream>
#include <sstream>

extern DispatchFn g_dispatch;

// Simple JSON parser for config (subset needed for ModelConfig)
ModelConfig parse_model_config(const std::string& json, const std::string& model_name) {
    ModelConfig cfg;
    
    // Find the model section
    size_t models_pos = json.find("\"" + model_name + "\"");
    if (models_pos == std::string::npos) {
        throw std::runtime_error("Model '" + model_name + "' not found in config");
    }
    
    // Extract values (simple string search - good enough for known format)
    auto extract_string = [&](const std::string& key) -> std::string {
        std::string search = "\"" + key + "\":";
        size_t pos = json.find(search, models_pos);
        if (pos == std::string::npos) return "";
        pos = json.find("\"", pos + search.length());
        size_t end = json.find("\"", pos + 1);
        return json.substr(pos + 1, end - pos - 1);
    };
    
    auto extract_int = [&](const std::string& key, int def = 0) -> int {
        std::string search = "\"" + key + "\":";
        size_t pos = json.find(search, models_pos);
        if (pos == std::string::npos) return def;
        pos += search.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        try {
            return std::stoi(json.substr(pos));
        } catch (...) {
            // Handle overflow - return default for values like 4294967295
            return def;
        }
    };
    
    auto extract_float = [&](const std::string& key, float def = 0.0f) -> float {
        std::string search = "\"" + key + "\":";
        size_t pos = json.find(search, models_pos);
        if (pos == std::string::npos) return def;
        pos += search.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        return std::stof(json.substr(pos));
    };
    
    cfg.model_path = extract_string("model_path");
    cfg.mmproj_path = extract_string("mmproj_path");
    cfg.context_size = extract_int("context_size", 2048);
    cfg.batch_size = extract_int("batch_size", 2048);
    cfg.threads = extract_int("threads", 4);
    cfg.gpu_layers = extract_int("gpu_layers", 99);
    cfg.max_tokens = extract_int("max_tokens", 1024);
    cfg.system_prompt = extract_string("system_prompt");
    cfg.temperature = extract_float("temperature", 0.8f);
    cfg.min_p = extract_float("min_p", 0.05f);
    cfg.repeat_penalty = extract_float("repeat_penalty", 1.1f);
    cfg.repeat_last_n = extract_int("repeat_last_n", 64);
    cfg.seed = (uint32_t)extract_int("seed", -1);
    
    // Buffer configs with defaults
    cfg.query_buffers.chat_template_size = extract_int("chat_template_size", 8192);
    cfg.query_buffers.token_buffer_size = extract_int("token_buffer_size", 128);
    cfg.query_buffers.max_tokens_for_prompt = extract_int("max_tokens_for_prompt", 512);
    
    return cfg;
}

handler_def llm_config_with() {
    return {
        .sid = "llm.config",
        .tag = "llm",
        .fun = [](const char* payload, const char* /* options */, std::string& err) -> std::any {
            try {
                std::string model_name = payload ? payload : "default";
                
                // Read config via EFS
                const char* json_data = g_dispatch("efs.read", "json/llm.json", nullptr);
                if (!json_data || std::strlen(json_data) == 0) {
                    err = "Failed to read json/llm.json or file is empty";
                    return std::string(R"({"success":false,"error":"config not found"})");
                }
                
                std::cout << "LLM: Read " << std::strlen(json_data) << " bytes from json/llm.json" << std::endl;
                
                ModelConfig cfg = parse_model_config(json_data, model_name);
                
                // Return config as JSON
                std::ostringstream result;
                result << R"({"success":true,"model":")" << model_name << R"(",)";
                result << R"("model_path":")" << cfg.model_path << R"(",)";
                result << R"("context_size":)" << cfg.context_size << ",";
                result << R"("threads":)" << cfg.threads << ",";
                result << R"("gpu_layers":)" << cfg.gpu_layers << ",";
                result << R"("max_tokens":)" << cfg.max_tokens << "}";
                
                return result.str();
            } catch (const std::exception& ex) {
                err = ex.what();
                return std::string(R"({"success":false,"error":")" + std::string(ex.what()) + R"("})");
            }
        }
    };
}
