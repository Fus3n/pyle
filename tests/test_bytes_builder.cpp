#include "test.hpp"

TEST(bytes_builder_basic) {
    Ctx t;
    CHECK_RUN(t, R"(
        let b = BytesBuilder()
        b.push(104)
        b.push(105)
        let sz = b.size()
        let out = b.to_bytes()
        let s = out.to_string()
        let empty_sz = b.size()
        let out2 = b.to_bytes()
        let out2sz = out2.size()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "sz", "2");
    CHECK_STR(t, "s", "hi");
    CHECK_STR(t, "empty_sz", "0");
    CHECK_STR(t, "out2sz", "0");
}

TEST(bytes_builder_extend_array) {
    Ctx t;
    CHECK_RUN(t, R"(
        let b = BytesBuilder(4)
        b.extend_array([65, 66, 67, 68, 69, 70])
        let grow = b.reserve(1024)
        b.push(71)
        let out = b.to_bytes()
        let total = 0
        for byte in out { total += byte }
        let sz = out.size()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "sz", "7");
    CHECK_STR(t, "total", "476");
    CHECK_STR(t, "grow", "1024");
}

TEST(bytes_builder_to_string) {
    Ctx t;
    CHECK_RUN(t, R"(
        let b = BytesBuilder()
        b.push(72)
        b.extend_array([101, 108, 108, 111])
        let s = b.to_string()
        let sz = b.size()
        let out = b.to_bytes()
        let outs = out.to_string()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "s", "Hello");
    CHECK_STR(t, "sz", "5");
    CHECK_STR(t, "outs", "Hello");
}

TEST(bytes_builder_invalid_push) {
    Ctx t;
    CHECK_RUN(t, R"(
        let b = BytesBuilder()
        b.push(256)
    )");
    CHECK(t.panicked());
    Ctx t2;
    CHECK_RUN(t2, R"(
        let b = BytesBuilder()
        b.push(-1)
    )");
    CHECK(t2.panicked());
}

TEST(bytes_builder_invalid_type) {
    Ctx t;
    CHECK_RUN(t, R"(
        let b = BytesBuilder()
        b.push("nope")
    )");
    CHECK(t.panicked());
    Ctx t2;
    CHECK_RUN(t2, R"(
        let b = BytesBuilder()
        b.extend_array([65, 1.5])
    )");
    CHECK(t2.panicked());
}

TEST(bytes_builder_hashable_exclusion) {
    Ctx t;
    CHECK_RUN(t, R"(
        let b = BytesBuilder()
        let m = {}
        m[b] = 1
    )");
    CHECK(t.panicked());
}
