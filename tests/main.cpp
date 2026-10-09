#include "test.hpp"

int main() {
    for (auto& t : test_registry()) {
        ++g_tests;
        g_current_failed = false;
        try {
            t.fn();
        } catch (const std::exception& e) {
            g_current_failed = true;
            std::cout << "    FAIL exception: " << e.what() << "\n";
        } catch (...) {
            g_current_failed = true;
            std::cout << "    FAIL unknown exception\n";
        }
        if (g_current_failed) {
            ++g_failed_tests;
            std::cout << "[FAIL] " << t.name << "\n";
        } else {
            std::cout << "[ok] " << t.name << "\n";
        }
    }
    std::cout << "tests: " << g_tests << ", checks: " << g_checks
              << ", failed tests: " << g_failed_tests
              << ", failed checks: " << g_check_failures << "\n";
    return g_failed_tests == 0 ? 0 : 1;
}
