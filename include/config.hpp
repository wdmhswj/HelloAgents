#pragma once

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>

#include "nlohmann/json.hpp"

namespace shared {

using json = nlohmann::json;

// ------------------------------
// 配置结构
// ------------------------------

struct ModelConfig {
    std::string base_url;
    std::string api_key;
    std::string model;
    int context_window = 0;
};

struct LLMProviders {
    ModelConfig front_model;
    ModelConfig back_model;
};

struct AppConfig {
    LLMProviders llm_providers;
};

// ------------------------------
// 环境变量工具函数
// ------------------------------

inline std::string getEnvDefault(const std::string& key, const std::string& defaultValue) {
    const char* value = std::getenv(key.c_str());
    if (value != nullptr) {
        return std::string(value);
    }
    return defaultValue;
}

inline int getEnvDefaultInt(const std::string& key, int defaultValue) {
    const char* value = std::getenv(key.c_str());
    if (value != nullptr) {
        try {
            return std::stoi(value);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

// ------------------------------
// 默认 ModelConfig
// ------------------------------

inline ModelConfig NewModelConfig() {
    return ModelConfig{
        .base_url = getEnvDefault("OPENAI_BASE_URL", "https://api.openai.com/v1"),
        .api_key = getEnvDefault("OPENAI_API_KEY", ""),
        .model = getEnvDefault("OPENAI_MODEL", "gpt-5.2"),
        .context_window = 200000
    };
}

// ------------------------------
// JSON 序列化 / 反序列化
// ------------------------------

inline void to_json(json& j, const ModelConfig& c) {
    j = json{
        {"base_url", c.base_url},
        {"api_key", c.api_key},
        {"model", c.model},
        {"context_window", c.context_window}
    };
}

inline void from_json(const json& j, ModelConfig& c) {
    if (j.contains("base_url")) j.at("base_url").get_to(c.base_url);
    if (j.contains("api_key")) j.at("api_key").get_to(c.api_key);
    if (j.contains("model")) j.at("model").get_to(c.model);
    if (j.contains("context_window")) j.at("context_window").get_to(c.context_window);
}

inline void to_json(json& j, const LLMProviders& p) {
    j = json{
        {"front_model", p.front_model},
        {"back_model", p.back_model}
    };
}

inline void from_json(const json& j, LLMProviders& p) {
    if (j.contains("front_model")) j.at("front_model").get_to(p.front_model);
    if (j.contains("back_model")) j.at("back_model").get_to(p.back_model);
}

inline void to_json(json& j, const AppConfig& c) {
    j = json{
        {"llm_providers", c.llm_providers}
    };
}

inline void from_json(const json& j, AppConfig& c) {
    if (j.contains("llm_providers")) {
        j.at("llm_providers").get_to(c.llm_providers);
    }
}

// ------------------------------
// 加载配置文件
// ------------------------------

inline AppConfig LoadAppConfig(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("failed to open config file: " + path);
    }

    json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        throw std::runtime_error("failed to parse json config file: " + path + ", error: " + e.what());
    }

    try {
        return j.get<AppConfig>();
    } catch (const std::exception& e) {
        throw std::runtime_error("failed to deserialize app config: " + path + ", error: " + e.what());
    }
}

} // namespace shared