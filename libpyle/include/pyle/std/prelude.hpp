#pragma once
#include <string_view>

namespace pyle {

    inline constexpr std::string_view PRELUDE_SOURCE = R"pyle(
        fn waitfor(task) {
            while not task.is_done {
                yield
            }
            if task.has_failed {
                print("Error: " + task.error)
                return none
            }
            return task.data
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
                return res
            },

            all: fn(tasks) {
                let coros = []
                let results = []
                
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
                                results[j] = res
                            } else {
                                active_count += 1
                            }
                        }
                        j += 1
                    }
                    yield
                }
                return results
            }
        }
    )pyle";

}