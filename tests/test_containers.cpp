#include "test.hpp"

TEST(array_append_paths) {
    Ctx t;
    CHECK_RUN(t, R"(
        let local = []
        local.append(1)
        local.append(2)
        let n1 = local.size()
        let g = []
        fn fill() {
            g.append(10)
            g.append(20)
        }
        fill()
        let e = [0]
        let use_expr = e.append(99)
        let nested = [[1]]
        nested[0].append(2)
        let up = []
        fn add_up(v) { up.append(v) }
        add_up(5)
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "local", "[1, 2]");
    CHECK_STR(t, "n1", "2");
    CHECK_STR(t, "g", "[10, 20]");
    CHECK_STR(t, "e", "[0, 99]");
    CHECK_STR(t, "nested", "[[1, 2]]");
    CHECK_STR(t, "up", "[5]");
}

TEST(array_higher_order) {
    Ctx t;
    CHECK_RUN(t, R"(
        let base = [1, 2, 3, 4, 5]
        let doubled = base.map(fn(x) => x * 2)
        let evens = base.filter(fn(x) => x % 2 == 0)
        let chained = base.map(fn(x) => x + 1).filter(fn(x) => x % 2 == 0)
        let found = base.find(fn(x) => x > 3)
        let missing = base.find(fn(x) => x > 99)
        let dbl = fn(x) => x * 10
        let via_var = base.map(dbl)
        let cond = fn(x) => x < 3
        let via_var_f = base.filter(cond)
        let orig = base.size()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "doubled", "[2, 4, 6, 8, 10]");
    CHECK_STR(t, "evens", "[2, 4]");
    CHECK_STR(t, "chained", "[2, 4, 6]");
    CHECK_STR(t, "found", "4");
    CHECK_STR(t, "missing", "none");
    CHECK_STR(t, "via_var", "[10, 20, 30, 40, 50]");
    CHECK_STR(t, "via_var_f", "[1, 2]");
    CHECK_STR(t, "orig", "5");
}

TEST(array_mutators) {
    Ctx t;
    CHECK_RUN(t, R"(
        let a = [3, 1, 2]
        let p = a.pop()
        a.reverse()
        let sl = a.slice(0, 1)
        let r = a.slice(0..2)
        let idx = [10, 20, 30].index_of(20)
        let idxmiss = [10, 20].index_of(99)
        a[0] = 99
        let c = [1, 2]
        c.clear()
        let n = c.size()
        let cap = []
        cap.reserve(100)
        cap.append(1)
        let rz = [1]
        rz.resize(3, 0)
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "p", "2");
    CHECK_STR(t, "a", "[99, 3]");
    CHECK_STR(t, "sl", "[1]");
    CHECK_STR(t, "r", "[1, 3]");
    CHECK_STR(t, "idx", "1");
    CHECK_STR(t, "idxmiss", "none");
    CHECK_STR(t, "n", "0");
    CHECK_STR(t, "cap", "[1]");
    CHECK_STR(t, "rz", "[1, 0, 0]");
}

TEST(array_oob) {
    Ctx t;
    CHECK_RUN(t, R"(
        let a = [1, 2]
        let x = a[5]
    )");
    CHECK(t.panicked());
}

TEST(maps) {
    Ctx t;
    CHECK_RUN(t, R"(
        let m = {name: "amy", age: 30}
        let n = m["name"]
        let d = m.age
        m["age"] = 31
        m.city = "oslo"
        let has1 = m.has("name")
        let has2 = m.has("zzz")
        let missing_dot = m.zzz
        let sz = m.size()
        let ks = m.keys()
        let vs = m.values()
        let rm = m.remove("age")
        let sz2 = m.size()
        let comp = {5 + 5: "ten"}
        let ck = comp[10]
        m.clear()
        let sz3 = m.size()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "n", "amy");
    CHECK_STR(t, "d", "30");
    CHECK_STR(t, "has1", "true");
    CHECK_STR(t, "has2", "false");
    CHECK_STR(t, "missing_dot", "none");
    CHECK_STR(t, "sz", "3");
    CHECK_STR(t, "rm", "31");
    CHECK_STR(t, "sz2", "2");
    CHECK_STR(t, "ck", "ten");
    CHECK_STR(t, "sz3", "0");
    CHECK(pyle::from_value<std::vector<std::string>>(t.p.vm, t.p.vm.get_global("ks")).size() == 3);
    CHECK(pyle::from_value<std::vector<std::string>>(t.p.vm, t.p.vm.get_global("vs")).size() == 3);
}

TEST(map_missing_bracket) {
    Ctx t;
    CHECK_RUN(t, R"(
        let m = {a: 1}
        let x = m["zzz"]
    )");
    CHECK(t.panicked());
}

TEST(strings) {
    Ctx t;
    CHECK_RUN(t, R"(
        let s = "  hello world  "
        let tr = s.trim()
        let c1 = s.contains("lo wo")
        let c2 = s.contains("zzz")
        let sw = "hello".starts_with("he")
        let ew = "hello".ends_with("lo")
        let rp = "aaa".replace("a", "b")
        let sp = "a,b,c".split(",")
        let jn = "-".join(["x", "y"])
        let up = "abc".upper()
        let lo = "ABC".lower()
        let sl = "hello".slice(1, 4)
        let ch = "hello"[0]
        let cat = "a" + "b"
        let eq = "x" == "x"
        let dg = "123".is_digit()
        let al = "abc".is_alpha()
        let num = "12".to_num()
        let f = format("{} + {} = {}", 1, 2, 3)
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "tr", "hello world");
    CHECK_STR(t, "c1", "true");
    CHECK_STR(t, "c2", "false");
    CHECK_STR(t, "sw", "true");
    CHECK_STR(t, "ew", "true");
    CHECK_STR(t, "rp", "bbb");
    CHECK_STR(t, "sp", "[a, b, c]");
    CHECK_STR(t, "jn", "x-y");
    CHECK_STR(t, "up", "ABC");
    CHECK_STR(t, "lo", "abc");
    CHECK_STR(t, "sl", "ell");
    CHECK_STR(t, "ch", "h");
    CHECK_STR(t, "cat", "ab");
    CHECK_STR(t, "eq", "true");
    CHECK_STR(t, "dg", "true");
    CHECK_STR(t, "al", "true");
    CHECK_STR(t, "num", "12");
    CHECK_STR(t, "f", "1 + 2 = 3");
}

TEST(structs) {
    Ctx t;
    CHECK_RUN(t, R"(
        struct Pos(x, y) {}
        struct Player(name, health) {
            fn _init(n) {
                self.name = n
                self.health = 100
            }
            fn damage(n) { self.health -= n }
            fn status() { return format("{}:{}", self.name, self.health) }
            static fn spawn(n) { return Player(n) }
        }
        struct V(x) {
            fn plus(o) { return V(self.x + o.x) }
            fn eql(o) { return self.x == o.x }
        }
        let p = Player("amy")
        let st0 = p.status()
        p.damage(30)
        let st1 = p.status()
        let q = Player.spawn("bob")
        let v = V(2).plus(V(3))
        let vx = v.x
        let veq = V(5).eql(v)
        let vne = V(1).eql(v)
        let pos = Pos(x: 1, y: 2)
        let px = pos.x
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "st0", "amy:100");
    CHECK_STR(t, "st1", "amy:70");
    CHECK_STR(t, "q", "{name: bob, health: 100}");
    CHECK_STR(t, "vx", "5");
    CHECK_STR(t, "veq", "true");
    CHECK_STR(t, "vne", "false");
    CHECK_STR(t, "px", "1");
}

TEST(struct_fixed_shape) {
    Ctx t;
    CHECK_RUN(t, R"(
        struct P(x) {}
        let p = P(1)
        p.zzz = 2
    )");
    CHECK(t.panicked());
}
