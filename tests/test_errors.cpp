#include "test.hpp"

TEST(pcall_success) {
    Ctx t;
    CHECK_RUN(t, R"(
        let r = pcall(fn(a, b) => a + b, 20, 22)
        let ok = r.ok
        let v = r.value
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ok", "true");
    CHECK_STR(t, "v", "42");
}

TEST(pcall_catches_shape) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn inner() { return [1][9] }
        fn outer() { return inner() + 1 }
        let r = pcall(fn() { return outer() })
        let ok = r.ok
        let ty = r.error.type
        let msg = r.error.message
        let n = r.error.trace.size()
        let last_fn = r.error.trace[r.error.trace.size() - 1]["function"]
        let last_file = r.error.trace[r.error.trace.size() - 1]["file"]
        let last_line = r.error.trace[r.error.trace.size() - 1]["line"] > 0
        let after = 6 * 7
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ok", "false");
    CHECK_STR(t, "ty", "IndexError");
    CHECK_STR(t, "msg", "Array index 9 out of bounds for size 1.");
    CHECK_STR(t, "last_fn", "inner");
    CHECK_STR(t, "last_file", "test.pyl");
    CHECK_STR(t, "last_line", "true");
    CHECK_STR(t, "after", "42");
}

TEST(pcall_nested) {
    Ctx t;
    CHECK_RUN(t, R"(
        let r = pcall(fn() {
            let inner = pcall(fn() { return [1][5] })
            return inner.ok
        })
        let outer_ok = r.ok
        let inner_ok = r.value
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "outer_ok", "true");
    CHECK_STR(t, "inner_ok", "false");
}

TEST(pcall_propagates_through_calls) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn boom() { return [1][5] }
        fn mid() { return boom() }
        let r = pcall(fn() { return mid() })
        let ok = r.ok
        let n = r.error.trace.size()
        let f0 = r.error.trace[0]["function"]
        let flast = r.error.trace[r.error.trace.size() - 1]["function"]
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ok", "false");
    CHECK_STR(t, "f0", "main");
    CHECK_STR(t, "flast", "boom");
}

TEST(pcall_native_callback_error) {
    Ctx t;
    CHECK_RUN(t, R"(
        let r = pcall(fn() { return [1, 2].map(fn(x) => [x][9]) })
        let ok = r.ok
        let ty = r.error.type
        let still = [3].map(fn(x) => x * 2)
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ok", "false");
    CHECK_STR(t, "ty", "IndexError");
    CHECK_STR(t, "still", "[6]");
}

TEST(pcall_bad_callee) {
    Ctx t;
    CHECK_RUN(t, R"(
        let r = pcall(42)
        let ok = r.ok
        let ty = r.error.type
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ok", "false");
    CHECK_STR(t, "ty", "TypeError");
}

TEST(pcall_arity_caught) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn one(x) { return x }
        let r = pcall(one, 1, 2)
        let ok = r.ok
        let ty = r.error.type
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ok", "false");
    CHECK_STR(t, "ty", "ArgumentError");
}

TEST(pcall_no_args_loud) {
    Ctx t;
    CHECK_RUN(t, "let r = pcall()");
    CHECK(t.panicked());
}

TEST(assert_pass_fail) {
    Ctx t;
    CHECK_RUN(t, R"(
        let v = assert(1 + 1 == 2, "math broke")
        let r = pcall(fn() { assert(false, "boom") })
        let ty = r.error.type
        let mg = r.error.message
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "v", "true");
    CHECK_STR(t, "ty", "AssertionError");
    CHECK_STR(t, "mg", "boom");
}

TEST(assert_uncaught_panics) {
    Ctx t;
    CHECK_RUN(t, R"(
        assert(1 > 2, "loud")
    )");
    CHECK(t.panicked());
}

TEST(print_trace_calm) {
    Ctx t;
    CHECK_RUN(t, R"(
        let r = pcall(fn() { return [1][9] })
        print_trace(r)
        print_trace(r.error)
        print_trace({})
        let s = pcall(fn() { return 1 })
        print_trace(s)
        let done = true
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "done", "true");
}

TEST(pcall_yield_transparent) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn task() {
            let a = pcall(fn() { yield 10; return 20 })
            yield a.value
            return 99
        }
        let c = Coro(task)
        let r1 = c.resume()
        let r2 = c.resume()
        let r3 = c.resume()
        let st = c.state()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "r1", "10");
    CHECK_STR(t, "r2", "20");
    CHECK_STR(t, "r3", "99");
    CHECK_STR(t, "st", "dead");
}

TEST(pcall_upvalue_after_catch) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn f() {
            let x = 41
            let get = fn() { return x }
            let r = pcall(fn() { x += 1; return [1][5] })
            return [r.ok, get()]
        }
        let v = f()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "v", "[false, 42]");
}

TEST(pcall_module_env_restored) {
    Ctx t;
    CHECK_RUN(t, R"(
        let m = import("math")
        let r = pcall(fn() { return m.nosuch() })
        let ok = r.ok
        let still = m.sqrt(81)
        let g = 6 * 7
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ok", "false");
    CHECK_STR(t, "still", "9");
    CHECK_STR(t, "g", "42");
}
