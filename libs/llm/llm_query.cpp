#include "handler.h"
#include "llm_types.h"
#include "contract.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>
#include <mtmd.h>
#include <mtmd-helper.h>
#include "stb_image.h"

extern std::map<std::string, LLMContext*> g_llm_contexts;
extern DispatchFn g_dispatch;

// Extract context_id from payload JSON
std::string extract_context_id_json(const char* payload) {
    if (!payload) return "";
    std::string p(payload);
    size_t pos = p.find("\"context_id\":");
    if (pos == std::string::npos) return "";
    pos = p.find("\"", pos + 13);
    size_t end = p.find("\"", pos + 1);
    return p.substr(pos + 1, end - pos - 1);
}

// Extract prompt from payload JSON
std::string extract_prompt(const char* payload) {
    if (!payload) return "";
    std::string p(payload);
    size_t pos = p.find("\"prompt\":");
    if (pos == std::string::npos) return "";
    pos = p.find("\"", pos + 9);
    size_t end = p.find("\"", pos + 1);
    return p.substr(pos + 1, end - pos - 1);
}



handler_def llm_query_with() {
    return {
        .sid = "llm.query",
        .tag = "llm",
        .fun = [](const char* payload, const char* /* options */, std::string& err) -> std::any {
            try {
                std::string context_id = extract_context_id_json(payload);
                std::string prompt = extract_prompt(payload);
                
                if (context_id.empty() || prompt.empty()) {
                    err = "Missing context_id or prompt";
                    return std::string(R"({"success":false,"error":"invalid payload"})");
                }
                
                // Get context
                auto it = g_llm_contexts.find(context_id);
                if (it == g_llm_contexts.end()) {
                    err = "Context not found: " + context_id;
                    return std::string(R"({"success":false,"error":"context not found"})");
                }
                
                LLMContext* ctx = it->second;
                const llama_vocab* vocab = llama_model_get_vocab(ctx->model->model);
                
                std::cout << "LLM: Generating response..." << std::endl;
       
                // Build chat messages (system + user)
                std::vector<llama_chat_message> messages(2);
                messages[0].role = "system";
                messages[0].content = ctx->config.system_prompt.c_str();
                messages[1].role = "user";
                messages[1].content = prompt.c_str();
                
                // Apply chat template
                const char* tmpl = llama_model_chat_template(ctx->model->model, nullptr);
                
                int buf_size = ctx->config.query_buffers.chat_template_size;
                std::vector<char> formatted(buf_size);
                
                int formatted_len = llama_chat_apply_template(
                    tmpl,
                    messages.data(),
                    messages.size(),
                    true,  // add_ass
                    formatted.data(),
                    buf_size
                );
                
                if (formatted_len < 0) {
                    err = "Failed to apply chat template";
                    return std::string(R"({"success":false,"error":"template failed"})");
                }
                
                if (formatted_len > buf_size) {
                    formatted.resize(formatted_len);
                    formatted_len = llama_chat_apply_template(
                        tmpl, messages.data(), messages.size(),
                        true, formatted.data(), formatted_len
                    );
                }
                
                std::string formatted_prompt(formatted.data(), formatted_len);
                
                llama_pos n_past = 0;
                (void)n_past; // Used in vision path
                
             
                    int max_tokens = ctx->config.query_buffers.max_tokens_for_prompt;
                    std::vector<llama_token> tokens(max_tokens);
                    
                    int n_tokens = llama_tokenize(
                        vocab, formatted_prompt.c_str(), formatted_prompt.length(),
                        tokens.data(), max_tokens, true, true
                    );
                    
                    if (n_tokens < 0) {
                        err = "Tokenization failed";
                        return std::string(R"({"success":false,"error":"tokenize failed"})");
                    }
                    
                    tokens.resize(n_tokens);
                    
                    // Decode prompt
                    llama_batch batch = llama_batch_get_one(tokens.data(), n_tokens);
                    if (llama_decode(ctx->model->ctx, batch) != 0) {
                        err = "Decode failed";
                        return std::string(R"({"success":false,"error":"decode failed"})");
                    }
                    
                    n_past = n_tokens;
            
                
                // Initialize sampler chain
                llama_sampler* smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
                
                // Add penalties
                llama_sampler_chain_add(smpl, llama_sampler_init_penalties(
                    ctx->config.repeat_last_n,
                    ctx->config.repeat_penalty,
                    0.0f, 0.0f
                ));
                
                // Add min-p
                llama_sampler_chain_add(smpl, llama_sampler_init_min_p(ctx->config.min_p, 1));
                
                // Add temperature
                llama_sampler_chain_add(smpl, llama_sampler_init_temp(ctx->config.temperature));
                
                // Add distribution sampler
                llama_sampler_chain_add(smpl, llama_sampler_init_dist(ctx->config.seed));
                
                // Generate tokens
                std::string result;
                int token_buf_size = ctx->config.query_buffers.token_buffer_size;
                std::vector<char> token_buf(token_buf_size);
                std::vector<llama_token> gen_tokens(1);
                
                for (int i = 0; i < ctx->config.max_tokens; i++) {
                    llama_token next_token = llama_sampler_sample(smpl, ctx->model->ctx, -1);
                    
                    // Check for EOS
                    if (llama_vocab_is_eog(vocab, next_token)) {
                        break;
                    }
                    
                    // Convert token to text
                    int n = llama_token_to_piece(vocab, next_token, token_buf.data(), token_buf_size, 0, true);
                    if (n > 0) {
                        result.append(token_buf.data(), n);
                    }
                    
                    // Accept token into sampler
                    llama_sampler_accept(smpl, next_token);
                    
                    // Prepare next iteration
                    gen_tokens[0] = next_token;
                    llama_batch batch = llama_batch_get_one(gen_tokens.data(), 1);
                    if (llama_decode(ctx->model->ctx, batch) != 0) {
                        break;
                    }
                }
                
                llama_sampler_free(smpl);
                
                std::cout << "LLM: Generated " << result.length() << " bytes" << std::endl;
                
                return std::string(R"({"success":true,"text":")" + result + "\"}");
                
            } catch (const std::exception& ex) {
                err = ex.what();
                return std::string(R"({"success":false,"error":")" + std::string(ex.what()) + R"("})");
            }
        }
    };
}
