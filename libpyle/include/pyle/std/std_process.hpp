#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace pyle {
class VM;
class NativeModule;
namespace proc {
int cpu_count();
bool worker_init_from_env(VM& vm);
void bind_to_os(VM& vm, NativeModule& mod);

#ifdef _WIN32
using RawHandle = void*;
#else
using RawHandle = int;
#endif
extern const RawHandle kBadHandle;
bool handle_valid(RawHandle h);
void handle_close(RawHandle h);
bool write_all(RawHandle h, const char* src, size_t n);
bool read_exact(RawHandle h, char* dst, size_t n);
bool await_readable(RawHandle h, int64_t timeout_ms);
long read_available(RawHandle h, char* dst, size_t n);
#ifdef _WIN32
std::string last_error_string(const char* what);
std::wstring widen(const std::string& s);
std::string quote_arg(const std::string& s);
#endif
}
}
