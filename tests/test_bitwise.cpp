#include "test.hpp"

TEST(bitwise_and_or_xor) {
    Ctx t;
    CHECK_RUN(t, R"(
        let a = 12 & 10
        let b = 12 | 10
        let c = 12 ^ 10
        let d = -1 & 0xFF
        let e = -6 | 1
        let f = -1 ^ -1
        let g = 0 & 12345
        let h = -1 | 0
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "a", "8");
    CHECK_STR(t, "b", "14");
    CHECK_STR(t, "c", "6");
    CHECK_STR(t, "d", "255");
    CHECK_STR(t, "e", "-5");
    CHECK_STR(t, "f", "0");
    CHECK_STR(t, "g", "0");
    CHECK_STR(t, "h", "-1");
}

TEST(bitwise_not) {
    Ctx t;
    CHECK_RUN(t, R"(
        let a = ~0
        let b = ~5
        let n = 9
        let c = ~n
        let m = -1
        let d = ~m
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "a", "-1");
    CHECK_STR(t, "b", "-6");
    CHECK_STR(t, "c", "-10");
    CHECK_STR(t, "d", "0");
}

TEST(bitwise_runtime_values) {
    Ctx t;
    CHECK_RUN(t, R"(
        let x = 12
        let y = 10
        let a = x & y
        let b = x | y
        let c = x ^ y
        let d = -1 & 0xFF
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "a", "8");
    CHECK_STR(t, "b", "14");
    CHECK_STR(t, "c", "6");
    CHECK_STR(t, "d", "255");
}

TEST(shift_values) {
    Ctx t;
    CHECK_RUN(t, R"(
        let a = 1 << 63
        let b = 1 << 62
        let c = -4 >> 1
        let d = 3 << 4
        let n = 63
        let e = 1 << n
        let k = 1
        let f = -4 >> k
        let g = 256 >> 4
        let sh = 6 * 2
        let h = 3 << sh
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "a", "-9223372036854775808");
    CHECK_STR(t, "b", "4611686018427387904");
    CHECK_STR(t, "c", "-2");
    CHECK_STR(t, "d", "48");
    CHECK_STR(t, "e", "-9223372036854775808");
    CHECK_STR(t, "f", "-2");
    CHECK_STR(t, "g", "16");
    CHECK_STR(t, "h", "12288");
}

TEST(shift_bounds) {
    Ctx t;
    CHECK_RUN(t, R"(
        let a = 1 << 64
        let b = 1 >> 64
        let c = -1 >> 64
        let k = 64
        let d = 1 << k
        let e = -8 >> k
        let f = 0 >> 1000
        let g = 123 << 200
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "a", "0");
    CHECK_STR(t, "b", "0");
    CHECK_STR(t, "c", "-1");
    CHECK_STR(t, "d", "0");
    CHECK_STR(t, "e", "-1");
    CHECK_STR(t, "f", "0");
    CHECK_STR(t, "g", "0");
}

TEST(shift_negative_count_errors) {
    Ctx t;
    CHECK_RUN(t, R"(
        let r1 = pcall(fn() { return 1 << -1 })
        let o1 = r1.ok
        let t1 = r1.error.type
        let m1 = r1.error.message
        let n = -1
        let r2 = pcall(fn() { return 1 >> n })
        let o2 = r2.ok
        let t2 = r2.error.type
        let m2 = r2.error.message
        let after = 6 * 7
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "o1", "false");
    CHECK_STR(t, "t1", "TypeError");
    CHECK_STR(t, "m1", "Negative shift count.");
    CHECK_STR(t, "o2", "false");
    CHECK_STR(t, "t2", "TypeError");
    CHECK_STR(t, "m2", "Negative shift count.");
    CHECK_STR(t, "after", "42");
}

TEST(bitwise_precedence) {
    Ctx t;
    CHECK_RUN(t, R"(
        let a = 5 & 3 == 1
        let b = 1 | 2 ^ 3 & 4 << 1
        let c = 1 + 2 & 3
        let d = 2 << 1 + 1
        let e = 7 ^ 3 == 4
        let f = 1 | 2 == 3
        let g = ~1 & 3
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "a", "true");
    CHECK_STR(t, "b", "3");
    CHECK_STR(t, "c", "3");
    CHECK_STR(t, "d", "8");
    CHECK_STR(t, "e", "true");
    CHECK_STR(t, "f", "true");
    CHECK_STR(t, "g", "2");
}

TEST(bitwise_compound_assignment) {
    Ctx t;
    CHECK_RUN(t, R"(
        let x = 6
        x &= 3
        let a = x
        x |= 8
        let b = x
        x ^= 3
        let c = x
        x <<= 2
        let d = x
        x >>= 1
        let e = x
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "a", "2");
    CHECK_STR(t, "b", "10");
    CHECK_STR(t, "c", "9");
    CHECK_STR(t, "d", "36");
    CHECK_STR(t, "e", "18");
}

TEST(bit_packing_roundtrip) {
    Ctx t;
    CHECK_RUN(t, R"(
        let buf = 31
        let low = 2037
        let packed = (buf << 11) | low
        let unpacked_low = packed & 0x7FF
        let unpacked_buf = (packed >> 11) & 0x1F
        let mid = (packed >> 6) & 0xFF
        let ok = unpacked_low == low and unpacked_buf == buf
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "packed", "65525");
    CHECK_STR(t, "unpacked_low", "2037");
    CHECK_STR(t, "unpacked_buf", "31");
    CHECK_STR(t, "mid", "255");
    CHECK_STR(t, "ok", "true");
}

TEST(bitwise_type_errors) {
    Ctx t;
    CHECK_RUN(t, R"(
        let r1 = pcall(fn() { return 1.5 & 1 })
        let t1 = r1.error.type
        let r2 = pcall(fn() { return 1 & 1.5 })
        let t2 = r2.error.type
        let r3 = pcall(fn() { return "a" | 1 })
        let t3 = r3.error.type
        let r4 = pcall(fn() { return 1 ^ true })
        let t4 = r4.error.type
        let r5 = pcall(fn() { return ~1.5 })
        let t5 = r5.error.type
        let r6 = pcall(fn() { return true << 1 })
        let t6 = r6.error.type
        let r7 = pcall(fn() { return none >> 1 })
        let t7 = r7.error.type
        let after = 6 * 7
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "t1", "TypeError");
    CHECK_STR(t, "t2", "TypeError");
    CHECK_STR(t, "t3", "TypeError");
    CHECK_STR(t, "t4", "TypeError");
    CHECK_STR(t, "t5", "TypeError");
    CHECK_STR(t, "t6", "TypeError");
    CHECK_STR(t, "t7", "TypeError");
    CHECK_STR(t, "after", "42");
}

TEST(bitwise_vs_union_annotation_parsing) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn pack(hi: int, lo: int): int { return (hi << 8) | lo }
        fn mask(x: int): int { return x & 0xFF }
        let p: int = pack(2, 11)
        let m: int = mask(511)
        struct Flags(bits: int | none) {}
        let f: Flags = Flags(bits: 3 & 1)
        let fb: int | none = f.bits
        let u: string | int | none = 1 | 2 | 4
        let opt: int? = 1 << 3
        let nested: map[string, int | none] = {"k": 1 ^ 3}
        let keyed: map[int, string] = {1 | 2: "three"}
        let union_arr: array[int | none] = [~0, 1 << 4]
        let nk: int | none = nested["k"]
        let kt: string = keyed[3]
        let ua0: int | none = union_arr[0]
        let ua1: int | none = union_arr[1]
        let r: int = ~0
        let hits = 0
        for i in 0..1 << 3 { hits = i }
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "p", "523");
    CHECK_STR(t, "m", "255");
    CHECK_STR(t, "fb", "1");
    CHECK_STR(t, "u", "7");
    CHECK_STR(t, "opt", "8");
    CHECK_STR(t, "nk", "2");
    CHECK_STR(t, "kt", "three");
    CHECK_STR(t, "ua0", "-1");
    CHECK_STR(t, "ua1", "16");
    CHECK_STR(t, "r", "-1");
    CHECK_STR(t, "hits", "7");
}

TEST(bitwise_type_error_panics) {
    Ctx t;
    CHECK_RUN(t, "let x = 1.5 & 1");
    CHECK(t.panicked());
}

TEST(bitwise_keeps_comparisons_and_unions) {
    Ctx t;
    CHECK_RUN(t, R"(
        let a = 1 < 2
        let b = 1 <= 1
        let c = 2 > 1
        let d = 2 >= 2
        let e = 1 << 2
        let f = 8 >> 1
        let z: string | int = 3
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "a", "true");
    CHECK_STR(t, "b", "true");
    CHECK_STR(t, "c", "true");
    CHECK_STR(t, "d", "true");
    CHECK_STR(t, "e", "4");
    CHECK_STR(t, "f", "4");
    CHECK_STR(t, "z", "3");
}
