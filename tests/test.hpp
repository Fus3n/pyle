#pragma once
#include <functional>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <io.h>
#include <fcntl.h>
#include "pyle/pyle.hpp"
#include "pyle/std/std_core.hpp"
#include "pyle/binder.hpp"

struct TestCase {
    const char* name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& test_registry() {
    static std::vector<TestCase> r;
    return r;
}

struct TestRegistrar {
    TestRegistrar(const char* n, std::function<void()> fn) { test_registry().push_back({n, fn}); }
};

#define TEST(name) \
    static void test_body_##name(); \
    static TestRegistrar test_reg_##name(#name, test_body_##name); \
    static void test_body_##name()

inline int g_checks = 0;
inline int g_check_failures = 0;
inline int g_tests = 0;
inline int g_failed_tests = 0;
inline bool g_current_failed = false;

#define CHECK(cond) \
    do { \
        ++g_checks; \
        if (!(cond)) { \
            ++g_check_failures; \
            g_current_failed = true; \
            std::cout << "    FAIL " << __FILE__ << ":" << __LINE__ << ": " << #cond << "\n"; \
        } \
    } while (0)

struct StderrCap {
    int saved = -1;
    const char* path = "test_stderr_capture.tmp";

    void start() {
        fflush(stderr);
        saved = _dup(2);
        int tmp = _open(path, _O_WRONLY | _O_CREAT | _O_TRUNC, 0600);
        if (tmp < 0) return;
        _dup2(tmp, 2);
        _close(tmp);
    }

    std::string stop() {
        fflush(stderr);
        if (saved >= 0) {
            _dup2(saved, 2);
            _close(saved);
            saved = -1;
        }
        std::string s;
        {
            std::ifstream in(path, std::ios::binary);
            s.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        }
        std::remove(path);
        return s;
    }
};

inline int count_in(const std::string& haystack, const std::string& needle) {
    int n = 0;
    size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos) {
        ++n;
        pos += needle.size();
    }
    return n;
}

struct Ctx {
    pyle::Pyle p;

    Ctx() { pyle::register_core_natives(p.vm); }

    bool run(const std::string& src) { return p.execute(src, false, "test.pyl"); }

    bool panicked() { return p.vm.is_panicked(); }

    std::string get(const std::string& name) {
        return p.vm.value_to_string(p.vm.get_global(name));
    }
};

#define CHECK_RUN(ctx, src) CHECK((ctx).run(src))
#define CHECK_CALM(ctx) CHECK(!(ctx).panicked())
#define CHECK_STR(ctx, name, expected) CHECK((ctx).get(name) == (expected))
