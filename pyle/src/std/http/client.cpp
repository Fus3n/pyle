#include "http_binding.hpp"
#include <fstream>
#include <iterator>

namespace pyle {
namespace http_binding {

    namespace {

        std::mutex g_clients_mutex;
        std::vector<std::shared_ptr<httplib::Client>> g_clients;

        void track_client(const std::shared_ptr<httplib::Client>& c) {
            std::lock_guard<std::mutex> lock(g_clients_mutex);
            g_clients.push_back(c);
        }

        void untrack_client(const std::shared_ptr<httplib::Client>& c) {
            std::lock_guard<std::mutex> lock(g_clients_mutex);
            for (auto it = g_clients.begin(); it != g_clients.end(); ++it) {
                if (it->get() == c.get()) { g_clients.erase(it); break; }
            }
        }

        void cancel_clients() {
            std::lock_guard<std::mutex> lock(g_clients_mutex);
            for (auto& c : g_clients) c->stop();
        }

        struct ParsedUrl {
            bool https = false;
            std::string host;
            int port = 0;
            std::string path;
        };

        bool parse_url(const std::string& url, ParsedUrl& out) {
            std::string rest = url;
            auto scheme_pos = rest.find("://");
            if (scheme_pos != std::string::npos) {
                std::string scheme = rest.substr(0, scheme_pos);
                for (auto& c : scheme) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
                if (scheme == "https") out.https = true;
                else if (scheme == "http") out.https = false;
                else return false;
                rest = rest.substr(scheme_pos + 3);
            } else {
                out.https = false;
            }

            auto slash = rest.find('/');
            std::string authority = (slash == std::string::npos) ? rest : rest.substr(0, slash);
            out.path = (slash == std::string::npos) ? "/" : rest.substr(slash);

            if (!authority.empty() && authority.front() == '[') {
                auto close = authority.find(']');
                if (close == std::string::npos) return false;
                out.host = authority.substr(1, close - 1);
                if (close + 1 < authority.size() && authority[close + 1] == ':') {
                    out.port = std::atoi(authority.c_str() + close + 2);
                }
            } else {
                auto colon = authority.rfind(':');
                if (colon != std::string::npos) {
                    out.host = authority.substr(0, colon);
                    out.port = std::atoi(authority.c_str() + colon + 1);
                } else {
                    out.host = authority;
                }
            }
            if (out.host.empty()) return false;
            if (out.port == 0) out.port = out.https ? 443 : 80;
            return true;
        }

        std::shared_ptr<httplib::Client> make_client(const ParsedUrl& url) {
            std::string base;
            base += url.https ? "https://" : "http://";
            base += url.host;
            bool default_port = (url.port == (url.https ? 443 : 80));
            if (!default_port) base += ":" + std::to_string(url.port);
            auto client = std::make_shared<httplib::Client>(base);
            client->set_follow_location(true);
            return client;
        }

        httplib::Headers to_httplib_headers(const std::map<std::string, std::string>& in) {
            httplib::Headers out;
            for (const auto& [k, v] : in) out.insert({k, v});
            return out;
        }

        struct FormItem {
            std::string name;
            std::string value;
            std::string file_path;
            std::string content_type;
        };

        struct RequestOptions {
            std::map<std::string, std::string> headers;
            std::string body;
            double timeout = -1.0;
            std::string content_type = "text/plain";
            bool follow_redirects = true;
            bool verify_ssl = true;
            std::string proxy_host;
            int proxy_port = 0;
            bool multipart = false;
            std::vector<FormItem> form;
        };

        void parse_options(pyle::VM& vm, const pyle::Value& val, RequestOptions& opts) {
            if (val.tag == pyle::Value::Tag::None) return;

            const auto& entries = std::get<pyle::MapObject>(vm.get_heap_object(val.as_ref).data).entries;
            for (const auto& [k, v] : entries) {
                std::string key = (k.tag == pyle::Value::Tag::StringRef)
                    ? std::get<std::string>(vm.get_heap_object(k.as_ref).data)
                    : vm.value_to_string(k);
                if (key == "headers") {
                    opts.headers = pyle::from_value<std::map<std::string, std::string>>(vm, v);
                } else if (key == "body") {
                    if (v.tag != pyle::Value::Tag::StringRef) {
                        vm.runtime_error(pyle::RuntimeError::Type, "http option 'body' expects a string.");
                        return;
                    }
                    opts.body = std::get<std::string>(vm.get_heap_object(v.as_ref).data);
                } else if (key == "timeout") {
                    if (v.tag != pyle::Value::Tag::Int && v.tag != pyle::Value::Tag::Float) {
                        vm.runtime_error(pyle::RuntimeError::Type, "http option 'timeout' expects a number of seconds.");
                        return;
                    }
                    opts.timeout = (v.tag == pyle::Value::Tag::Int) ? static_cast<double>(v.as_int) : v.as_float;
                } else if (key == "content_type") {
                    if (v.tag != pyle::Value::Tag::StringRef) {
                        vm.runtime_error(pyle::RuntimeError::Type, "http option 'content_type' expects a string.");
                        return;
                    }
                    opts.content_type = std::get<std::string>(vm.get_heap_object(v.as_ref).data);
                } else if (key == "follow_redirects") {
                    if (v.tag == pyle::Value::Tag::Bool) opts.follow_redirects = v.as_bool;
                } else if (key == "verify_ssl") {
                    if (v.tag == pyle::Value::Tag::Bool) opts.verify_ssl = v.as_bool;
                } else if (key == "proxy") {
                    if (v.tag == pyle::Value::Tag::StringRef) {
                        std::string p = std::get<std::string>(vm.get_heap_object(v.as_ref).data);
                        auto colon = p.rfind(':');
                        if (colon != std::string::npos) {
                            opts.proxy_host = p.substr(0, colon);
                            opts.proxy_port = std::atoi(p.c_str() + colon + 1);
                        } else if (!p.empty()) {
                            opts.proxy_host = p;
                            opts.proxy_port = 8080;
                        }
                    }
                } else if (key == "form") {
                    if (v.tag == pyle::Value::Tag::ArrayRef) {
                        opts.multipart = true;
                        const auto& arr = std::get<pyle::ArrayType>(vm.get_heap_object(v.as_ref).data);
                        for (const auto& item : arr) {
                            if (item.tag != pyle::Value::Tag::MapRef) continue;
                            FormItem fi;
                            const auto& fe = std::get<pyle::MapObject>(vm.get_heap_object(item.as_ref).data).entries;
                            for (const auto& [fk, fv] : fe) {
                                if (fv.tag != pyle::Value::Tag::StringRef) continue;
                                std::string fkey = (fk.tag == pyle::Value::Tag::StringRef)
                                    ? std::get<std::string>(vm.get_heap_object(fk.as_ref).data)
                                    : vm.value_to_string(fk);
                                std::string fval = std::get<std::string>(vm.get_heap_object(fv.as_ref).data);
                                if (fkey == "name") fi.name = fval;
                                else if (fkey == "value") fi.value = fval;
                                else if (fkey == "path") fi.file_path = fval;
                                else if (fkey == "content_type") fi.content_type = fval;
                            }
                            if (!fi.name.empty()) opts.form.push_back(std::move(fi));
                        }
                    }
                }
            }
        }

        pyle::Value wrap_response(pyle::VM& vm, httplib::Result&& result) {
            if (!result) {
                vm.runtime_error(pyle::RuntimeError::Runtime,
                    "http request failed: " + std::string(httplib::to_string(result.error())));
                return pyle::Value();
            }
            auto* data = new HttpResponseData();
            data->status = result->status;
            data->body = result->body;
            for (const auto& [k, v] : result->headers) data->headers.insert({k, v});
            return pyle::to_value_owned(vm, data);
        }

        bool iequals(const std::string& a, const std::string& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) {
                if (tolower(static_cast<unsigned char>(a[i])) != tolower(static_cast<unsigned char>(b[i]))) return false;
            }
            return true;
        }

        httplib::Result perform_request(httplib::Client& client, const std::string& verb,
                                        const std::string& path, const RequestOptions& opts) {
            httplib::Headers headers = to_httplib_headers(opts.headers);
            std::string ctype = opts.content_type;
            httplib::Headers clean;
            for (const auto& [k, v] : headers) {
                if (iequals(k, "Content-Type")) { ctype = v; continue; }
                clean.insert({k, v});
            }
            headers = clean;
            if (opts.multipart) {
                httplib::UploadFormDataItems items;
                for (const auto& f : opts.form) {
                    httplib::UploadFormData item;
                    item.name = f.name;
                    if (!f.file_path.empty()) {
                        std::ifstream ifs(f.file_path, std::ios::binary);
                        std::string content((std::istreambuf_iterator<char>(ifs)),
                                            std::istreambuf_iterator<char>());
                        item.content = std::move(content);
                        auto slash = f.file_path.find_last_of("/\\");
                        item.filename = (slash == std::string::npos) ? f.file_path : f.file_path.substr(slash + 1);
                        item.content_type = f.content_type.empty() ? "application/octet-stream" : f.content_type;
                    } else {
                        item.content = f.value;
                    }
                    items.push_back(std::move(item));
                }
                if (verb == "PUT") return client.Put(path, headers, items);
                if (verb == "PATCH") return client.Patch(path, headers, items);
                return client.Post(path, headers, items);
            }
            if (verb == "GET")    return client.Get(path, headers);
            if (verb == "HEAD")   return client.Head(path, headers);
            if (verb == "OPTIONS")return client.Options(path, headers);
            if (verb == "DELETE") return client.Delete(path, headers);
            if (verb == "POST")   return client.Post(path, headers, opts.body, ctype.c_str());
            if (verb == "PUT")    return client.Put(path, headers, opts.body, ctype.c_str());
            if (verb == "PATCH")  return client.Patch(path, headers, opts.body, ctype.c_str());
            return client.Get(path, headers);
        }

        std::optional<HttpResponseData> exec_request(std::shared_ptr<httplib::Client> client,
                                                     const std::string& verb, const std::string& path,
                                                     const RequestOptions& opts, std::string& error) {
            if (!client) {
                error = "https requires an ssl-enabled build of the http module.";
                return std::nullopt;
            }
            client->set_follow_location(opts.follow_redirects);
            client->enable_server_certificate_verification(opts.verify_ssl);
            if (!opts.proxy_host.empty() && opts.proxy_port > 0) client->set_proxy(opts.proxy_host, opts.proxy_port);
            double timeout = opts.timeout > 0 ? opts.timeout : 30.0;
            client->set_connection_timeout(std::chrono::duration<double>(timeout));
            client->set_read_timeout(std::chrono::duration<double>(timeout));
            track_client(client);
            httplib::Result result = perform_request(*client, verb, path, opts);
            untrack_client(client);
            if (!result) {
                error = "http request failed: " + std::string(httplib::to_string(result.error()));
                return std::nullopt;
            }
            HttpResponseData data;
            data.status = result->status;
            data.body = result->body;
            for (const auto& [k, v] : result->headers) data.headers.insert({k, v});
            return data;
        }

        pyle::Value url_request_dispatch(pyle::VM& vm, const std::string& verb, const std::string& url,
                                         RequestOptions& opts, bool async) {
            ParsedUrl parsed;
            if (!parse_url(url, parsed)) {
                vm.runtime_error(pyle::RuntimeError::Runtime, "http invalid url: '" + url + "'.");
                return pyle::Value();
            }

            if (!async) {
                auto client = make_client(parsed);
                std::string error;
                auto data = exec_request(std::move(client), verb, parsed.path, opts, error);
                if (!data) {
                    vm.runtime_error(pyle::RuntimeError::Runtime, error);
                    return pyle::Value();
                }
                auto* out = new HttpResponseData(std::move(*data));
                return pyle::to_value_owned(vm, out);
            }

            auto [fval, fut] = pyle::Future::create(vm);
            auto client = make_client(parsed);
            fut->set_cancel_hook([client]() { client->stop(); });
            std::thread([fut, &vm, verb, url, parsed, opts, client]() {
                std::string error;
                auto data = exec_request(client, verb, parsed.path, opts, error);
                std::lock_guard<std::recursive_mutex> lock(vm.get_mutex());
                if (!data) {
                    fut->failed = true;
                    fut->error = error;
                    fut->finished.store(true);
                } else {
                    auto* out = new HttpResponseData(std::move(*data));
                    fut->resolve(pyle::to_value_owned(vm, out));
                }
            }).detach();
            return fval;
        }

        pyle::Value run_url_request(pyle::VM& vm, const std::string& verb, const pyle::Value* url_val,
                                    const pyle::Value* opts_val, bool async = false) {
            GcDisable gc(vm);
            if (!url_val || url_val->tag != pyle::Value::Tag::StringRef) {
                vm.runtime_error(pyle::RuntimeError::ArgumentError, "http." + verb + " expects a url string.");
                return pyle::Value();
            }
            const std::string& url = std::get<std::string>(vm.get_heap_object(url_val->as_ref).data);

            RequestOptions opts;
            if (opts_val) parse_options(vm, *opts_val, opts);
            if (vm.is_panicked()) return pyle::Value();

            return url_request_dispatch(vm, verb, url, opts, async);
        }

        struct HttpClientWrapper {
            ParsedUrl base;
            std::shared_ptr<httplib::Client> client;

            explicit HttpClientWrapper(const ParsedUrl& u)
                : base(u), client(make_client(u)) {}

            httplib::Headers default_headers;
        };

        pyle::Value client_call(pyle::VM& vm, pyle::HeapIdx self_idx, pyle::ArgView args,
                                const std::string& verb, bool async = false) {
            GcDisable gc(vm);
            if (args.size() < 1 || args.size() > 2 || args[0].tag != pyle::Value::Tag::StringRef) {
                vm.runtime_error(pyle::RuntimeError::ArgumentError,
                    "expected get(path, options = none).");
                return pyle::Value();
            }
            auto& native = std::get<pyle::NativeObject>(vm.get_heap_object(self_idx).data);
            auto* wrapper = static_cast<HttpClientWrapper*>(native.ptr);
            if (!wrapper->client && wrapper->base.https) {
                vm.runtime_error(pyle::RuntimeError::Runtime, "client was constructed with an https base url on a non-ssl build.");
                return pyle::Value();
            }

            const std::string& path = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
            RequestOptions opts;
            if (args.size() == 2) parse_options(vm, args[1], opts);
            if (vm.is_panicked()) return pyle::Value();

            if (async) {
                ParsedUrl base = wrapper->base;
                for (const auto& [k, v] : wrapper->default_headers) opts.headers[k] = v;
                auto [fval, fut] = pyle::Future::create(vm);
                auto client = make_client(base);
                fut->set_cancel_hook([client]() { client->stop(); });
                std::thread([fut, &vm, verb, base, path, opts, client]() {
                    std::string error;
                    auto data = exec_request(client, verb, path, opts, error);
                    std::lock_guard<std::recursive_mutex> lock(vm.get_mutex());
                    if (!data) {
                        fut->failed = true;
                        fut->error = error;
                        fut->finished.store(true);
                    } else {
                        auto* out = new HttpResponseData(std::move(*data));
                        fut->resolve(pyle::to_value_owned(vm, out));
                    }
                }).detach();
                return fval;
            }

            httplib::Headers merged = wrapper->default_headers;
            for (const auto& [k, v] : to_httplib_headers(opts.headers)) merged.insert({k, v});
            opts.headers.clear();
            for (const auto& [k, v] : merged) {
                if (opts.headers.find(k) == opts.headers.end()) opts.headers[k] = v;
            }

            double timeout = opts.timeout > 0 ? opts.timeout : 30.0;
            wrapper->client->set_follow_location(opts.follow_redirects);
            wrapper->client->enable_server_certificate_verification(opts.verify_ssl);
            if (!opts.proxy_host.empty() && opts.proxy_port > 0) wrapper->client->set_proxy(opts.proxy_host, opts.proxy_port);
            wrapper->client->set_connection_timeout(std::chrono::duration<double>(timeout));
            wrapper->client->set_read_timeout(std::chrono::duration<double>(timeout));

            track_client(wrapper->client);
            httplib::Result result = perform_request(*wrapper->client, verb, path, opts);
            untrack_client(wrapper->client);
            return wrap_response(vm, std::move(result));
        }

        std::string percent_encode_byte(char c) {
            static const char* hex = "0123456789ABCDEF";
            std::string out = "%";
            out += hex[(c >> 4) & 0xF];
            out += hex[c & 0xF];
            return out;
        }

        pyle::Value native_url_encode(pyle::VM& vm, pyle::ArgView args) {
            if (args.size() != 1 || args[0].tag != pyle::Value::Tag::StringRef) {
                vm.runtime_error(pyle::RuntimeError::ArgumentError, "http.url_encode expects 1 string argument.");
                return pyle::Value();
            }
            const std::string& raw = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
            std::string out;
            out.reserve(raw.size());
            for (char c : raw) {
                unsigned char uc = static_cast<unsigned char>(c);
                if ((uc >= 'A' && uc <= 'Z') || (uc >= 'a' && uc <= 'z') || (uc >= '0' && uc <= '9') ||
                    uc == '-' || uc == '_' || uc == '.' || uc == '~') {
                    out += c;
                } else {
                    out += percent_encode_byte(c);
                }
            }
            return pyle::to_value(vm, out);
        }

        pyle::Value native_url_decode(pyle::VM& vm, pyle::ArgView args) {
            if (args.size() != 1 || args[0].tag != pyle::Value::Tag::StringRef) {
                vm.runtime_error(pyle::RuntimeError::ArgumentError, "http.url_decode expects 1 string argument.");
                return pyle::Value();
            }
            const std::string& raw = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
            std::string out;
            out.reserve(raw.size());
            for (size_t i = 0; i < raw.size(); ++i) {
                if (raw[i] == '%' && i + 2 < raw.size() &&
                    isxdigit(static_cast<unsigned char>(raw[i + 1])) && isxdigit(static_cast<unsigned char>(raw[i + 2]))) {
                    auto hex_val = [](char h) -> int {
                        if (h >= '0' && h <= '9') return h - '0';
                        if (h >= 'a' && h <= 'f') return h - 'a' + 10;
                        return h - 'A' + 10;
                    };
                    out += static_cast<char>((hex_val(raw[i + 1]) << 4) | hex_val(raw[i + 2]));
                    i += 2;
                } else if (raw[i] == '+') {
                    out += ' ';
                } else {
                    out += raw[i];
                }
            }
            return pyle::to_value(vm, out);
        }

        pyle::Value client_get_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "GET"); }
        pyle::Value client_post_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "POST"); }
        pyle::Value client_put_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "PUT"); }
        pyle::Value client_patch_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "PATCH"); }
        pyle::Value client_delete_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "DELETE"); }
        pyle::Value client_head_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "HEAD"); }
        pyle::Value client_options_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "OPTIONS"); }

        pyle::Value client_get_async_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "GET", true); }
        pyle::Value client_post_async_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "POST", true); }
        pyle::Value client_put_async_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "PUT", true); }
        pyle::Value client_patch_async_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "PATCH", true); }
        pyle::Value client_delete_async_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "DELETE", true); }
        pyle::Value client_head_async_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "HEAD", true); }
        pyle::Value client_options_async_thunk(pyle::VM& m, pyle::HeapIdx s, pyle::ArgView a) { return client_call(m, s, a, "OPTIONS", true); }

    }

    void register_client(NativeModule& mod) {
        mod.raw_function("get", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "GET", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr);
        });
        mod.raw_function("post", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "POST", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr);
        });
        mod.raw_function("put", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "PUT", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr);
        });
        mod.raw_function("patch", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "PATCH", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr);
        });
        mod.raw_function("delete", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "DELETE", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr);
        });
        mod.raw_function("head", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "HEAD", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr);
        });
        mod.raw_function("get_async", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "GET", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr, true);
        });
        mod.raw_function("post_async", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "POST", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr, true);
        });
        mod.raw_function("put_async", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "PUT", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr, true);
        });
        mod.raw_function("patch_async", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "PATCH", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr, true);
        });
        mod.raw_function("delete_async", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "DELETE", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr, true);
        });
        mod.raw_function("head_async", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "HEAD", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr, true);
        });
        mod.raw_function("options", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "OPTIONS", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr);
        });
        mod.raw_function("options_async", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            return run_url_request(m, "OPTIONS", args.size() > 0 ? &args[0] : nullptr,
                                   args.size() > 1 ? &args[1] : nullptr, true);
        });
        mod.raw_function("request", [](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
            if (args.size() < 2 || args.size() > 3 || args[0].tag != pyle::Value::Tag::StringRef) {
                m.runtime_error(pyle::RuntimeError::ArgumentError,
                    "http.request expects (method: string, url: string, options = none).");
                return pyle::Value();
            }
            std::string method = pyle::from_value<std::string>(m, args[0]);
            for (auto& c : method) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
            return run_url_request(m, method, &args[1], args.size() > 2 ? &args[2] : nullptr);
        });

        mod.raw_function("url_encode", native_url_encode);
        mod.raw_function("url_decode", native_url_decode);

        mod.raw_function("cancel", [](pyle::VM&, pyle::ArgView) -> pyle::Value {
            cancel_clients();
            return pyle::Value();
        });

        {
            pyle::ClassBinder<HttpClientWrapper> binder(mod.get_vm(), "Client");
            binder.custom_constructor([](pyle::VM& m, pyle::ArgView args) -> pyle::Value {
                if (args.size() != 1 || args[0].tag != pyle::Value::Tag::StringRef) {
                    m.runtime_error(pyle::RuntimeError::ArgumentError, "http.Client expects a base url string.");
                    return pyle::Value();
                }
                const std::string& base = std::get<std::string>(m.get_heap_object(args[0].as_ref).data);
                ParsedUrl parsed;
                if (!parse_url(base, parsed)) {
                    m.runtime_error(pyle::RuntimeError::Runtime, "http.Client invalid base url: '" + base + "'.");
                    return pyle::Value();
                }
                auto* wrapper = new HttpClientWrapper(parsed);
                if (!wrapper->client && parsed.https) {
                    delete wrapper;
                    m.runtime_error(pyle::RuntimeError::Runtime,
                        "https requires an ssl-enabled build of the http module.");
                    return pyle::Value();
                }
                return pyle::to_value_owned(m, wrapper);
            });

            auto bind_verb = [&](const char* name, pyle::NativeMethodFn thunk) {
                binder.custom_method(name, thunk);
            };
            bind_verb("get", client_get_thunk);
            bind_verb("post", client_post_thunk);
            bind_verb("put", client_put_thunk);
            bind_verb("patch", client_patch_thunk);
            bind_verb("delete", client_delete_thunk);
            bind_verb("head", client_head_thunk);
            bind_verb("options", client_options_thunk);
            bind_verb("get_async", client_get_async_thunk);
            bind_verb("post_async", client_post_async_thunk);
            bind_verb("put_async", client_put_async_thunk);
            bind_verb("patch_async", client_patch_async_thunk);
            bind_verb("delete_async", client_delete_async_thunk);
            bind_verb("head_async", client_head_async_thunk);
            bind_verb("options_async", client_options_async_thunk);

            binder.custom_method("set_header", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView args) -> pyle::Value {
                if (args.size() != 2 || args[0].tag != pyle::Value::Tag::StringRef || args[1].tag != pyle::Value::Tag::StringRef) {
                    m.runtime_error(pyle::RuntimeError::ArgumentError, "set_header expects (name: string, value: string).");
                    return pyle::Value();
                }
                auto& native = std::get<pyle::NativeObject>(m.get_heap_object(self).data);
                auto* wrapper = static_cast<HttpClientWrapper*>(native.ptr);
                wrapper->default_headers.insert({pyle::from_value<std::string>(m, args[0]),
                                                 pyle::from_value<std::string>(m, args[1])});
                return pyle::Value();
            });

            binder.custom_method("set_timeout", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView args) -> pyle::Value {
                if (args.size() != 1 || (args[0].tag != pyle::Value::Tag::Int && args[0].tag != pyle::Value::Tag::Float)) {
                    m.runtime_error(pyle::RuntimeError::ArgumentError, "set_timeout expects seconds as a number.");
                    return pyle::Value();
                }
                auto& native = std::get<pyle::NativeObject>(m.get_heap_object(self).data);
                auto* wrapper = static_cast<HttpClientWrapper*>(native.ptr);
                double timeout = (args[0].tag == pyle::Value::Tag::Int)
                    ? static_cast<double>(args[0].as_int) : args[0].as_float;
                if (wrapper->client) {
                    wrapper->client->set_connection_timeout(std::chrono::duration<double>(timeout));
                    wrapper->client->set_read_timeout(std::chrono::duration<double>(timeout));
                }
                return pyle::Value();
            });

            mod.class_binder(binder);
        }

        {
            pyle::ClassBinder<HttpResponseData> binder(mod.get_vm(), "Response");
            binder.custom_getter("status", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto* d = static_cast<HttpResponseData*>(std::get<pyle::NativeObject>(m.get_heap_object(self).data).ptr);
                return pyle::Value(d->status);
            });
            binder.custom_getter("ok", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto* d = static_cast<HttpResponseData*>(std::get<pyle::NativeObject>(m.get_heap_object(self).data).ptr);
                return pyle::Value(d->status >= 200 && d->status < 400);
            });
            binder.custom_getter("body", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto* d = static_cast<HttpResponseData*>(std::get<pyle::NativeObject>(m.get_heap_object(self).data).ptr);
                return pyle::to_transient_string(m, d->body);
            });
            binder.custom_getter("headers", [](pyle::VM& m, pyle::HeapIdx self, pyle::ArgView) -> pyle::Value {
                auto* d = static_cast<HttpResponseData*>(std::get<pyle::NativeObject>(m.get_heap_object(self).data).ptr);
                GcDisable gc(m);
                MapType out;
                for (const auto& [k, v] : d->headers) {
                    Value key(Value::Tag::StringRef, m.intern_string(k));
                    auto it = out.find(key);
                    if (it == out.end()) {
                        out[key] = pyle::to_transient_string(m, v);
                    } else if (it->second.tag == Value::Tag::ArrayRef) {
                        std::get<ArrayType>(m.get_heap_object(it->second.as_ref).data).push_back(pyle::to_transient_string(m, v));
                    } else {
                        ArrayType arr;
                        arr.push_back(it->second);
                        arr.push_back(pyle::to_transient_string(m, v));
                        it->second = Value(Value::Tag::ArrayRef, m.alloc(Object(std::move(arr))));
                    }
                }
                return Value(Value::Tag::MapRef, m.alloc(Object(std::move(out))));
            });
            mod.class_binder(binder);
        }
    }

}
}

