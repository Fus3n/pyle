#include "http_binding.hpp"

namespace pyle {
namespace http_binding {

    namespace {

        struct HttpRequestData {
            std::string method;
            std::string path;
            std::string body;
            std::string remote_addr;
            std::map<std::string, std::string> headers;
            std::map<std::string, std::string> query;
            std::vector<std::string> captures;
        };

        struct HttpSlot {
            std::promise<HttpResponseData> prom;
            HttpSlot() = default;
            explicit HttpSlot(std::promise<HttpResponseData>&& p) : prom(std::move(p)) {}
        };

        struct HttpServerWrapper;

        std::mutex& g_live_servers_mutex() {
            static std::mutex m;
            return m;
        }

        std::vector<HttpServerWrapper*>& g_live_servers() {
            static std::vector<HttpServerWrapper*> servers;
            return servers;
        }

        std::map<std::string, std::string>& mime_overrides() {
            static std::map<std::string, std::string> table;
            return table;
        }

        void apply_mime_overrides(httplib::Server& svr) {
            for (const auto& [ext, type] : mime_overrides()) {
                svr.set_file_extension_and_mimetype_mapping(ext, type);
            }
        }

        struct HttpServerWrapper {
            struct Route {
                std::string verb;
                std::string pattern;
                pyle::Value closure;
            };

            struct PendingJob {
                HttpRequestData* req = nullptr;
                pyle::Value closure;
                std::promise<HttpResponseData> slot;
            };

            pyle::VM* vm = nullptr;
            httplib::Server svr;
            std::string host = "0.0.0.0";
            int port = 8080;
            std::vector<pyle::Value> handlers;
            std::vector<Route> routes;
            std::mutex jobs_mutex;
            std::vector<PendingJob> jobs;
            std::vector<std::pair<pyle::Value, std::shared_ptr<HttpSlot>>> tasks;
            std::atomic<bool> running{false};
            char bound_mode = 0;

            HttpServerWrapper() {
                svr.new_task_queue = [] { return new httplib::ThreadPool(1); };
                apply_mime_overrides(svr);
                std::lock_guard<std::mutex> lock(g_live_servers_mutex());
                g_live_servers().push_back(this);
            }

            void gc_mark(pyle::VM& m) {
                for (const auto& h : handlers) m.mark_value(h);
                for (auto& [coro, slot] : tasks) m.mark_value(coro);
            }

            ~HttpServerWrapper() {
                svr.stop();
                std::lock_guard<std::mutex> lock(g_live_servers_mutex());
                auto& live = g_live_servers();
                for (size_t i = 0; i < live.size(); ++i) {
                    if (live[i] == this) {
                        live[i] = live.back();
                        live.pop_back();
                        break;
                    }
                }
            }

            static void fill_request_data(const httplib::Request& req, HttpRequestData* rd) {
                rd->method = req.method;
                rd->path = req.path;
                rd->body = req.body;
                rd->remote_addr = req.remote_addr;
                for (const auto& [k, v] : req.params) rd->query[k] = v;
                for (const auto& [k, v] : req.headers) rd->headers[k] = v;
                for (size_t i = 1; i < req.matches.size(); ++i) {
                    if (req.matches[i].matched) rd->captures.push_back(req.matches[i].str());
                }
            }

            static void apply_result_data(pyle::VM& m, const pyle::Value& out, HttpResponseData& res) {
                switch (out.tag) {
                    case pyle::Value::Tag::None:
                        return;
                    case pyle::Value::Tag::StringRef:
                        res.body = std::get<std::string>(m.get_heap_object(out.as_ref).data);
                        res.headers["Content-Type"] = "text/plain";
                        return;
                    case pyle::Value::Tag::MapRef: {
                        const auto& entries = std::get<pyle::MapObject>(m.get_heap_object(out.as_ref).data).entries;
                        for (const auto& [k, v] : entries) {
                            std::string key = m.value_to_string(k);
                            if (key == "status") {
                                if (v.tag != pyle::Value::Tag::Int) {
                                    m.runtime_error(pyle::RuntimeError::Type, "handler response 'status' expects an int.");
                                    return;
                                }
                                res.status = static_cast<int>(v.as_int);
                            } else if (key == "body") {
                                if (v.tag != pyle::Value::Tag::StringRef) {
                                    m.runtime_error(pyle::RuntimeError::Type, "handler response 'body' expects a string.");
                                    return;
                                }
                                res.body = std::get<std::string>(m.get_heap_object(v.as_ref).data);
                            } else if (key == "headers") {
                                for (const auto& [hk, hv] : pyle::from_value<std::map<std::string, std::string>>(m, v)) {
                                    res.headers[hk] = hv;
                                }
                            }
                        }
                        return;
                    }
                    default:
                        m.runtime_error(pyle::RuntimeError::Type,
                            "http handler must return none, a string, or a map {status, headers, body}.");
                        return;
                }
            }

            pyle::Value pump(pyle::VM& m) {
                GcDisable gc(m);
                std::vector<PendingJob> batch;
                {
                    std::lock_guard<std::mutex> lock(jobs_mutex);
                    batch.swap(jobs);
                }

                static pyle::Value factory;
                static bool factory_resolved = false;

                for (auto& job : batch) {
                    if (!factory_resolved) {
                        factory = m.get_global("__task_make");
                        factory_resolved = (factory.tag == pyle::Value::Tag::ClosureRef ||
                                            factory.tag == pyle::Value::Tag::FuncRef);
                    }
                    auto slot = std::make_shared<HttpSlot>(std::move(job.slot));
                    pyle::Value req_val = pyle::to_value_owned(m, job.req);
                    job.req = nullptr;
                    pyle::Value slot_val = pyle::to_value_owned(m, new std::shared_ptr<HttpSlot>(slot));
                    pyle::Value coro_val = m.call_func(factory, job.closure, req_val, slot_val);
                    if (coro_val.tag == pyle::Value::Tag::CoroutineRef) {
                        tasks.push_back({coro_val, slot});
                    } else {
                        HttpResponseData err;
                        err.status = 500;
                        err.body = "internal error";
                        try {
                            slot->prom.set_value(std::move(err));
                        } catch (...) {
                        }
                    }
                }

                for (size_t i = 0; i < tasks.size();) {
                    auto& t = tasks[i];
                    if (t.first.tag == pyle::Value::Tag::CoroutineRef) {
                        pyle::Coroutine* c = std::get_if<pyle::Coroutine>(&m.get_heap_object(t.first.as_ref).data);
                        if (!c) {
                            tasks.erase(tasks.begin() + i);
                            continue;
                        }
                        if (c->state == pyle::Coroutine::State::Dead) {
                            tasks.erase(tasks.begin() + i);
                            continue;
                        }
                        if (c->state == pyle::Coroutine::State::Suspended) {
                            std::lock_guard<std::mutex> lock(pyle::ready_tasks_mutex());
                            pyle::ready_tasks().push_back(t.first);
                        }
                    }
                    ++i;
                }

                return pyle::Value();
            }

            void materialize(bool enqueue_mode) {
                for (const auto& r : routes) {
                    pyle::VM* m = vm;
                    const pyle::Value cb = r.closure;
                    if (enqueue_mode) {
                        auto handler = [this, cb](const httplib::Request& req, httplib::Response& res) {
                            auto* rd = new HttpRequestData();
                            fill_request_data(req, rd);
                            PendingJob job;
                            job.req = rd;
                            job.closure = cb;
                            auto fut = job.slot.get_future();
                            {
                                std::lock_guard<std::mutex> lock(jobs_mutex);
                                jobs.push_back(std::move(job));
                            }
                            HttpResponseData out = fut.get();
                            res.status = static_cast<int>(out.status);
                            for (const auto& [hk, hv] : out.headers) res.headers.insert({hk, hv});
                            if (!out.body.empty() && out.headers.find("Content-Type") == out.headers.end()) {
                                res.set_content(out.body, "text/plain");
                            } else {
                                res.body = out.body;
                            }
                        };
                        register_handler(r.verb, r.pattern, handler);
                    } else {
                        auto handler = [m, cb](const httplib::Request& req, httplib::Response& res) {
                            GcDisable gc(*m);
                            try {
                                auto* rd = new HttpRequestData();
                                fill_request_data(req, rd);
                                pyle::Value req_val = pyle::to_value_owned(*m, rd);
                                pyle::Value out = m->call_func(cb, req_val);
                                HttpResponseData data;
                                data.status = 200;
                                if (!m->is_panicked()) apply_result_data(*m, out, data);
                                if (m->is_panicked()) {
                                    data.status = 500;
                                    data.body = "internal error";
                                }
                                res.status = static_cast<int>(data.status);
                                for (const auto& [hk, hv] : data.headers) res.headers.insert({hk, hv});
                                if (!data.body.empty() && data.headers.find("Content-Type") == data.headers.end()) {
                                    res.set_content(data.body, "text/plain");
                                } else {
                                    res.body = data.body;
                                }
                            } catch (...) {
                                res.status = 500;
                                res.set_content("internal error", "text/plain");
                            }
                        };
                        register_handler(r.verb, r.pattern, handler);
                    }
                }
            }

            void register_handler(const std::string& verb, const std::string& pattern,
                                  std::function<void(const httplib::Request&, httplib::Response&)> handler) {
                if (verb == "GET" || verb == "HEAD") svr.Get(pattern, handler);
                else if (verb == "POST")   svr.Post(pattern, handler);
                else if (verb == "PUT")    svr.Put(pattern, handler);
                else if (verb == "PATCH")  svr.Patch(pattern, handler);
                else if (verb == "DELETE") svr.Delete(pattern, handler);
                else                       svr.Options(pattern, handler);
            }
        };

        pyle::Value server_route(pyle::VM& vm, pyle::HeapIdx self_idx, pyle::ArgView args, const std::string& verb) {
            if (args.size() != 2 || args[0].tag != pyle::Value::Tag::StringRef ||
                (args[1].tag != pyle::Value::Tag::ClosureRef && args[1].tag != pyle::Value::Tag::FuncRef)) {
                vm.runtime_error(pyle::RuntimeError::ArgumentError,
                    "server." + verb + " expects (pattern: string, handler: fn(req)).");
                return pyle::Value();
            }
            auto& native = std::get<pyle::NativeObject>(vm.get_heap_object(self_idx).data);
            auto* server = static_cast<HttpServerWrapper*>(native.ptr);
            server->handlers.push_back(args[1]);
            server->routes.push_back({verb, pyle::from_value<std::string>(vm, args[0]), args[1]});
            return pyle::Value();
        }

        pyle::Value server_get_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return server_route(m, s, a, "GET"); }
        pyle::Value server_post_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return server_route(m, s, a, "POST"); }
        pyle::Value server_put_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return server_route(m, s, a, "PUT"); }
        pyle::Value server_patch_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return server_route(m, s, a, "PATCH"); }
        pyle::Value server_delete_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return server_route(m, s, a, "DELETE"); }
        pyle::Value server_head_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return server_route(m, s, a, "HEAD"); }
        pyle::Value server_options_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return server_route(m, s, a, "OPTIONS"); }

    }

    void register_server(NativeModule& mod) {
        VM& vm = mod.get_vm();

        static bool tick_registered = false;
        if (!tick_registered) {
            tick_registered = true;
            pyle::background_ticks().push_back([](pyle::VM& m, pyle::ArgView) -> pyle::Value {
                std::vector<HttpServerWrapper*> live;
                {
                    std::lock_guard<std::mutex> lock(g_live_servers_mutex());
                    live = g_live_servers();
                }
                for (auto* server : live) {
                    server->pump(m);
                }
                return pyle::Value();
            });
        }

        {
            pyle::SharedClassBinder<HttpSlot> slot_binder(vm, "__Slot");
            slot_binder.custom_constructor([](pyle::VM& m, pyle::ArgView) -> pyle::Value {
                return pyle::to_value_owned(m, new std::shared_ptr<HttpSlot>(std::make_shared<HttpSlot>()));
            });
            slot_binder.custom_method("fulfill", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView args) -> pyle::Value {
                if (args.size() != 1) {
                    m.runtime_error(pyle::RuntimeError::ArgumentError, "fulfill expects exactly 1 value.");
                    return pyle::Value();
                }
                auto& sp = *static_cast<std::shared_ptr<HttpSlot>*>(std::get<pyle::NativeObject>(m.get_heap_object(self).data).ptr);
                HttpResponseData data;
                data.status = 200;
                try {
                    HttpServerWrapper::apply_result_data(m, args[0], data);
                } catch (...) {
                }
                if (data.status == 0) data.status = 200;
                if (m.is_panicked()) {
                    data.status = 500;
                    data.body = "internal error";
                }
                try {
                    sp->prom.set_value(std::move(data));
                } catch (...) {
                }
                return pyle::Value();
            });
            mod.class_binder(slot_binder);
        }

        mod.raw_function("mime", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            if (args.size() != 2 || args[0].tag != pyle::Value::Tag::StringRef || args[1].tag != pyle::Value::Tag::StringRef) {
                m.runtime_error(pyle::RuntimeError::ArgumentError,
                    "http.mime expects (extension: string, content_type: string).");
                return pyle::Value();
            }
            std::string ext = pyle::from_value<std::string>(m, args[0]);
            std::string type = pyle::from_value<std::string>(m, args[1]);
            for (auto& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            if (!ext.empty() && ext.front() == '.') ext = ext.substr(1);
            mime_overrides()[ext] = type;
            return pyle::Value();
        });

        {
            pyle::ClassBinder<HttpRequestData> binder(vm, "Request");
            binder.custom_getter("method", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto* d = static_cast<HttpRequestData*>(std::get<pyle::NativeObject>(m.get_heap_object(self).data).ptr);
                return pyle::to_value(m, d->method);
            });
            binder.custom_getter("path", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto* d = static_cast<HttpRequestData*>(std::get<pyle::NativeObject>(m.get_heap_object(self).data).ptr);
                return pyle::to_value(m, d->path);
            });
            binder.custom_getter("body", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto* d = static_cast<HttpRequestData*>(std::get<pyle::NativeObject>(m.get_heap_object(self).data).ptr);
                return pyle::to_value(m, d->body);
            });
            binder.custom_getter("remote_addr", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto* d = static_cast<HttpRequestData*>(std::get<pyle::NativeObject>(m.get_heap_object(self).data).ptr);
                return pyle::to_value(m, d->remote_addr);
            });
            binder.custom_getter("headers", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto* d = static_cast<HttpRequestData*>(std::get<pyle::NativeObject>(m.get_heap_object(self).data).ptr);
                return pyle::to_value(m, d->headers);
            });
            binder.custom_getter("query", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto* d = static_cast<HttpRequestData*>(std::get<pyle::NativeObject>(m.get_heap_object(self).data).ptr);
                return pyle::to_value(m, d->query);
            });
            binder.custom_getter("captures", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto* d = static_cast<HttpRequestData*>(std::get<pyle::NativeObject>(m.get_heap_object(self).data).ptr);
                return pyle::to_value(m, d->captures);
            });
            mod.class_binder(binder);
        }

        {
            pyle::ClassBinder<HttpServerWrapper> binder(vm, "Server");
            binder.custom_constructor([](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
                auto* server = new HttpServerWrapper();
                server->vm = &m;
                for (size_t i = 0; i < args.size(); ++i) {
                    if (args[i].tag == pyle::Value::Tag::Int) {
                        server->port = static_cast<int>(args[i].as_int);
                    } else if (args[i].tag == pyle::Value::Tag::Float) {
                        server->port = static_cast<int>(args[i].as_float);
                    } else if (args[i].tag == pyle::Value::Tag::StringRef) {
                        server->host = std::get<std::string>(m.get_heap_object(args[i].as_ref).data);
                    } else {
                        delete server;
                        m.runtime_error(pyle::RuntimeError::ArgumentError,
                            "http.Server expects optional (host: string) and/or (port: int).");
                        return pyle::Value();
                    }
                }
                return pyle::to_value_owned(m, server);
            });

            auto bind_route = [&](const char* name, pyle::NativeMethodFn thunk) {
                binder.custom_method(name, thunk);
            };
            bind_route("get", server_get_thunk);
            bind_route("post", server_post_thunk);
            bind_route("put", server_put_thunk);
            bind_route("patch", server_patch_thunk);
            bind_route("delete", server_delete_thunk);
            bind_route("head", server_head_thunk);
            bind_route("options", server_options_thunk);

            binder.custom_method("mount", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView args) -> pyle::Value {
                if (args.size() != 2 || args[0].tag != pyle::Value::Tag::StringRef || args[1].tag != pyle::Value::Tag::StringRef) {
                    m.runtime_error(pyle::RuntimeError::ArgumentError, "mount expects (prefix: string, directory: string).");
                    return pyle::Value();
                }
                auto& native = std::get<pyle::NativeObject>(m.get_heap_object(self).data);
                auto* server = static_cast<HttpServerWrapper*>(native.ptr);
                if (!server->svr.set_mount_point(pyle::from_value<std::string>(m, args[0]),
                                                 pyle::from_value<std::string>(m, args[1]))) {
                    m.runtime_error(pyle::RuntimeError::Runtime, "mount failed: directory does not exist.");
                }
                return pyle::Value();
            });

            binder.custom_method("run", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto& native = std::get<pyle::NativeObject>(m.get_heap_object(self).data);
                auto* server = static_cast<HttpServerWrapper*>(native.ptr);
                if (server->running.load()) {
                    m.runtime_error(pyle::RuntimeError::Runtime, "http server is already running.");
                    return pyle::Value();
                }
                if (server->bound_mode == 'a') {
                    m.runtime_error(pyle::RuntimeError::Runtime, "http server was started with serve_async and cannot switch to run.");
                    return pyle::Value();
                }
                server->bound_mode = 'b';
                server->materialize(false);
                apply_mime_overrides(server->svr);
                if (!server->svr.listen(server->host, server->port)) {
                    m.runtime_error(pyle::RuntimeError::Runtime,
                        "http server failed to listen on " + server->host + ":" + std::to_string(server->port) + ".");
                }
                return pyle::Value();
            });

            binder.custom_method("serve_async", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto& native = std::get<pyle::NativeObject>(m.get_heap_object(self).data);
                auto* server = static_cast<HttpServerWrapper*>(native.ptr);
                if (server->running.exchange(true)) {
                    m.runtime_error(pyle::RuntimeError::Runtime, "http server is already running.");
                    return pyle::Value();
                }
                if (server->bound_mode == 'b') {
                    server->running.store(false);
                    m.runtime_error(pyle::RuntimeError::Runtime, "http server was started with run and cannot switch to serve_async.");
                    return pyle::Value();
                }
                if (server->host.empty()) {
                    server->running.store(false);
                    m.runtime_error(pyle::RuntimeError::Runtime, "http server host is empty.");
                    return pyle::Value();
                }
                server->bound_mode = 'a';
                pyle::ready_tasks_owner() = m.active_coroutine_idx;
                auto [fval, fut] = pyle::Future::create(m);
                auto* fut_ptr = new std::shared_ptr<pyle::Future>(fut);
                server->materialize(true);
                apply_mime_overrides(server->svr);
                server->svr.new_task_queue = [] { return new httplib::ThreadPool(16); };
                std::thread([server, fut_ptr]() {
                    bool ok = server->svr.listen(server->host, server->port);
                    server->running.store(false);
                    std::lock_guard<std::recursive_mutex> lock(server->vm->get_mutex());
                    if (!ok) {
                        (*fut_ptr)->failed = true;
                        (*fut_ptr)->error = "http server failed to listen on " + server->host + ":" +
                                            std::to_string(server->port) + ".";
                    }
                    (*fut_ptr)->finished.store(true);
                    delete fut_ptr;
                }).detach();
                return fval;
            });

            binder.custom_method("pump", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto& native = std::get<pyle::NativeObject>(m.get_heap_object(self).data);
                auto* server = static_cast<HttpServerWrapper*>(native.ptr);
                return server->pump(m);
            });

            binder.custom_method("stop", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto& native = std::get<pyle::NativeObject>(m.get_heap_object(self).data);
                auto* server = static_cast<HttpServerWrapper*>(native.ptr);
                server->svr.stop();
                return pyle::Value();
            });

            binder.custom_method("port", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto& native = std::get<pyle::NativeObject>(m.get_heap_object(self).data);
                auto* server = static_cast<HttpServerWrapper*>(native.ptr);
                return pyle::Value(static_cast<int64_t>(server->port));
            });

            mod.class_binder(binder);
        }
    }

}
}
