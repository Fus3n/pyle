#include "test.hpp"

TEST(async_all_collects) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn good() { return 1 }
        fn bad() { return [1][9] }
        fn main() {
            return async.all([good, bad, good])
        }
        let res = async.run(main)
        let o0 = res[0]["ok"]
        let v0 = res[0]["value"]
        let o1 = res[1]["ok"]
        let ty1 = res[1]["error"]["type"]
        let o2 = res[2]["ok"]
        let v2 = res[2]["value"]
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "o0", "true");
    CHECK_STR(t, "v0", "1");
    CHECK_STR(t, "o1", "false");
    CHECK_STR(t, "ty1", "IndexError");
    CHECK_STR(t, "o2", "true");
    CHECK_STR(t, "v2", "1");
}

TEST(async_all_coro_inputs) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn bad2() { return [1][5] }
        fn ok2() { return 7 }
        let c1 = Coro(bad2)
        let c2 = Coro(ok2)
        fn main() {
            return async.all([c1, c2])
        }
        let res = async.run(main)
        let o0 = res[0]["ok"]
        let ty0 = res[0]["error"]["type"]
        let o1 = res[1]["ok"]
        let v1 = res[1]["value"]
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "o0", "false");
    CHECK_STR(t, "ty0", "IndexError");
    CHECK_STR(t, "o1", "true");
    CHECK_STR(t, "v1", "7");
}

TEST(async_run_reraises) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn bad() { return [1][9] }
        let r = pcall(fn() { return async.run(bad) })
        let ok = r.ok
        let ty = r.error.type
        let last = r.error.trace[r.error.trace.size() - 1]["function"]
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ok", "false");
    CHECK_STR(t, "ty", "IndexError");
    CHECK_STR(t, "last", "bad");
}

TEST(async_sleep_tasks) {
    Ctx t;
    CHECK_RUN(t, R"(
        let os = import("os")
        fn s1() { waitfor(os.sleep_async(5)); return 1 }
        fn s2() { waitfor(os.sleep_async(5)); return 2 }
        fn main() { return async.all([s1, s2]) }
        let out = async.run(main)
        let v0 = out[0]["value"]
        let v1 = out[1]["value"]
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "v0", "1");
    CHECK_STR(t, "v1", "2");
}

TEST(waitfor_reject_raises) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn main() {
            let ft = Future()
            ft.reject("nope")
            return pcall(fn() { return waitfor(ft) })
        }
        let r = async.run(main)
        let ok = r.ok
        let ty = r.error.type
        let mg = r.error.message
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ok", "false");
    CHECK_STR(t, "ty", "FutureRejected");
    CHECK_STR(t, "mg", "nope");
}

TEST(pcall_native_fn) {
    Ctx t;
    CHECK_RUN(t, R"(
        let m = import("math")
        let r = pcall(m.sqrt, 144)
        let ok = r.ok
        let v = r.value
        let r2 = pcall(m.sqrt)
        let ok2 = r2.ok
        let ty2 = r2.error.type
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ok", "true");
    CHECK_STR(t, "v", "12");
    CHECK_STR(t, "ok2", "false");
    CHECK_STR(t, "ty2", "ArgumentError");
}

TEST(raise_roundtrip) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn boom() { return [1][9] }
        let inner = pcall(boom)
        let outer = pcall(fn() { raise(inner.error) })
        let ok = outer.ok
        let ty = outer.error.type
        let last = outer.error.trace[outer.error.trace.size() - 1]["function"]
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ok", "false");
    CHECK_STR(t, "ty", "IndexError");
    CHECK_STR(t, "last", "boom");
}

TEST(raise_string) {
    Ctx t;
    CHECK_RUN(t, R"(
        let r = pcall(fn() { raise("boom") })
        let ty = r.error.type
        let mg = r.error.message
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ty", "Error");
    CHECK_STR(t, "mg", "boom");
}

TEST(raise_garbage) {
    Ctx t;
    CHECK_RUN(t, R"(
        let r = pcall(fn() { raise(42) })
        let ty = r.error.type
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "ty", "TypeError");
}

TEST(async_all_top_level_returns) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn good() { return 1 }
        fn bad() { return [1][9] }
        let res = async.all([good, bad, good])
        let o0 = res[0]["ok"]
        let o1 = res[1]["ok"]
        let ty1 = res[1]["error"]["type"]
        let o2 = res[2]["ok"]
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "o0", "true");
    CHECK_STR(t, "o1", "false");
    CHECK_STR(t, "ty1", "IndexError");
    CHECK_STR(t, "o2", "true");
}

TEST(async_run_top_level_reports_once) {
    Ctx t;
    StderrCap cap;
    cap.start();
    bool ok = t.run(R"(
        fn bad() { return [1][9] }
        async.run(bad)
    )");
    std::string out = cap.stop();
    CHECK(ok);
    CHECK(t.panicked());
    CHECK(count_in(out, "IndexError:") == 1);
    CHECK(count_in(out, "Cannot yield from the root coro") == 0);
}
