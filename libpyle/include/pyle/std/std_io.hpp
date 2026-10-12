#pragma once
#include <istream>
#include <string>

namespace pyle {

    inline std::string read_stream_all(std::istream& in) {
        std::string data;

        in.clear();
        std::streampos cur = in.tellg();
        if (cur != std::streampos(-1)) {
            in.seekg(0, std::ios::end);
            std::streamoff end = in.tellg();
            in.seekg(cur);
            if (end != std::streamoff(-1)) {
                std::streamoff size = end - static_cast<std::streamoff>(cur);
                if (size > 0) {
                    data.resize(static_cast<size_t>(size));
                    in.read(&data[0], size);
                    data.resize(static_cast<size_t>(in.gcount()));
                }
                return data;
            }
        }

        char buf[65536];
        while (in) {
            in.read(buf, sizeof(buf));
            data.append(buf, static_cast<size_t>(in.gcount()));
        }
        return data;
    }

}
