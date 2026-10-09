#include "test.hpp"

static bool in_stack(pyle::Value* p, pyle::Value* base, size_t cap) {
    return p >= base && p < base + cap;
}

TEST(upvalue_list_interleaves_across_coros) {
    Ctx t;
    CHECK_RUN(t, R"(
        fn worker() {
            let x = 1
            let get = fn() { return x }
            yield get
            return get()
        }
        let d = Coro(worker)
        let g1 = d.resume()
        let tt = Coro(worker)
        let g2 = tt.resume()
    )");
    CHECK_CALM(t);
    CHECK(t.p.vm.open_upvalues.size() == 2);
    pyle::Coroutine& dc = std::get<pyle::Coroutine>(
        t.p.vm.get_heap_object(t.p.vm.get_global("d").as_ref).data);
    pyle::Coroutine& tc = std::get<pyle::Coroutine>(
        t.p.vm.get_heap_object(t.p.vm.get_global("tt").as_ref).data);
    for (size_t i = 0; i < t.p.vm.open_upvalues.size(); ++i) {
        pyle::Upvalue& uv = std::get<pyle::Upvalue>(
            t.p.vm.get_heap_object(t.p.vm.open_upvalues[i]).data);
        CHECK(uv.location != &uv.closed);
    }
    pyle::Upvalue& first = std::get<pyle::Upvalue>(
        t.p.vm.get_heap_object(t.p.vm.open_upvalues[0]).data);
    pyle::Upvalue& second = std::get<pyle::Upvalue>(
        t.p.vm.get_heap_object(t.p.vm.open_upvalues[1]).data);
    bool first_in_d = in_stack(first.location, dc.stack, dc.stack_capacity);
    bool first_in_t = in_stack(first.location, tc.stack, tc.stack_capacity);
    bool second_in_d = in_stack(second.location, dc.stack, dc.stack_capacity);
    bool second_in_t = in_stack(second.location, tc.stack, tc.stack_capacity);
    CHECK(first_in_d != first_in_t);
    CHECK(second_in_d != second_in_t);
    CHECK(first_in_d != second_in_d);
    CHECK_RUN(t, R"(
        let r1 = d.resume()
        let r2 = tt.resume()
        let sd = d.state()
        let st = tt.state()
    )");
    CHECK_CALM(t);
    CHECK_STR(t, "r1", "1");
    CHECK_STR(t, "r2", "1");
    CHECK_STR(t, "sd", "dead");
    CHECK_STR(t, "st", "dead");
    CHECK(t.p.vm.open_upvalues.empty());
}
