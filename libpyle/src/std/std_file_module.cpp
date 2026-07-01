#include "pyle/std/std_file_module.hpp"
#include "pyle/binder.hpp"
#include <sstream>

namespace pyle {
    
    FileInstance::FileInstance(const std::string& path, const std::string& mode) 
        : path(path), mode(mode) {
        
        std::ios_base::openmode openmode = std::ios_base::in;
        
        if (mode == "r" || mode == "rb") {
            openmode = std::ios_base::in;
            if (mode == "rb") openmode |= std::ios_base::binary;
        } else if (mode == "w" || mode == "wb") {
            openmode = std::ios_base::out | std::ios_base::trunc;
            if (mode == "wb") openmode |= std::ios_base::binary;
        } else if (mode == "a" || mode == "ab") {
            openmode = std::ios_base::out | std::ios_base::app;
            if (mode == "ab") openmode |= std::ios_base::binary;
        }
        
        stream.open(path, openmode);
    }

    FileInstance::~FileInstance() {
        if (stream.is_open()) {
            stream.close();
        }
    }

    void FileInstance::close(VM& vm) {
        if (!stream.is_open()) {
            vm.runtime_error(pyle::RuntimeError::Runtime, "File: file is already closed.");
            return;
        }
        stream.close();
    }

    int64_t FileInstance::size(VM& vm) {
        if (!stream.is_open()) {
            vm.runtime_error(RuntimeError::Runtime, "Cannot get size of a closed file.");
            return 0;
        }
        
        auto current_pos = stream.tellg();
        stream.seekg(0, std::ios::end);
        auto total_size = stream.tellg();
        stream.clear();
        stream.seekg(current_pos, std::ios::beg);
        return total_size;
    }

    pyle::Value FileInstance::read_line(VM& vm) {
        if (!stream.is_open()) {
            vm.runtime_error(pyle::RuntimeError::Runtime, "File: cannot read from a closed file.");
            return pyle::Value();
        }
        
        std::string line;
        if (std::getline(stream, line)) {
            return pyle::to_value(vm, line);
        }
        
        return pyle::Value(); 
    }

    pyle::Value FileInstance::read_all(VM& vm) {
        if (!stream.is_open()) {
            vm.runtime_error(pyle::RuntimeError::Runtime, "File: cannot read from a closed file.");
            return pyle::Value();
        }
        
        std::stringstream ss;
        ss << stream.rdbuf();
        return pyle::to_value(vm, ss.str());
    }

    void FileInstance::write(VM& vm, const std::string& text) {
        if (!stream.is_open()) {
            vm.runtime_error(pyle::RuntimeError::Runtime, "File: cannot write to a closed file.");
            return;
        }
        
        stream << text;
        
        if (stream.fail()) {
            vm.runtime_error(pyle::RuntimeError::Runtime, "File: failed to write data to file: " + path);
        }
    }

    pyle::Value pyle_file_open(pyle::VM& vm, std::string path, std::string mode) {
        if (mode != "r" && mode != "w" && mode != "a" && 
            mode != "rb" && mode != "wb" && mode != "ab") {
            vm.runtime_error(pyle::RuntimeError::ArgumentError, 
                "File: invalid file mode '" + mode + "'. Expected 'r', 'w', 'a', 'rb', 'wb', or 'ab'.");
            return pyle::Value();
        }

        auto* file = new FileInstance(path, mode);
        
        if (!file->is_open()) {
            delete file;
            vm.runtime_error(pyle::RuntimeError::Runtime, "Could not open file: " + path);
            return pyle::Value();
        }
        
        return to_value_owned(vm, file);
    }

    void FileInstance::seek(VM& vm, int64_t offset, int64_t origin) {
        if (!stream.is_open()) {
            vm.runtime_error(RuntimeError::Runtime, "File: cannot seek in a closed file.");
            return;
        }

        std::ios_base::seekdir dir;
        if (origin == 0) dir = std::ios_base::beg;
        else if (origin == 1) dir = std::ios_base::cur;
        else if (origin == 2) dir = std::ios_base::end;
        else {
            vm.runtime_error(RuntimeError::ArgumentError, "File: invalid seek origin. Use 0 (SET), 1 (CUR), or 2 (END).");
            return;
        }

        stream.clear(); 
        stream.seekg(offset, dir);
        stream.seekp(offset, dir);

        if (stream.fail()) {
            vm.runtime_error(RuntimeError::Runtime, "File: seek operation failed. Offset out of bounds?");
        }
    }

       pyle::Value FileInstance::tell(VM& vm) {
        if (!stream.is_open()) {
            vm.runtime_error(RuntimeError::Runtime, "File: cannot tell in a closed file.");
            return pyle::Value();
        }
        
        int64_t pos = stream.tellg();
        if (pos == -1) {
            vm.runtime_error(RuntimeError::Runtime, "File: tell operation failed.");
            return pyle::Value();
        }
        return pyle::Value(pos);
    }

    pyle::Value FileInstance::read_bytes(VM& vm, pyle::ArgView args) {
        if (!stream.is_open()) {
            vm.runtime_error(RuntimeError::Runtime, "Cannot read from a closed file.");
            return pyle::Value();
        }

        int64_t count = -1; 
        
        if (args.size() == 1) {
            if (args[0].tag != pyle::Value::Tag::Int) {
                vm.runtime_error(RuntimeError::Type, "file.read_bytes expects an integer count.");
                return pyle::Value();
            }
            count = args[0].as_int;
            if (count < 0) {
                vm.runtime_error(RuntimeError::ArgumentError, "Byte count cannot be negative.");
                return pyle::Value();
            }
        } else if (args.size() > 1) {
            vm.runtime_error(RuntimeError::ArgumentError, "file.read_bytes expects 0 or 1 arguments.");
            return pyle::Value();
        }

        if (count == -1) {
            auto current_pos = stream.tellg();
            stream.seekg(0, std::ios::end); 
            auto end_pos = stream.tellg(); 
            stream.seekg(current_pos, std::ios::beg); 
            
            count = end_pos - current_pos;
        }

        if (count == 0) {
            return pyle::Value(); 
        }

        pyle::BytesType buffer(count);
        stream.read(reinterpret_cast<char*>(buffer.data()), count);
        std::streamsize bytes_read = stream.gcount();

        if (bytes_read == 0) {
            return pyle::Value(); 
        }

        buffer.resize(bytes_read); 
        HeapIdx idx = vm.alloc(Object(std::move(buffer)));
        return pyle::Value(pyle::Value::Tag::BytesRef, idx);
    }

    void FileInstance::write_bytes(VM& vm, pyle::ArgView args) {
        if (!stream.is_open()) {
            vm.runtime_error(RuntimeError::Runtime, "Cannot write to a closed file.");
            return;
        }
        if (args.size() != 1 || args[0].tag != pyle::Value::Tag::BytesRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "write_bytes expects 1 bytes object argument.");
            return;
        }

        const auto& buffer = std::get<pyle::BytesType>(vm.get_heap_object(args[0].as_ref).data);
        stream.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        
        if (stream.fail()) {
            vm.runtime_error(RuntimeError::Runtime, "Failed to write bytes to file.");
        }
    }

    void register_file_module(VM& vm) {
        pyle::ClassBinder<FileInstance>(vm, "File")
            .static_method<&pyle_file_open>("open") 
            .method<&FileInstance::read_line>("read_line")
            .method<&FileInstance::read_all>("read_all")
            .method<&FileInstance::write>("write")
            .method<&FileInstance::read_bytes>("read_bytes")
            .method<&FileInstance::write_bytes>("write_bytes")
            .method<&FileInstance::seek>("seek")
            .method<&FileInstance::tell>("tell")
            .method<&FileInstance::close>("close")
            .method<&FileInstance::size>("size")
            .register_globally(); 
    }
}