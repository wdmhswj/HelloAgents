#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "raw.hpp"


static std::string Trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r' || s[start] == '\n')) {
        ++start;
    }

    size_t end = s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r' || s[end - 1] == '\n')) {
        --end;
    }

    return s.substr(start, end - start);
}

static std::string Unquote(const std::string& s) {
    if (s.size() >= 2) {
        if ((s.front() == '"' && s.back() == '"') ||
            (s.front() == '\'' && s.back() == '\'')) {
            return s.substr(1, s.size() - 2);
        }
    }
    return s;
}

static void LoadDotEnv(const std::string& path = ".env") {
    std::ifstream in(path);
    if (!in.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        line = Trim(line);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = Trim(line.substr(0, pos));
        std::string value = Trim(line.substr(pos + 1));
        value = Unquote(value);

#ifdef _WIN32
        _putenv_s(key.c_str(), value.c_str());
#else
        setenv(key.c_str(), value.c_str(), 1);
#endif
    }
}


int main(int argc, char* argv[]) {
    LoadDotEnv();

    bool useRaw = false;
    bool useStream = false;
    std::string query = "hello";

    // 简单命令行解析
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--raw") {
            useRaw = true;
        } else if (arg == "--stream") {
            useStream = true;
        } else if ((arg == "-q" || arg == "--q") && i + 1 < argc) {
            query = argv[++i];
        } else if (arg.rfind("-q=", 0) == 0) {
            query = arg.substr(3);
        } else if (arg.rfind("--q=", 0) == 0) {
            query = arg.substr(4);
        } else {
            std::cerr << "unknown argument: " << arg << "\n";
            return 1;
        }
    }

    // 对应 Go: modelConf := shared.NewModelConfig()
    shared::ModelConfig modelConf = shared::NewModelConfig();

    // 如果你的 chat.hpp 是 header-only，并需要 curl 全局初始化
    shared::CurlGlobalGuard curlGuard;

    // 对应 Go 的 switch
    if (useRaw && useStream) {
        shared::StreamingRequestRawHTTP(
            modelConf,
            query
        );
    } else if (useRaw) {
        shared::NonStreamingRequestRawHTTP(
            modelConf,
            query
        );
    } else if (useStream) {
        // 这里对应 Go 的 StreamingRequestSDK
        // 如果你还没实现 SDK 版本，可以先提示
        std::cerr << "StreamingRequestSDK is not implemented in C++ yet.\n";
        return 1;
    } else {
        // 这里对应 Go 的 NonStreamingRequestSDK
        // 如果你还没实现 SDK 版本，可以先提示
        std::cerr << "NonStreamingRequestSDK is not implemented in C++ yet.\n";
        return 1;
    }

    return 0;
}