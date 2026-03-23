#pragma once

#include "openai/openai.hpp"

#include <string_view>
#include <optional>
#include <unordered_map>
#include <cstdlib> // for std::getenv

enum class Provider {
    OpenAI,
    DeepSeek,
    Qwen,
    ModelScope,
    Kimi,
    Zhipu,
    Ollama,
    VLLM,
    Local,
    Auto,
    Custom,
    // 可以添加 Unknown 用于非法输入
};

// 将枚举转为字符串（可用于日志、序列化）
constexpr std::string_view providerToString(Provider p) {
    switch (p) {
        case Provider::OpenAI:     return "openai";
        case Provider::DeepSeek:   return "deepseek";
        case Provider::Qwen:       return "qwen";
        case Provider::ModelScope: return "modelscope";
        case Provider::Kimi:       return "kimi";
        case Provider::Zhipu:      return "zhipu";
        case Provider::Ollama:     return "ollama";
        case Provider::VLLM:       return "vllm";
        case Provider::Local:      return "local";
        case Provider::Auto:       return "auto";
        case Provider::Custom:     return "custom";
        default:         return "unknown";
    }
}

// 将字符串解析为枚举（返回 std::optional 表示可能失败）
inline Provider stringToProvider(std::string_view sv) {
    static const std::unordered_map<std::string_view, Provider> map = {
        {"openai",     Provider::OpenAI},
        {"deepseek",   Provider::DeepSeek},
        {"qwen",       Provider::Qwen},
        {"modelscope", Provider::ModelScope},
        {"kimi",       Provider::Kimi},
        {"zhipu",      Provider::Zhipu},
        {"ollama",     Provider::Ollama},
        {"vllm",       Provider::VLLM},
        {"local",      Provider::Local},
        {"auto",       Provider::Auto},
        {"custom",     Provider::Custom},
    };
    auto it = map.find(sv);
    if (it != map.end())
        return it->second;
    return Provider::Auto; // 默认返回Auto，表示未识别的输入
}


// class HelloAgentsLLM {
// /*
//     为HelloAgents定制的LLM客户端。
//     它用于调用任何兼容OpenAI接口的服务，并默认使用流式响应。

//     设计理念：
//     - 参数优先，环境变量兜底
//     - 流式响应为默认，提供更好的用户体验
//     - 支持多种LLM提供商
//     - 统一的调用接口
// */

// private:
//     std::string _model;
//     std::string _api_key;
//     std::string _base_url;
//     std::string _provider;
//     float _temperature;
//     int _max_tokens;
//     int _timeout;
//     // openai client
//     std::unique_ptr<openai::OpenAI> _client;

// public:
//     HelloAgentsLLM(const std::string& model = "", const std::string& api_key = "", const std::string& base_url = "", const std::string& provider = "", float temperature = 0.7, int max_tokens = 2048, int timeout = 60)
//         : _model(model), _api_key(api_key), _base_url(base_url), _provider(provider), _temperature(temperature), _max_tokens(max_tokens), _timeout(timeout) {
//         // 自动检测provider或使用指定的provider
//         _provider = provider.empty() ? "auto" : provider;
        
//         if (_provider == "custom") {
//             _api_key = (_api_key.empty() ? std::getenv("LLM_API_KEY") : _api_key);
//             _base_url = (_base_url.empty() ? std::getenv("LLM_BASE_URL") : _base_url);
//         } else {
//             std::tie(_api_key, _base_url) = _resolve_credentials(api_key, base_url);
//         }

//         _model = _get_default_model();

//         if (_api_key.empty() || _base_url.empty()) {
//             throw std::runtime_error("API密钥和服务地址必须被提供或在.env文件中定义。");
//         }
//         _client = std::make_unique<openai::OpenAI>(_max_tokens, "", true, _base_url, "");

//     }

// private:
//     void _auto_detect_provider() {

//     }

//     std::pair<std::string, std::string> _resolve_credentials(const std::string& api_key, const std::string& base_url) {
//         // 根据_provider自动解析API Key和Base URL
//         // 这里可以添加不同provider的默认URL和环境变量逻辑
//         if (_provider == "openai") {
//             return {api_key.empty() ? std::getenv("OPENAI_API_KEY") : api_key,
//                     base_url.empty() ? "https://api.openai.com/v1" : base_url};
//         }
//         // 其他provider的解析逻辑...
//         return {api_key, base_url}; // 默认返回输入值
//     }

//     std::string _get_default_model() {
//         switch (stringToProvider(_provider)) {
//         case Provider::OpenAI:
//             return "gpt-3.5-turbo";
            
//         case Provider::DeepSeek:
//             return "deepseek-chat";
            
//         case Provider::Qwen:
//             return "qwen-plus";
            
//         case Provider::ModelScope:
//             return "Qwen/Qwen2.5-72B-Instruct";
            
//         case Provider::Kimi:
//             return "moonshot-v1-8k";
            
//         case Provider::Zhipu:
//             return "glm-4";
            
//         case Provider::Ollama:
//             return "llama3.2";  // Ollama常用模型
            
//         case Provider::VLLM:
//             return "meta-llama/Llama-2-7b-chat-hf";  // vLLM常用模型
            
//         case Provider::Local:
//             return "local-model";  // 本地模型占位符
            
//         case Provider::Custom:
//             return "deepseek-chat";
            
//         case Provider::Auto:
//         default: {
//             // auto或其他情况：根据base_url智能推断默认模型
//             std::string base_url_lower = _base_url;
//             std::transform(base_url_lower.begin(), base_url_lower.end(), 
//                           base_url_lower.begin(), ::tolower);
            
//             // 按优先级检查各种模式
//             if (base_url_lower.find("modelscope") != std::string::npos) {
//                 return "Qwen/Qwen2.5-72B-Instruct";
//             } else if (base_url_lower.find("deepseek") != std::string::npos) {
//                 return "deepseek-chat";
//             } else if (base_url_lower.find("dashscope") != std::string::npos) {
//                 return "qwen-plus";
//             } else if (base_url_lower.find("moonshot") != std::string::npos) {
//                 return "moonshot-v1-8k";
//             } else if (base_url_lower.find("bigmodel") != std::string::npos) {
//                 return "glm-4";
//             } else if (base_url_lower.find("ollama") != std::string::npos ||
//                        base_url_lower.find(":11434") != std::string::npos) {
//                 return "llama3.2";
//             } else if (base_url_lower.find(":8000") != std::string::npos ||
//                        base_url_lower.find("vllm") != std::string::npos) {
//                 return "meta-llama/Llama-2-7b-chat-hf";
//             } else if (base_url_lower.find("localhost") != std::string::npos ||
//                        base_url_lower.find("127.0.0.1") != std::string::npos) {
//                 return "local-model";
//             } else {
//                 return "gpt-3.5-turbo";
//             }
//         }
//         }
//     }
// };