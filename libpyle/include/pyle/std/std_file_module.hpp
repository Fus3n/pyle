#pragma once
#include "pyle/value.hpp"
#include <fstream>
#include <string>
#include "pyle/std/std_future.hpp"

namespace pyle {

    class FileInstance {
    private:
        std::fstream stream;
        std::string path;
        std::string mode;

    public:
        FileInstance(const std::string& path, const std::string& mode);
        ~FileInstance();

        bool is_open() const { return stream.is_open(); }

        int64_t size(VM& vm);
        
        void close(VM& vm);
        pyle::Value read_line(VM& vm);
        pyle::Value read_all(VM& vm);
        void write(VM& vm, const std::string& text);

        void seek(VM& vm, int64_t offset, int64_t origin);
        pyle::Value tell(VM& vm);

        pyle::Value read_bytes(VM& vm, pyle::ArgView args);
        void write_bytes(VM& vm, pyle::ArgView args);

        pyle::Value read_line_async(VM& vm);
        pyle::Value read_all_async(VM& vm);
        pyle::Value write_async(VM& vm, const std::string& text);
        pyle::Value read_bytes_async(VM& vm, pyle::ArgView args);
        pyle::Value write_bytes_async(VM& vm, pyle::ArgView args);
    };

    void register_file_module(VM& vm);
}