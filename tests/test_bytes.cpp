#include "test.hpp"

TEST(bytes_read_methods) {
    Ctx t;
    CHECK_RUN(t, R"(
        let le = Bytes([1, 0, 0, 0, 0, 0, 0, 0])
        let be = Bytes([0, 0, 0, 0, 0, 0, 0, 1])
        let li = le.to_int_le()
        let bi = le.to_int_be()
        let li2 = be.to_int_be()
        let bi2 = be.to_int_le()
        let f_le = Bytes([0, 0, 0, 0, 0, 0, 240, 63])
        let f_be = Bytes([63, 240, 0, 0, 0, 0, 0, 0])
        let f1 = f_le.to_float_le()
        let f2 = f_be.to_float_be()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "li", "1");
    CHECK_STR(t, "bi", "72057594037927936");
    CHECK_STR(t, "li2", "1");
    CHECK_STR(t, "bi2", "72057594037927936");
    CHECK_STR(t, "f1", "1");
    CHECK_STR(t, "f2", "1");
}

TEST(bytes_read_methods_short) {
    Ctx t;
    CHECK_RUN(t, R"(
        let short = Bytes([1, 2, 3])
        let x = short.to_int_le()
    )");
    CHECK(t.panicked());
}
