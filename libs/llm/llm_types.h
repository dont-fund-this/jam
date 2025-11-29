#pragma once

#include <string>
#include <map>

extern "C" {
    #include <llama.h>
    #include <mtmd.h>
}

// Configuration structures matching jam-2's JSON format
struct QueryBuffers {
    int chat_template_size = 8192;
    int token_buffer_size = 128;
    int max_tokens_for_prompt = 512;
};

struct VisionBuffers {
    int chat_template_size = 8192;
    int token_buffer_size = 128;
};

struct ImageBuffers {
    int rgb_bytes_per_pixel = 3;
    int color_bit_shift = 8;
};

struct ModelConfig {
    std::string model_path;
    std::string mmproj_path;  // Optional: for vision models
    int context_size;
    int batch_size;
    int threads;
    int gpu_layers;
    int max_tokens;
    std::string system_prompt;
    float temperature;
    float min_p;
    float repeat_penalty;
    int repeat_last_n;
    uint32_t seed;
    
    QueryBuffers query_buffers;
    VisionBuffers vision_buffers;
    ImageBuffers image_buffers;
};

struct LlamaModel {
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    mtmd_context* mtmd_ctx = nullptr;
    
    ~LlamaModel() {
        if (mtmd_ctx) mtmd_free(mtmd_ctx);
        if (ctx) llama_free(ctx);
        if (model) llama_model_free(model);
    }
};

struct LLMContext {
    LlamaModel* model = nullptr;
    ModelConfig config;
    std::string context_id;
};

// Global registry of loaded contexts
extern std::map<std::string, LLMContext*> g_llm_contexts;
