#pragma once

#include <iostream>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef ERROR
#undef TRUE
#undef FALSE
#undef IN
#endif

namespace pyle::lsp {

class JsonRpcTransport {
public:
    static std::string read_message() {
        int content_length = 0;
#ifdef _WIN32
        HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
        std::string buf;
        char c;
        while (true) {
            DWORD read = 0;
            if (!ReadFile(in, &c, 1, &read, NULL) || read == 0) return {};
            buf += c;
            if (buf.size() >= 4 && buf.compare(buf.size() - 4, 4, "\r\n\r\n") == 0) {
                auto hdr = buf.substr(0, buf.size() - 4);
                auto pos = hdr.find("Content-Length: ");
                if (pos != std::string::npos)
                    content_length = std::stoi(hdr.substr(pos + 16));
                break;
            }
        }
        if (content_length <= 0) return {};
        std::string body(content_length, '\0');
        DWORD read = 0;
        if (!ReadFile(in, body.data(), content_length, &read, NULL) || read != static_cast<DWORD>(content_length))
            return {};
        return body;
#else
        std::string header;
        while (std::getline(std::cin, header)) {
            if (header == "\r" || header.empty()) break;
            auto pos = header.find("Content-Length: ");
            if (pos != std::string::npos)
                content_length = std::stoi(header.substr(pos + 16));
        }
        if (content_length <= 0) return {};
        std::string body(content_length, '\0');
        std::cin.read(body.data(), content_length);
        if (std::cin.gcount() != content_length) return {};
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
