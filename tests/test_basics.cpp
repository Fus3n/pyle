#include "test.hpp"

TEST(arith_int) {
    Ctx t;
    CHECK_RUN(t, R"(
        let a = 2 + 3 * 4
        let b = (2 + 3) * 4
        let c = 10 % 3
        let d = 7 - 10
        let e = -d * 2
        let f = 0xFF
        let g = 10
        g += 5
        g *= 2
        g -= 4
        let h = 17 / 2
        let h2 = 17.0 / 2
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "a", "14");
    CHECK_STR(t, "b", "20");
    CHECK_STR(t, "c", "1");
    CHECK_STR(t, "d", "-3");
    CHECK_STR(t, "e", "6");
    CHECK_STR(t, "f", "255");
    CHECK_STR(t, "g", "26");
    CHECK_STR(t, "h", "8");
    CHECK_STR(t, "h2", "8.5");
}

TEST(arith_float_conv) {
    Ctx t;
    CHECK_RUN(t, R"(
        let m = import("math")
        let a = 1.5 + 2.25
        let b = m.floor(3.7)
        let c = m.trunc(-3.7)
        let d = m.round(2.5)
        let e = "42".to_num()
        let f = "3.5".to_num()
        let g = "nope".to_num()
        let h = m.sqrt(144)
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "a", "3.75");
    CHECK_STR(t, "b", "3");
    CHECK_STR(t, "c", "-3");
    CHECK_STR(t, "d", "3");
    CHECK_STR(t, "e", "42");
    CHECK_STR(t, "f", "3.5");
    CHECK_STR(t, "g", "none");
    CHECK_STR(t, "h", "12");
}

TEST(compare_logic) {
    Ctx t;
    CHECK_RUN(t, R"(
        let e1 = 1 == 1
        let e2 = 1 != 2
        let e3 = "a" == "a"
        let e3b = "a" != "b"
        let e4 = 5 >= 5
        let o1 = 0 or 5
        let o2 = 0 and 5
        let n1 = not false
        let t1b = true
        if 0 { t1b = false }
        let t1c = false
        if 1 { t1c = true }
        let t2 = true
        if none { t2 = false }
        if [] { } else { t2 = false }
        let s1 = true and false or true
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "e1", "true");
    CHECK_STR(t, "e2", "true");
    CHECK_STR(t, "e3", "true");
    CHECK_STR(t, "e3b", "true");
    CHECK_STR(t, "e4", "true");
    CHECK_STR(t, "o1", "5");
    CHECK_STR(t, "o2", "0");
    CHECK_STR(t, "n1", "true");
    CHECK_STR(t, "t1b", "true");
    CHECK_STR(t, "t1c", "true");
    CHECK_STR(t, "t2", "true");
    CHECK_STR(t, "s1", "true");
}

TEST(control_flow) {
    Ctx t;
    CHECK_RUN(t, R"(
        let grade = ""
        let score = 75
        if score > 90 { grade = "a" }
        elif score > 60 { grade = "b" }
        else { grade = "c" }
        let w = 0
        while w < 10 { w += 1 }
        let rs = 0
        for i in 1..11 { rs += i }
        let asum = 0
        for x in [1, 2, 3, 4] { asum += x }
        let chars = []
        for c in "hey" { chars.append(c) }
        let m = {a: 1, b: 2}
        let kv = 0
        for k in m.keys() { kv += m[k] }
        let li = 0
        loop { li += 1; if li >= 3 { break } }
        let sk = 0
        let ci = 0
        while ci < 10 {
            ci += 1
            if ci % 2 == 0 { continue }
            sk += ci
        }
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "grade", "b");
    CHECK_STR(t, "w", "10");
    CHECK_STR(t, "rs", "55");
    CHECK_STR(t, "asum", "10");
    CHECK_STR(t, "chars", "[h, e, y]");
    CHECK_STR(t, "kv", "3");
    CHECK_STR(t, "li", "3");
    CHECK_STR(t, "sk", "25");
}

TEST(functions) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn add(a, b) { return a + b }
        fn double(x) => x * 2
        fn noreturn(x) { let y = x }
        fn fib(n) {
            if n <= 1 { return n }
            return fib(n - 1) + fib(n - 2)
        }
        fn early(n) {
            if n < 0 { return -1 }
            return n
        }
        let r1 = add(3, 4)
        let r2 = double(21)
        let r3 = noreturn(9)
        let r4 = fib(15)
        let r5 = early(-5)
        let r6 = early(5)
        let t1 = typeof(1)
        let t2 = typeof(1.5)
        let t3 = typeof("s")
        let t4 = typeof([1])
        let t5 = typeof({a: 1})
        let t6 = typeof(none)
        let t7 = typeof(true)
        let t8 = typeof(add)
        let t9 = typeof(1..3)
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "r1", "7");
    CHECK_STR(t, "r2", "42");
    CHECK_STR(t, "r3", "none");
    CHECK_STR(t, "r4", "610");
    CHECK_STR(t, "r5", "-1");
    CHECK_STR(t, "r6", "5");
    CHECK_STR(t, "t1", "int");
    CHECK_STR(t, "t2", "float");
    CHECK_STR(t, "t3", "string");
    CHECK_STR(t, "t4", "array");
    CHECK_STR(t, "t5", "map");
    CHECK_STR(t, "t6", "none");
    CHECK_STR(t, "t7", "bool");
    CHECK_STR(t, "t8", "function");
    CHECK_STR(t, "t9", "range");
}
