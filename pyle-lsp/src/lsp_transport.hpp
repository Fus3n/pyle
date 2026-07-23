#pragma once

#include <iostream>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace pyle::lsp {

class JsonRpcTransport {
public:
    static std::string read_message() {
    #ifdef _WIN32
        std::string buf;
        int content_length = 0;
        char c;
        while (true) {
            DWORD read = 0;
            if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), &c, 1, &read, NULL) || read == 0)
                return {};
            buf += c;
            if (buf.size() >= 4 && buf.substr(buf.size() - 4) == "\r\n\r\n") {
                auto hdr = buf.substr(0, buf.size() - 4);
                auto p = hdr.find("Content-Length: ");
                if (p != std::string::npos)
                    content_length = std::stoi(hdr.substr(p + 16));
                break;
            }
        }
        if (content_length <= 0) return {};
        std::string body(content_length, '\0');
        DWORD read = 0;
        ReadFile(GetStdHandle(STD_INPUT_HANDLE), body.data(), content_length, &read, NULL);
        if (read != static_cast<DWORD>(content_length)) return {};
        return body;
    #else
        std::string header;
        int content_length = 0;
        while (std::getline(std::cin, header)) {
            if (header == "\r" || header.empty()) break;
            if (header.find("Content-Length: ") == 0) {
                content_length = std::stoi(header.substr(16));
            }
        }
        if (content_length <= 0) return {};
        std::string body(content_length, '\0');
        std::cin.read(body.data(), content_length);
        return body;
    #endif
    }

    static void write_message(const std::string& body) {
        std::string msg = "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    #ifdef _WIN32
        DWORD written = 0;
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msg.data(), static_cast<DWORD>(msg.size()), &written, NULL);
    #else
        std::cout << msg << std::flush;
    #endif
    }
};

} 