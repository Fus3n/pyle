#pragma once
#include <string_view>

namespace pyle {

    inline constexpr std::string_view PRELUDE_SOURCE = R"pyle(
        fn waitfor(task) {
            let rooted = __is_root()
            while not task.is_done {
                __tick()
                let job = __next_ready_task()
                if job != none {
                    job.resume()
                } else if not rooted {
                    yield
                }
            }
            if task.has_failed {
                raise({type: "FutureRejected", message: task.error, trace: []})
            }
            return task.data
        }

        fn __task_make(handler, arg, slot) {
            return Coro(fn() {
                slot.fulfill(handler(arg))
            })
        }

        let async = {
            run: fn(t) {
                let c = none
                if typeof(t) == "function" {
                    c = Coro(t)
                } else {
                    c = t
                }

                let res = none
                while c.state() != "dead" {
                    res = c.resume()
                }
                let e = c.error()
                if e != none {
                    raise(e)
                }
                return res
            },

            all: fn(tasks) {
                let coros = []
                let results = []
                let rooted = __is_root()

                coros.reserve(tasks.size())
                results.resize(tasks.size(), none)

                for t in tasks {
                    if typeof(t) == "function" {
                        coros.append(Coro(t))
                    } else {
                        coros.append(t)
                    }
                }

                let active_count = coros.size()
                while active_count > 0 {
                    active_count = 0
                    let j = 0
                    while j < coros.size() {
                        let c = coros[j]
                        if c.state() != "dead" {
                            let res = c.resume()
                            if c.state() == "dead" {
                                let e = c.error()
                                if e == none {
                                    results[j] = {ok: true, value: res}
                                } else {
                                    results[j] = {ok: false, error: e}
                                }
                            } else {
                                active_count += 1
                            }
                        }
                        j += 1
                    }
                    __tick()
                    let job = __next_ready_task()
                    if job != none {
                        job.resume()
                    }
                    if not rooted {
                        yield
                    }
                }
                return results
            }
        }
    )pyle";

}