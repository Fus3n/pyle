#include "test.hpp"

TEST(gc_pressure_survival) {
    Ctx t;
    CHECK_RUN(t, R"(
        let keep = []
        let i = 0
        while i < 3000 {
            keep.append({id: i, tag: "k"})
            i += 1
        }
        let junk = 0
        while junk < 20000 {
            let tmp = [junk, junk + 1, "trash"]
            junk += 1
        }
        let n = keep.size()
        let first = keep[0]["id"]
        let last = keep[keep.size() - 1]["id"]
        let spot = keep[1500]["id"]
        let tag = keep[1500]["tag"]
        fn make() {
            let x = 0
            return fn() { x += 1; return x }
        }
        let c = make()
        let w = 0
        while w < 5000 {
            let tmp2 = {a: w, b: [w]}
            w += 1
        }
        let v1 = c()
        let v2 = c()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "n", "3000");
    CHECK_STR(t, "first", "0");
    CHECK_STR(t, "last", "2999");
    CHECK_STR(t, "spot", "1500");
    CHECK_STR(t, "tag", "k");
    CHECK_STR(t, "v1", "1");
    CHECK_STR(t, "v2", "2");
}

TEST(gc_cycles_collected) {
    Ctx t;
    CHECK_RUN(t, R"(
        let a = []
        let m = {list: a, name: "cyc"}
        a.append(m)
        a.append(a)
        let n = a.size()
        let back = a[0]["list"].size()
        let selfn = a[1].size()
        let nm = a[0]["name"]
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "n", "2");
    CHECK_STR(t, "back", "2");
    CHECK_STR(t, "selfn", "2");
    CHECK_STR(t, "nm", "cyc");
    t.p.vm.gc_collect_now();
    CHECK_CALM(t);
    CHECK_STR(t, "n", "2");
    CHECK_STR(t, "back", "2");
    CHECK_RUN(t, R"(
        a = none
        let w = 0
        while w < 10000 {
            let tmp = {k: [w, w]}
            w += 1
        }
        let alive = true
    )");
    CHECK_CALM(t);
    t.p.vm.gc_collect_now();
    CHECK_CALM(t);
    CHECK_STR(t, "alive", "true");
}

TEST(gcroot_cpp_keeps_alive) {
    Ctx t;
    CHECK_RUN(t, "let arr = [10, 20, 30]");
    CHECK_CALM(t);
    pyle::Value v = t.p.vm.get_global("arr");
    CHECK(v.tag == pyle::Value::Tag::ArrayRef);
    {
        pyle::GCRoot guard(t.p.vm, v.as_ref, pyle::Value::Tag::ArrayRef);
        CHECK_RUN(t, R"(
            arr = none
            let w = 0
            while w < 20000 {
                let tmp = [w, w + 1, w + 2]
                w += 1
            }
        )");
        CHECK_CALM(t);
        t.p.vm.gc_collect_now();
        CHECK_CALM(t);
        const auto& vec = t.p.vm.get_heap_object<pyle::ArrayType>(v.as_ref);
        CHECK(vec.size() == 3);
        CHECK(vec[0].as_int == 10);
        CHECK(vec[2].as_int == 30);
    }
}

struct TestAcc {
    int64_t total = 0;
    TestAcc() = default;
    void add(int64_t n) { total += n; }
    int64_t sum() const { return total; }
};

TEST(cpp_bound_class) {
    Ctx t;
    pyle::ClassBinder<TestAcc>(t.p.vm, "TestAcc")
        .constructor<>()
        .member<int64_t, &TestAcc::total>("total")
        .method<&TestAcc::add>("add")
        .method<&TestAcc::sum>("sum")
        .register_globally();
    CHECK_RUN(t, R"(
        let a = TestAcc()
        a.add(5)
        a.add(7)
        let s = a.sum()
        let m = a.total
        let w = 0
        while w < 5000 {
            let tmp = [w]
            w += 1
        }
        let s2 = a.sum()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "s", "12");
    CHECK_STR(t, "m", "12");
    CHECK_STR(t, "s2", "12");
}

TEST(string_interning) {
    Ctx t;
    CHECK_RUN(t, R"(
        let a = "hello"
        let b = "hello"
        let eq = a == b
        let c = "he" + "llo"
        let eq2 = a == c
        let m = {}
        m[a] = 1
        let hit = m[b]
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "eq", "true");
    CHECK_STR(t, "eq2", "true");
    CHECK_STR(t, "hit", "1");
}

TEST(coroutines) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn task() {
            yield 1
            yield 2
            return 3
        }
        let c = Coro(task)
        let s0 = c.state()
        let r1 = c.resume()
        let s1 = c.state()
        let r2 = c.resume()
        let r3 = c.resume()
        let s3 = c.state()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "s0", "suspended");
    CHECK_STR(t, "r1", "1");
    CHECK_STR(t, "s1", "suspended");
    CHECK_STR(t, "r2", "2");
    CHECK_STR(t, "r3", "3");
    CHECK_STR(t, "s3", "dead");
}

TEST(native_imports) {
    Ctx t;
    CHECK_RUN(t, R"(
        let m = import("math")
        let r = m.sqrt(144)
        let pi_ok = m.PI > 3.14 and m.PI < 3.15
        let pw = m.pow(2, 10)
        let os = import("os")
        let cores_ok = os.cpu_count() >= 1
        let tm_ok = os.time() > 0
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "r", "12");
    CHECK_STR(t, "pi_ok", "true");
    CHECK_STR(t, "pw", "1024");
    CHECK_STR(t, "cores_ok", "true");
    CHECK_STR(t, "tm_ok", "true");
}

static int64_t triple_it(int64_t x) { return x * 3; }

TEST(cpp_bind_call) {
    Ctx t;
    pyle::bind_function<&triple_it>(t.p.vm, "triple_it");
    CHECK_RUN(t, R"(
        let out = triple_it(14)
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "out", "42");
}

TEST(syntax_error_fails) {
    Ctx t;
    CHECK(!t.run("let x = "));
    CHECK(!t.run("fn broken( { }"));
}

TEST(compile_errors_fail) {
    Ctx t;
    CHECK(!t.run("break"));
    CHECK(!t.run("let x = 1\nx = "));
}

TEST(string_oob) {
    Ctx t;
    CHECK_RUN(t, R"(
        let s = "hi"
        let c = s[9]
    )");
    CHECK(t.panicked());
}

TEST(enum_desugar) {
    Ctx t;
    CHECK_RUN(t, R"(
        enum Direction { NORTH, SOUTH, EAST, WEST }
        let n = Direction.NORTH
        let w = Direction.WEST
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "n", "0");
    CHECK_STR(t, "w", "3");
}
