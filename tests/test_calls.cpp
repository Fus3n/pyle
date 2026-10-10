#include "test.hpp"

TEST(call_arities_cpp) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn k0() { return 7 }
        fn k1(a) { return a * 2 }
        fn k2(a, b) { return a + b }
        fn k3(a, b, c) { return a * b + c }
        fn k4(a, b, c, d) { return a + b + c + d }
    )");
    CHECK_CALM(t);
    CHECK(pyle::from_value<int64_t>(t.p.vm, t.p.vm.call_func(t.p.vm.get_global("k0"))) == 7);
    CHECK(pyle::from_value<int64_t>(t.p.vm, t.p.vm.call_func(t.p.vm.get_global("k1"), 21)) == 42);
    CHECK(pyle::from_value<std::string>(t.p.vm, t.p.vm.call_func(t.p.vm.get_global("k2"), std::string("a"), std::string("b"))) == "ab");
    CHECK(pyle::from_value<int64_t>(t.p.vm, t.p.vm.call_func(t.p.vm.get_global("k3"), 3, 4, 5)) == 17);
    CHECK(pyle::from_value<int64_t>(t.p.vm, t.p.vm.call_func(t.p.vm.get_global("k4"), 1, 2, 3, 4)) == 10);
    CHECK(!t.p.vm.is_panicked());
}

TEST(call_paths_cpp) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn sq(x) { return x * x }
    )");
    CHECK_CALM(t);
    pyle::Value f = t.p.vm.get_global("sq");
    CHECK(pyle::from_value<int64_t>(t.p.vm, t.p.vm.call_func1(f, pyle::Value(int64_t(9)))) == 81);
    std::vector<pyle::Value> args = {pyle::Value(int64_t(8))};
    CHECK(pyle::from_value<int64_t>(t.p.vm, t.p.vm.call_func_raw(f, args)) == 64);
    CHECK(!t.p.vm.is_panicked());
}

TEST(closures_upvalues) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn make_counter() {
            let x = 0
            fn count() {
                x += 1
                return x
            }
            return count
        }
        let c1 = make_counter()
        let c2 = make_counter()
        let a = c1()
        let b = c1()
        let c = c2()
        fn outer() {
            let y = 10
            fn mid() {
                let z = 20
                fn inner() { return y + z }
                return inner()
            }
            return mid()
        }
        let n = outer()
        let adder = fn(x) => fn(y) => x + y
        let add5 = adder(5)
        let s = add5(3)
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "a", "1");
    CHECK_STR(t, "b", "2");
    CHECK_STR(t, "c", "1");
    CHECK_STR(t, "n", "30");
    CHECK_STR(t, "s", "8");
}

TEST(loop_capture_freezes) {
    Ctx t;
    CHECK_RUN(t, R"(
        let fns = []
        for x in [1, 2, 3] { fns.append(fn() { return x }) }
        let a = fns[0]()
        let b = fns[1]()
        let c = fns[2]()
        let wns = []
        let i = 0
        while i < 2 {
            let y = i * 10
            wns.append(fn() { return y })
            i += 1
        }
        let w0 = wns[0]()
        let w1 = wns[1]()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "a", "1");
    CHECK_STR(t, "b", "2");
    CHECK_STR(t, "c", "3");
    CHECK_STR(t, "w0", "0");
    CHECK_STR(t, "w1", "10");
}

TEST(block_capture_freezes) {
    Ctx t;
    CHECK_RUN(t, R"(
        let fns = []
        {
            let y = 41
            fns.append(fn() { return y })
        }
        let v = fns[0]()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "v", "41");
}

TEST(outer_var_stays_live) {
    Ctx t;
    CHECK_RUN(t, R"(
        let x = 1
        let get = fn() { return x }
        x = 10
        let live = get()
        {
            let y = 2
        }
        let still = get()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "live", "10");
    CHECK_STR(t, "still", "10");
}

TEST(closure_mutation_ref) {
    Ctx t;
    CHECK_RUN(t, R"(
        let total = 0
        fn bump(n) { total += n }
        bump(5)
        bump(7)
        let fns = [fn() => 1, fn() => 2, fn() => 3]
        let fsum = fns[0]() + fns[1]() + fns[2]()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "total", "12");
    CHECK_STR(t, "fsum", "6");
}

TEST(call_arity_mismatch) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn one(x) { return x }
        let r = one(1, 2)
    )");
    CHECK(t.panicked());
}

TEST(call_non_callable) {
    Ctx t;
    CHECK_RUN(t, R"(
        let f = 42
        let r = f()
    )");
    CHECK(t.panicked());
}

TEST(break_in_callback_rejected) {
    Ctx t;
    CHECK(!t.run(R"(
        let a = [1, 2, 3]
        let r = a.map(fn(x) { if x == 2 { break } return x })
    )"));
}
