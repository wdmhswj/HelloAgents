#pragma once

#include <curl/curl.h>
#include "nlohmann/json.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <sstream>

#include "config.hpp"

namespace shared
{

using json = nlohmann::json;

// ------------------------------
// 数据结构
// ------------------------------

struct RequestMessage {
    std::string role;
    std::string content;
};

struct ResponseMessage {
    std::string role;
    std::string content;
    std::string finish_reason;
    std::optional<std::string> reasoning_content;
    std::optional<std::string> reasoning;
};

struct Usage {
    int prompt_tokens = 0;
    int completion_tokens = 0;
    int total_tokens = 0;
};

struct OpenAIChatCompletionRequest {
    std::string model;
    std::vector<RequestMessage> messages;
    bool stream = false;
};

struct OpenAIChatCompletionResponse {
    struct Choice {
        ResponseMessage message;
    };

    std::vector<Choice> choices;
    std::optional<Usage> usage;
};

struct OpenAIChatCompletionStreamChunk {
    struct Choice {
        ResponseMessage delta;
    };

    std::vector<Choice> choices;
    std::optional<Usage> usage;
};

// ------------------------------
// JSON序列化 / 反序列化
// ------------------------------

inline void to_json(json& j, const RequestMessage& m) {
    j = json{
        {"role", m.role},
        {"content", m.content}
    };
}

inline void from_json(const json& j, ResponseMessage& m) {
    if (j.contains("role")) j.at("role").get_to(m.role);
    if (j.contains("content")) j.at("content").get_to(m.content);
    if (j.contains("finish_reason") && !j.at("finish_reason").is_null()) j.at("finish_reason").get_to(m.content);
    if (j.contains("reasoning_content") && !j.at("reasoning_content").is_null()) j.at("reasoning_content").get_to(m.content);
    if (j.contains("reasoning") && !j.at("reasoning").is_null()) j.at("reasoning").get_to(m.content);
}

inline void from_json(const json& j, Usage& u) {
    if (j.contains("prompt_tokens")) j.at("prompt_tokens").get_to(u.prompt_tokens);
    if (j.contains("completion_tokens")) j.at("completion_tokens").get_to(u.completion_tokens);
    if (j.contains("total_tokens")) j.at("total_tokens").get_to(u.total_tokens);
}

inline void to_json(json& j, const OpenAIChatCompletionRequest& r) {
    j = json{
        {"model", r.model},
        {"messages", r.messages},
        {"stream", r.stream}
    };
}

inline void from_json(const json& j, OpenAIChatCompletionResponse::Choice& c) {
    if (j.contains("message")) {
        c.message = j.at("message").get<ResponseMessage>();
    }
}

inline void from_json(const json& j, OpenAIChatCompletionResponse& r) {
    if (j.contains("choices")) {
        r.choices = j.at("choices").get<std::vector<OpenAIChatCompletionResponse::Choice>>();
    }
    if (j.contains("usage") && !j["usage"].is_null()) {
        r.usage = j.at("usage").get<Usage>();
    }
}

inline void from_json(const json& j, OpenAIChatCompletionStreamChunk::Choice& c) {
    if (j.contains("delta")) {
        c.delta = j.at("delta").get<ResponseMessage>();
    }
}

inline void from_json(const json& j, OpenAIChatCompletionStreamChunk& r) {
    if (j.contains("choices")) {
        r.choices = j.at("choices").get<std::vector<OpenAIChatCompletionStreamChunk::Choice>>();
    }
    if (j.contains("usage") && !j["usage"].is_null()) {
        r.usage = j.at("usage").get<Usage>();
    }
}

// ------------------------------
// curl 全局初始化守卫
// ------------------------------

class CurlGlobalGuard {
public:
    CurlGlobalGuard() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~CurlGlobalGuard() {
        curl_global_cleanup();
    }

    CurlGlobalGuard(const CurlGlobalGuard&) = delete;
    CurlGlobalGuard& operator=(const CurlGlobalGuard&) = delete;
};

// ------------------------------
// libcurl 工具函数
// ------------------------------

inline size_t WriteToStringCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t real_size = size * nmemb;
    auto* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), real_size);
    return real_size;
}

struct StreamContext {
    std::string pending;
};

inline void ProcessSSELine(const std::string& line) {
    if (line.empty()) return;

    if (line.rfind("data:", 0) == 0) {
        std::string v = line.substr(5);

        while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) {
            v.erase(v.begin());
        }

        if (v == "[DONE]") {
            std::cout << "\n[stream done]\n";
            return;
        }

        try {
            auto j = json::parse(v);
            auto chunk = j.get<OpenAIChatCompletionStreamChunk>();

            if (!chunk.choices.empty()) {
                const auto& delta = chunk.choices[0].delta;
                if (!delta.content.empty()) {
                    std::cout << delta.content << std::flush;
                }
            }

            if (chunk.usage.has_value()) {
                const auto& u = chunk.usage.value();
                std::cout << "\n[token usage] "
                          << "prompt_tokens=" << u.prompt_tokens
                          << ", completion_tokens=" << u.completion_tokens
                          << ", total_tokens=" << u.total_tokens
                          << '\n';
            }
        } catch (const std::exception& e) {
            std::cerr << "failed to parse stream chunk: " << e.what() << "\n";
        }
    }
}

inline size_t StreamCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t real_size = size * nmemb;
    auto* ctx = static_cast<StreamContext*>(userp);

    ctx->pending.append(static_cast<char*>(contents), real_size);

    size_t pos = 0;
    while ((pos = ctx->pending.find('\n')) != std::string::npos) {
        std::string line = ctx->pending.substr(0, pos);

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        ProcessSSELine(line);
        ctx->pending.erase(0, pos + 1);
    }

    return real_size;
}

// ------------------------------
// 非流式请求
// ------------------------------

inline void NonStreamingRequestRawHTTP(const ModelConfig& modelConf, const std::string& query) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "failed to init curl\n";
        return;
    }

    OpenAIChatCompletionRequest requestBody;
    requestBody.model = modelConf.model;
    requestBody.messages = {{"user", query}};
    requestBody.stream = false;

    std::string request_json = json(requestBody).dump();
    std::string response_body;

    std::string url = modelConf.base_url + "/chat/completions";
    std::cout << "request url: " << url << "\n";
    std::string auth = "Authorization: Bearer " + modelConf.api_key;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_json.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(request_json.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "failed to send http request: " << curl_easy_strerror(res) << "\n";
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return;
    }

    long status_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    if (status_code != 200) {
        std::cerr << "http request failed, status code: " << status_code << "\n";
        std::cerr << "response body: " << response_body << "\n";
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return;
    }

    try {
        auto j = json::parse(response_body);
        auto resp = j.get<OpenAIChatCompletionResponse>();

        if (resp.choices.empty()) {
            std::cout << "no choices returned\n";
        } else {
            std::cout << "resp content: " << resp.choices[0].message.content << "\n";
        }

        if (resp.usage.has_value()) {
            const auto& u = resp.usage.value();
            std::cout << "token usage: "
                      << "prompt_tokens=" << u.prompt_tokens
                      << ", completion_tokens=" << u.completion_tokens
                      << ", total_tokens=" << u.total_tokens
                      << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "failed to unmarshal http response: " << e.what() << "\n";
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

// ------------------------------
// 流式请求
// ------------------------------

inline void StreamingRequestRawHTTP(const ModelConfig& modelConf, const std::string& query) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "failed to init curl\n";
        return;
    }

    OpenAIChatCompletionRequest requestBody;
    requestBody.model = modelConf.model;
    requestBody.messages = {{"user", query}};
    requestBody.stream = true;

    std::string request_json = json(requestBody).dump();

    std::string url = modelConf.base_url + "/chat/completions";
    std::string auth = "Authorization: Bearer " + modelConf.api_key;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth.c_str());

    StreamContext ctx;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_json.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(request_json.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StreamCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "failed to send http request: " << curl_easy_strerror(res) << "\n";
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return;
    }

    long status_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
    if (status_code != 200) {
        std::cerr << "http request failed, status code: " << status_code << "\n";
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return;
    }

    if (!ctx.pending.empty()) {
        ProcessSSELine(ctx.pending);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

} // namespace shared
