#include "handler.h"
#include "llm_types.h"
#include "contract.h"
#include <iostream>
#include <sstream>
#include <cstring>

extern DispatchFn g_dispatch;
extern std::map<std::string, LLMContext*> g_llm_contexts;

// Forward declaration from llm_config.cpp
ModelConfig parse_model_config(const std::string& json, const std::string& model_name);

static int g_context_counter = 0;

handler_def llm_load_with() {
    return {
        .sid = "llm.load",
        .tag = "llm",
        .fun = [](const char* payload, const char* /* options */, std::string& err) -> std::any {
            try {
                std::string model_name = payload ? payload : "default";
                
                std::cout << "LLM: Loading model '" << model_name << "'..." << std::endl;
                
                // Get config via EFS
                // Read config via EFS to get model path
                const char* json_data = g_dispatch("efs.read", "json/llm.json", nullptr);
                if (!json_data) {
                    err = "Failed to read config";
                    return std::string(R"({"success":false,"error":"config not found"})");
                }
                
                ModelConfig cfg = parse_model_config(json_data, model_name);
                
                // Load dynamic backends
                ggml_backend_load_all();
                
                // Load model from file
                llama_model_params model_params = llama_model_default_params();
                model_params.n_gpu_layers = cfg.gpu_layers;
                
                llama_model* model = llama_model_load_from_file(cfg.model_path.c_str(), model_params);
                if (!model) {
                    err = "Failed to load model from " + cfg.model_path;
                    return std::string(R"({"success":false,"error":"model load failed"})");
                }
                
                // Create context
                llama_context_params ctx_params = llama_context_default_params();
                ctx_params.n_ctx = cfg.context_size;
                ctx_params.n_batch = cfg.batch_size;
                ctx_params.n_threads = cfg.threads;
                
                llama_context* ctx = llama_init_from_model(model, ctx_params);
                if (!ctx) {
                    llama_model_free(model);
                    err = "Failed to create context";
                    return std::string(R"({"success":false,"error":"context init failed"})");
                }
                
                // Initialize vision context if mmproj provided
                mtmd_context* mtmd_ctx = nullptr;
                if (!cfg.mmproj_path.empty()) {
                    mtmd_context_params mparams = mtmd_context_params_default();
                    mparams.use_gpu = true;
                    mparams.n_threads = cfg.threads;
                    
                    mtmd_ctx = mtmd_init_from_file(cfg.mmproj_path.c_str(), model, mparams);
                    if (!mtmd_ctx) {
                        std::cout << "LLM: Warning - failed to load vision model" << std::endl;
                    } else {
                        std::cout << "LLM: Vision support enabled" << std::endl;
                    }
                }
                
                // Create LLMContext
                auto* llm_ctx = new LLMContext();
                llm_ctx->model = new LlamaModel{model, ctx, mtmd_ctx};
                llm_ctx->config = cfg;
                llm_ctx->context_id = "llm_ctx_" + std::to_string(++g_context_counter);
                
                // Store in registry
                g_llm_contexts[llm_ctx->context_id] = llm_ctx;
                
                std::cout << "LLM: Loaded successfully (" << llm_ctx->context_id << ")" << std::endl;
                
                return std::string(R"({"success":true,"context_id":")" + llm_ctx->context_id + "\"}");
                
            } catch (const std::exception& ex) {
                err = ex.what();
                return std::string(R"({"success":false,"error":")" + std::string(ex.what()) + R"("})");
            }
        }
    };
}
