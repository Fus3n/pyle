#include "pyle/std/std_core_modules.hpp"
#include "pyle/std/std_io.hpp"
#include "pyle/binder.hpp"
#include "pyle/value.hpp"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <chrono>

namespace pyle {


    namespace {
        namespace fs = std::filesystem;

        bool fs_string(VM& vm, ArgView args, size_t i, std::string& out, const char* who) {
            if (args.size() <= i || args[i].tag != Value::Tag::StringRef) {
                vm.runtime_error(RuntimeError::ArgumentError, std::string(who) + " expects string arguments.");
                return false;
            }
            out = std::get<std::string>(vm.get_heap_object(args[i].as_ref).data);
            return true;
        }

        bool wildcard_match(const std::string& name, const std::string& pat) {
            size_t n = 0, p = 0;
            size_t star = std::string::npos, mark = 0;
            while (n < name.size()) {
                if (p < pat.size() && (pat[p] == '?' || pat[p] == name[n])) { ++n; ++p; }
                else if (p < pat.size() && pat[p] == '*') { star = p++; mark = n; }
                else if (star != std::string::npos) { p = star + 1; n = ++mark; }
                else return false;
            }
            while (p < pat.size() && pat[p] == '*') ++p;
            return p == pat.size();
        }
    }

    Value fs_exists(VM& vm, ArgView args) {
        std::string p;
        if (!fs_string(vm, args, 0, p, "fs.exists")) return Value();
        std::error_code ec;
        return Value(fs::exists(p, ec) && !ec);
    }

    Value fs_is_file(VM& vm, ArgView args) {
        std::string p;
        if (!fs_string(vm, args, 0, p, "fs.is_file")) return Value();
        std::error_code ec;
        return Value(fs::is_regular_file(p, ec) && !ec);
    }

    Value fs_is_dir(VM& vm, ArgView args) {
        std::string p;
        if (!fs_string(vm, args, 0, p, "fs.is_dir")) return Value();
        std::error_code ec;
        return Value(fs::is_directory(p, ec) && !ec);
    }

    Value fs_remove(VM& vm, ArgView args) {
        std::string p;
        if (!fs_string(vm, args, 0, p, "fs.remove")) return Value();
        std::error_code ec;
        bool result = fs::remove(p, ec);
        return Value(result && !ec);
    }

    Value fs_remove_all(VM& vm, ArgView args) {
        std::string p;
        if (!fs_string(vm, args, 0, p, "fs.remove_all")) return Value();
        std::error_code ec;
        std::uintmax_t n = fs::remove_all(p, ec);
        return Value(n != static_cast<std::uintmax_t>(-1) && !ec);
    }

    Value fs_mkdir(VM& vm, ArgView args) {
        std::string p;
        if (!fs_string(vm, args, 0, p, "fs.mkdir")) return Value();
        std::error_code ec;
        fs::create_directories(p, ec);
        return Value(fs::is_directory(p) && !ec);
    }

    Value fs_listdir(VM& vm, ArgView args) {
        std::string p;
        if (!fs_string(vm, args, 0, p, "fs.listdir")) return Value();
        std::error_code ec;
        if (!fs::is_directory(p, ec) || ec) {
            vm.runtime_error(RuntimeError::ArgumentError, "fs.listdir path is not a directory.");
            return Value();
        }
        ArrayType names;
        for (const auto& entry : fs::directory_iterator(p, ec)) {
            if (ec) break;
            names.push_back(to_value(vm, entry.path().filename().string()));
        }
        return Value(Value::Tag::ArrayRef, vm.alloc(Object(std::move(names))));
    }

    Value fs_walk(VM& vm, ArgView args) {
        std::string p;
        if (!fs_string(vm, args, 0, p, "fs.walk")) return Value();
        std::error_code ec;
        if (!fs::is_directory(p, ec) || ec) {
            vm.runtime_error(RuntimeError::ArgumentError, "fs.walk path is not a directory.");
            return Value();
        }
        fs::path root(p);
        ArrayType out;
        for (auto it = fs::recursive_directory_iterator(root, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
            if (ec) break;
            out.push_back(to_value(vm, it->path().lexically_relative(root).string()));
        }
        return Value(Value::Tag::ArrayRef, vm.alloc(Object(std::move(out))));
    }

    Value fs_glob(VM& vm, ArgView args) {
        std::string pattern;
        if (!fs_string(vm, args, 0, pattern, "fs.glob")) return Value();
        fs::path pat(pattern);
        fs::path dir = pat.parent_path();
        std::string namepat = pat.filename().string();
        if (dir.empty()) dir = ".";
        std::error_code ec;
        if (!fs::is_directory(dir, ec) || ec) return Value(Value::Tag::ArrayRef, vm.alloc(Object(ArrayType{})));
        ArrayType out;
        for (const auto& entry : fs::directory_iterator(dir, ec)) {
            if (ec) break;
            std::string name = entry.path().filename().string();
            if (wildcard_match(name, namepat)) {
                out.push_back(to_value(vm, (dir / name).string()));
            }
        }
        return Value(Value::Tag::ArrayRef, vm.alloc(Object(std::move(out))));
    }

    Value fs_rename(VM& vm, ArgView args) {
        std::string src, dst;
        if (!fs_string(vm, args, 0, src, "fs.rename") || !fs_string(vm, args, 1, dst, "fs.rename")) return Value();
        std::error_code ec;
        fs::rename(src, dst, ec);
        return Value(!ec);
    }

    Value fs_copy(VM& vm, ArgView args) {
        std::string src, dst;
        if (!fs_string(vm, args, 0, src, "fs.copy") || !fs_string(vm, args, 1, dst, "fs.copy")) return Value();
        std::error_code ec;
        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
        return Value(!ec);
    }

    Value fs_file_size(VM& vm, ArgView args) {
        std::string p;
        if (!fs_string(vm, args, 0, p, "fs.file_size")) return Value();
        std::error_code ec;
        std::uintmax_t n = fs::file_size(p, ec);
        if (ec) return Value();
        return Value(static_cast<int64_t>(n));
    }

    Value fs_modified(VM& vm, ArgView args) {
        std::string p;
        if (!fs_string(vm, args, 0, p, "fs.modified")) return Value();
        std::error_code ec;
        auto ftime = fs::last_write_time(p, ec);
        if (ec) return Value();
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        return Value(static_cast<int64_t>(std::chrono::system_clock::to_time_t(sctp)));
    }

    Value fs_read_file(VM& vm, ArgView args) {
        std::string p;
        if (!fs_string(vm, args, 0, p, "fs.read_file")) return Value();
        std::ifstream file(p, std::ios::in | std::ios::binary);
        if (!file) return Value();
        return to_value(vm, read_stream_all(file));
    }

    Value fs_write_file(VM& vm, ArgView args) {
        std::string p, data;
        if (!fs_string(vm, args, 0, p, "fs.write_file") || !fs_string(vm, args, 1, data, "fs.write_file")) return Value();
        std::ofstream file(p, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file) return Value(false);
        file.write(data.data(), static_cast<std::streamsize>(data.size()));
        return Value(file.good());
    }

    Value fs_append_file(VM& vm, ArgView args) {
        std::string p, data;
        if (!fs_string(vm, args, 0, p, "fs.append_file") || !fs_string(vm, args, 1, data, "fs.append_file")) return Value();
        std::ofstream file(p, std::ios::out | std::ios::binary | std::ios::app);
        if (!file) return Value(false);
        file.write(data.data(), static_cast<std::streamsize>(data.size()));
        return Value(file.good());
    }

    Value fs_module_factory(VM& vm) {
        NativeModule mod(vm, "fs");
        mod.raw_function("exists", fs_exists)
           .raw_function("is_file", fs_is_file)
           .raw_function("is_dir", fs_is_dir)
           .raw_function("remove", fs_remove)
           .raw_function("remove_all", fs_remove_all)
           .raw_function("mkdir", fs_mkdir)
           .raw_function("listdir", fs_listdir)
           .raw_function("walk", fs_walk)
           .raw_function("glob", fs_glob)
           .raw_function("rename", fs_rename)
           .raw_function("copy", fs_copy)
           .raw_function("file_size", fs_file_size)
           .raw_function("modified", fs_modified)
           .raw_function("read_file", fs_read_file)
           .raw_function("write_file", fs_write_file)
           .raw_function("append_file", fs_append_file);
        return mod.build();
    }

}
