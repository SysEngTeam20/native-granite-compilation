#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>

// Include the httplib header
// #define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"

#include "llama.h"

// TODO:
// Fix GGML bindings

// Model server class to handle inference requests
class LlamaServer {
private:
    struct llama_model* model;
    struct llama_context* ctx;
    const struct llama_vocab* vocab;
    llama_sampler* smpl_chain;
    std::string model_path;
    int port;

public:
    LlamaServer(const std::string& model_path, int port = 8080) 
        : model_path(model_path), port(port), model(nullptr), ctx(nullptr), vocab(nullptr), smpl_chain(nullptr) {}

    ~LlamaServer() {
        cleanup();
    }

    bool initialize() {
        // Set llama model and context parameters
        struct llama_model_params mparams = llama_model_default_params();
        mparams.n_gpu_layers = 99;  // Using 99 to utilize GPU acceleration
        
        struct llama_context_params ctx_params = llama_context_default_params();
        ctx_params.embeddings = false;
        ctx_params.n_ctx     = 8192;  // context size
        ctx_params.n_threads = 4;     // CPU threads

        // Load the model
        model = llama_model_load_from_file(model_path.c_str(), mparams);
        if (!model) {
            fprintf(stderr, "Error: Failed to load model from '%s'\n", model_path.c_str());
            return false;
        }

        // Create context
        ctx = llama_init_from_model(model, ctx_params);
        if (!ctx) {
            fprintf(stderr, "Error: Failed to create llama_context\n");
            llama_model_free(model);
            model = nullptr;
            return false;
        }

        // Get model vocabulary
        vocab = llama_model_get_vocab(model);

        // Initialize sampler chain
        smpl_chain = llama_sampler_chain_init(llama_sampler_chain_default_params());
        llama_sampler_chain_add(smpl_chain, llama_sampler_init_min_p(0.05f, 1));
        llama_sampler_chain_add(smpl_chain, llama_sampler_init_temp(0.8f));
        llama_sampler_chain_add(smpl_chain, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

        return true;
    }

    void cleanup() {
        if (smpl_chain) {
            llama_sampler_free(smpl_chain);
            smpl_chain = nullptr;
        }
        if (ctx) {
            llama_free(ctx);
            ctx = nullptr;
        }
        if (model) {
            llama_model_free(model);
            model = nullptr;
        }
    }

    std::string generate(const std::string& user_prompt) {
        // Reset KV cache between requests
        llama_kv_cache_clear(ctx);
        
        // Prepare prompt
        std::vector<llama_chat_message> messages;
        std::vector<char> formatted(llama_n_ctx(ctx));
        const char* tmpl = llama_model_chat_template(model, nullptr);

        // Add the user input to the message list and format it
        messages.push_back({"user", strdup(user_prompt.c_str())});
        int new_len = llama_chat_apply_template(tmpl, messages.data(), messages.size(), true, formatted.data(), formatted.size());
        if (new_len > (int)formatted.size()) {
            formatted.resize(new_len);
            new_len = llama_chat_apply_template(tmpl, messages.data(), messages.size(), true, formatted.data(), formatted.size());
        }
        if (new_len < 0) {
            return "Error: Failed to apply chat template";
        }

        // Get formatted prompt
        std::string prompt(formatted.begin(), formatted.begin() + new_len);

        const bool is_first = llama_get_kv_cache_used_cells(ctx) == 0;

        // Tokenize the prompt
        const int n_prompt_tokens = -llama_tokenize(vocab, prompt.c_str(), prompt.size(), NULL, 0, is_first, true);
        std::vector<llama_token> prompt_tokens(n_prompt_tokens);
        if (llama_tokenize(vocab, prompt.c_str(), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), is_first, true) < 0) {
            return "Error: Failed to tokenize prompt";
        }

        // Prepare response
        std::string response;

        // Prepare a batch for the prompt
        llama_batch batch = llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
        llama_token new_token_id;

        while (true) {
            // Check if we have enough space in the context
            int n_ctx = llama_n_ctx(ctx);
            int n_ctx_used = llama_get_kv_cache_used_cells(ctx);
            if (n_ctx_used + batch.n_tokens > n_ctx) {
                response += "\n[Context size exceeded]";
                break;
            }

            if (llama_decode(ctx, batch)) {
                response += "\n[Decoding failed]";
                break;
            }

            // Sample the next token
            new_token_id = llama_sampler_sample(smpl_chain, ctx, -1);

            // Check if token generated is end of generation token
            if (llama_vocab_is_eog(vocab, new_token_id)) {
                break;
            }

            // Convert the token to a string and add it to the response
            char buf[256];
            int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, true);
            if (n < 0) {
                response += "\n[Token conversion failed]";
                break;
            }
            
            std::string piece(buf, n);
            response += piece;

            // Prepare the next batch with the sampled token
            batch = llama_batch_get_one(&new_token_id, 1);
        }

        // Free allocated chat message strings
        for (auto& msg : messages) {
            free((void*)msg.content);
        }

        return response;
    }

    std::string escape_json(const std::string &s) {
        std::ostringstream o;
        for (auto c = s.cbegin(); c != s.cend(); c++) {
            switch (*c) {
            case '"': o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b"; break;
            case '\f': o << "\\f"; break;
            case '\n': o << "\\n"; break;
            case '\r': o << "\\r"; break;
            case '\t': o << "\\t"; break;
            default:
                if ('\x00' <= *c && *c <= '\x1f') {
                    o << "\\u"
                      << std::hex << std::setw(4) << std::setfill('0') << (int)*c;
                } else {
                    o << *c;
                }
            }
        }
        return o.str();
    }

    void run() {
        httplib::Server server;

        // Define a health check endpoint
        server.Get("/health", [](const httplib::Request&, httplib::Response& res) {
            res.set_content("LLM Server is running", "text/plain");
        });

        // Define an endpoint for text generation
        server.Post("/generate", [this](const httplib::Request& req, httplib::Response& res) {
            if (!req.has_param("prompt")) {
                res.status = 400;
                res.set_content("Error: 'prompt' parameter is required", "text/plain");
                return;
            }

            const std::string prompt = req.get_param_value("prompt");
            std::string response = generate(prompt);
            
            res.set_content(response, "text/plain");
        });

        // Define an endpoint for JSON input/output
        server.Post("/api/generate", [this](const httplib::Request& req, httplib::Response& res) {
            // Parse JSON input
            if (req.body.empty() || req.get_header_value("Content-Type") != "application/json") {
                res.status = 400;
                res.set_content("{\"error\":\"Request must include JSON body\"}", "application/json");
                return;
            }

            // Simple JSON parsing - in a production app you'd use a proper JSON library
            std::string prompt;
            size_t pos = req.body.find("\"prompt\"");
            if (pos != std::string::npos) {
                pos = req.body.find(":", pos);
                if (pos != std::string::npos) {
                    pos = req.body.find("\"", pos);
                    if (pos != std::string::npos) {
                        size_t end = req.body.find("\"", pos + 1);
                        if (end != std::string::npos) {
                            prompt = req.body.substr(pos + 1, end - pos - 1);
                        }
                    }
                }
            }

            if (prompt.empty()) {
                res.status = 400;
                res.set_content("{\"error\":\"'prompt' field required in JSON body\"}", "application/json");
                return;
            }

            std::string response = generate(prompt);
            
            // Create JSON response
            std::ostringstream json_response;
            json_response << "{\"response\":\"" << escape_json(response) << "\"}";
            
            res.set_content(json_response.str(), "application/json");
        });

        std::cout << "Server starting on http://localhost:" << port << std::endl;
        server.listen("0.0.0.0", port);
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <model-path.gguf> [port=8080]\n", argv[0]);
        return 1;
    }

    const std::string model_path = argv[1];
    int port = 8080;
    
    if (argc >= 3) {
        port = std::stoi(argv[2]);
    }

    LlamaServer server(model_path, port);
    
    if (!server.initialize()) {
        return 1;
    }
    
    server.run();
    
    return 0;
}
