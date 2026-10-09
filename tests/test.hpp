#pragma once
#include <functional>
#include <iostream>
#include <string>
#include <vector>
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
