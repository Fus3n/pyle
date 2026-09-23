#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <httplib.h>
#undef TRUE
#undef FALSE
#undef ERROR
#undef DELETE
#undef IN
#undef OUT
#else
#include <httplib.h>
#endif
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <cctype>
#include <thread>
#include <mutex>
#include <deque>
#include <future>
#include <functional>
#include <optional>
#include "pyle/binder.hpp"
#include "pyle/std/std_future.hpp"

namespace pyle {
namespace http_binding {

    struct GcDisable {
        pyle::VM& vm;
        bool was;
        explicit GcDisable(pyle::VM& v) : vm(v), was(v.is_gc_enabled()) { vm.set_gc_enabled(false); }
        ~GcDisable() { vm.set_gc_enabled(was); }
    };

    struct HttpResponseData {
        int64_t status = 0;
        std::string body;
        std::map<std::string, std::string> headers;
    };

    void register_client(NativeModule& mod);
    void register_server(NativeModule& mod);

}
}
