#include "pyle/std/std_proc.hpp"
#include "pyle/std/std_process.hpp"
#include "pyle/binder.hpp"
#include "pyle/value.hpp"
#include "pyle/vm.hpp"
#include "pyle/std/std_future.hpp"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <cwctype>
#include <windows.h>
#else
#include <csignal>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace pyle {
namespace proc {

constexpr int64_t kDefaultReadMax = 65536;
constexpr size_t kDrainChunk = 8192;

long read_available(RawHandle h, char* dst, size_t n) {
    if (n == 0) return 0;
#ifdef _WIN32
    HANDLE fh = static_cast<HANDLE>(h);
    DWORD avail = 0;
    if (!PeekNamedPipe(fh, nullptr, 0, nullptr, &avail, nullptr)) {
        return GetLastError() == ERROR_BROKEN_PIPE ? -1 : -2;
    }
    if (avail == 0) return 0;
    DWORD want = avail > static_cast<DWORD>(n) ? static_cast<DWORD>(n) : avail;
    DWORD got = 0;
    if (!ReadFile(fh, dst, want, &got, nullptr)) return -2;
    if (got == 0) return -1;
    return static_cast<long>(got);
#else
    for (;;) {
        ssize_t r = read(h, dst, n);
        if (r > 0) return static_cast<long>(r);
        if (r == 0) return -1;
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        return -2;
    }
#endif
}

namespace {

const std::string& str_of(VM& vm, const Value& v) {
    return std::get<std::string>(vm.get_heap_object(v.as_ref).data);
}

const BytesType& bytes_of(VM& vm, const Value& v) {
    return std::get<BytesType>(vm.get_heap_object(v.as_ref).data);
}

Value bytes_value(VM& vm, const std::string& s) {
    BytesType b(s.begin(), s.end());
    return Value(Value::Tag::BytesRef, vm.alloc(Object(std::move(b))));
}

Value map_key(VM& vm, const char* name) {
    return Value(Value::Tag::StringRef, vm.intern_string(name));
}

struct SpawnOptions {
    std::vector<std::string> args;
    std::string cwd;
    std::vector<std::pair<std::string, std::optional<std::string>>> env;
    bool clear_env = false;
    std::string input;
    bool has_input = false;
    int64_t timeout_ms = -1;
};

struct Child {
    std::mutex wmutex;
    std::mutex out_mutex;
    std::mutex err_mutex;
    RawHandle stdin_h = kBadHandle;
    RawHandle stdout_h = kBadHandle;
    RawHandle stderr_h = kBadHandle;
    bool stdin_open = true;
    bool out_eof = false;
    bool err_eof = false;
#ifdef _WIN32
    HANDLE proc = nullptr;
    int64_t pid_value = -1;
#else
    pid_t pid = -1;
    int64_t exit_code = -1;
    bool exited = false;
#endif

    ~Child() {
        handle_close(stdin_h);
        handle_close(stdout_h);
        handle_close(stderr_h);
#ifdef _WIN32
        if (proc) CloseHandle(proc);
#else
        if (pid > 0 && !exited) {
            int st = 0;
            if (waitpid(pid, &st, WNOHANG) > 0) {
                exited = true;
                exit_code = normalize_status(st);
            }
        }
#endif
    }

    int64_t get_pid() {
#ifdef _WIN32
        return pid_value;
#else
        return static_cast<int64_t>(pid);
#endif
    }

    bool write_stdin(const char* data, size_t n) {
        std::lock_guard<std::mutex> lock(wmutex);
        if (!stdin_open || !handle_valid(stdin_h)) return false;
        return write_all(stdin_h, data, n);
    }

    bool close_stdin() {
        std::lock_guard<std::mutex> lock(wmutex);
        if (stdin_open) {
            stdin_open = false;
            handle_close(stdin_h);
            stdin_h = kBadHandle;
        }
        return true;
    }

    struct ReadOut {
        bool timed_out = false;
        std::string data;
    };

    ReadOut read_stream(bool use_stderr, size_t max_bytes, int64_t timeout_ms) {
        std::lock_guard<std::mutex> lock(use_stderr ? err_mutex : out_mutex);
        RawHandle h = use_stderr ? stderr_h : stdout_h;
        bool& eof = use_stderr ? err_eof : out_eof;
        ReadOut out;
        if (!handle_valid(h) || eof || max_bytes == 0) return out;
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms < 0 ? 0 : timeout_ms);
        std::string data;
        data.reserve(max_bytes < kDrainChunk ? max_bytes : kDrainChunk);
        for (;;) {
            while (data.size() < max_bytes) {
                size_t want = max_bytes - data.size();
                if (want > kDrainChunk) want = kDrainChunk;
                char buf[kDrainChunk];
                long r = read_available(h, buf, want);
                if (r > 0) {
                    data.append(buf, static_cast<size_t>(r));
                    continue;
                }
                if (r == 0) break;
                eof = true;
                break;
            }
            if (!data.empty() || eof) {
                out.data = std::move(data);
                return out;
            }
            if (timeout_ms == 0) {
                out.timed_out = true;
                return out;
            }
            int64_t left = -1;
            if (timeout_ms > 0) {
                left = std::chrono::duration_cast<std::chrono::milliseconds>(
                           deadline - std::chrono::steady_clock::now())
                           .count();
                if (left <= 0) {
                    out.timed_out = true;
                    return out;
                }
            }
            if (!await_readable(h, timeout_ms < 0 ? -1 : left)) {
                out.timed_out = true;
                return out;
            }
        }
    }

    static int64_t normalize_status(int st) {
#ifdef _WIN32
        return static_cast<int64_t>(st);
#else
        if (WIFEXITED(st)) return static_cast<int64_t>(WEXITSTATUS(st));
        if (WIFSIGNALED(st)) return static_cast<int64_t>(-WTERMSIG(st));
        return -1;
#endif
    }

    std::optional<int64_t> poll() { return wait_ms(0); }

    std::optional<int64_t> wait_ms(int64_t timeout_ms) {
#ifdef _WIN32
        if (!proc) return std::nullopt;
        DWORD t = timeout_ms < 0 ? INFINITE
                  : timeout_ms > 4294967294LL ? 4294967294u
                                              : static_cast<DWORD>(timeout_ms);
        if (WaitForSingleObject(proc, t) != WAIT_OBJECT_0) return std::nullopt;
        DWORD code = 0;
        if (!GetExitCodeProcess(proc, &code)) return std::nullopt;
        return static_cast<int64_t>(code);
#else
        if (pid <= 0) return std::nullopt;
        if (exited) return exit_code;
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms < 0 ? 0 : timeout_ms);
        for (;;) {
            int st = 0;
            pid_t r = waitpid(pid, &st, WNOHANG);
            if (r > 0) {
                exited = true;
                exit_code = normalize_status(st);
                return exit_code;
            }
            if (r < 0) return std::nullopt;
            if (timeout_ms == 0) return std::nullopt;
            if (timeout_ms > 0 && std::chrono::steady_clock::now() >= deadline) return std::nullopt;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
#endif
    }

    bool terminate() {
#ifdef _WIN32
        return proc && TerminateProcess(proc, 1);
#else
        return pid > 0 && kill(pid, SIGTERM) == 0;
#endif
    }

    bool force_kill() {
#ifdef _WIN32
        return proc && TerminateProcess(proc, 1);
#else
        return pid > 0 && kill(pid, SIGKILL) == 0;
#endif
    }
};

struct CommResult {
    bool timed_out = false;
    std::optional<int64_t> code;
    std::string out;
    std::string err;
};

void drain_to_eof(RawHandle h, std::string& out) {
    char buf[kDrainChunk];
#ifdef _WIN32
    HANDLE fh = static_cast<HANDLE>(h);
    for (;;) {
        DWORD avail = 0;
        if (!PeekNamedPipe(fh, nullptr, 0, nullptr, &avail, nullptr)) return;
        if (avail == 0) {
            Sleep(1);
            continue;
        }
        DWORD got = 0;
        if (!ReadFile(fh, buf, sizeof(buf), &got, nullptr) || got == 0) return;
        out.append(buf, got);
    }
#else
    for (;;) {
        pollfd p{h, POLLIN, 0};
        int r = poll(&p, 1, -1);
        if (r < 0) {
            if (errno == EINTR) continue;
            return;
        }
        ssize_t n = read(h, buf, sizeof(buf));
        if (n > 0) {
            out.append(buf, static_cast<size_t>(n));
            continue;
        }
        if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        return;
    }
#endif
}

CommResult communicate_with(Child& c, const std::string* input, int64_t timeout_ms) {
    CommResult res;
    std::thread in_thread([&]() {
        if (input) c.write_stdin(input->data(), input->size());
        c.close_stdin();
    });
    std::thread out_thread(drain_to_eof, c.stdout_h, std::ref(res.out));
    std::thread err_thread(drain_to_eof, c.stderr_h, std::ref(res.err));
    std::optional<int64_t> code = c.wait_ms(timeout_ms);
    if (!code) {
        res.timed_out = true;
        c.force_kill();
        code = c.wait_ms(-1);
    }
    in_thread.join();
    out_thread.join();
    err_thread.join();
    if (!res.timed_out) res.code = code;
    return res;
}

#ifdef _WIN32
std::vector<wchar_t> make_proc_env_block(const SpawnOptions& opt) {
    std::vector<std::pair<std::wstring, std::wstring>> kept;
    auto overridden = [&](const std::wstring& name) {
        for (auto& kv : opt.env) {
            const std::string& oname = kv.first;
            if (oname.size() != name.size()) continue;
            bool same = true;
            for (size_t i = 0; i < oname.size(); ++i) {
                if (towlower(static_cast<wint_t>(oname[i])) != towlower(name[i])) {
                    same = false;
                    break;
                }
            }
            if (same) return true;
        }
        return false;
    };
    if (!opt.clear_env) {
        LPWCH env = GetEnvironmentStringsW();
        if (env) {
            for (LPWCH p = env; *p;) {
                size_t n = wcslen(p) + 1;
                const wchar_t* eq = wcschr(p, L'=');
                if (eq && eq != p) {
                    std::wstring name(p, static_cast<size_t>(eq - p));
                    if (!overridden(name)) kept.emplace_back(name, std::wstring(eq + 1));
                }
                p += n;
            }
            FreeEnvironmentStringsW(env);
        }
    }
    for (auto& kv : opt.env) {
        if (kv.second) kept.emplace_back(widen(kv.first), widen(*kv.second));
    }
    std::vector<wchar_t> block;
    for (auto& kv : kept) {
        std::wstring entry = kv.first + L'=' + kv.second;
        block.insert(block.end(), entry.c_str(), entry.c_str() + entry.size() + 1);
    }
    block.push_back(L'\0');
    return block;
}

bool spawn_child_process(const std::string& program, const SpawnOptions& opt, Child*& out, std::string& err) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE in_r = nullptr, in_w = nullptr, out_r = nullptr, out_w = nullptr, err_r = nullptr, err_w = nullptr;
    if (!CreatePipe(&in_r, &in_w, &sa, 0) || !CreatePipe(&out_r, &out_w, &sa, 0) ||
        !CreatePipe(&err_r, &err_w, &sa, 0)) {
        err = "proc.popen could not create pipes.";
        handle_close(in_r);
        handle_close(in_w);
        handle_close(out_r);
        handle_close(out_w);
        handle_close(err_r);
        handle_close(err_w);
        return false;
    }
    SetHandleInformation(in_w, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(out_r, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(err_r, HANDLE_FLAG_INHERIT, 0);
    std::string cmd = quote_arg(program);
    for (auto& a : opt.args) {
        cmd += ' ';
        cmd += quote_arg(a);
    }
    std::wstring wcmd = widen(cmd);
    std::vector<wchar_t> cmdbuf(wcmd.begin(), wcmd.end());
    cmdbuf.push_back(L'\0');
    std::vector<wchar_t> env = make_proc_env_block(opt);
    const wchar_t* cwd = nullptr;
    std::wstring wcwd;
    if (!opt.cwd.empty()) {
        wcwd = widen(opt.cwd);
        cwd = wcwd.c_str();
    }
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput = in_r;
    si.hStdOutput = out_w;
    si.hStdError = err_w;
    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessW(nullptr, cmdbuf.data(), nullptr, nullptr, TRUE, CREATE_UNICODE_ENVIRONMENT,
                              env.data(), cwd, &si, &pi);
    handle_close(in_r);
    handle_close(out_w);
    handle_close(err_w);
    if (!ok) {
        handle_close(in_w);
        handle_close(out_r);
        handle_close(err_r);
        err = last_error_string(("proc.popen could not start '" + program + "'.").c_str());
        return false;
    }
    CloseHandle(pi.hThread);
    Child* c = new Child();
    c->stdin_h = in_w;
    c->stdout_h = out_r;
    c->stderr_h = err_r;
    c->proc = pi.hProcess;
    c->pid_value = static_cast<int64_t>(pi.dwProcessId);
    out = c;
    return true;
}
#else
bool spawn_child_process(const std::string& program, const SpawnOptions& opt, Child*& out, std::string& err) {
    int in_p[2] = {-1, -1};
    int out_p[2] = {-1, -1};
    int err_p[2] = {-1, -1};
    int failures[2] = {-1, -1};
    if (pipe(in_p) != 0 || pipe(out_p) != 0 || pipe(err_p) != 0 || pipe(failures) != 0) {
        err = "proc.popen could not create pipes.";
        handle_close(in_p[0]);
        handle_close(in_p[1]);
        handle_close(out_p[0]);
        handle_close(out_p[1]);
        handle_close(err_p[0]);
        handle_close(err_p[1]);
        handle_close(failures[0]);
        handle_close(failures[1]);
        return false;
    }
    fcntl(failures[0], F_SETFD, FD_CLOEXEC);
    fcntl(failures[1], F_SETFD, FD_CLOEXEC);
    fcntl(out_p[0], F_SETFL, O_NONBLOCK);
    fcntl(err_p[0], F_SETFL, O_NONBLOCK);
    std::vector<std::string> env_strings;
    if (opt.clear_env) {
        env_strings.reserve(opt.env.size());
    } else {
        for (char** e = environ; e && *e; ++e) env_strings.emplace_back(*e);
        env_strings.reserve(env_strings.size() + opt.env.size());
    }
    for (auto& kv : opt.env) {
        if (kv.second) {
            env_strings.push_back(kv.first + "=" + *kv.second);
        } else {
            std::string prefix = kv.first + "=";
            for (auto it = env_strings.begin(); it != env_strings.end();) {
                if (it->compare(0, prefix.size(), prefix) == 0) {
                    it = env_strings.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    std::vector<char*> envp;
    envp.reserve(env_strings.size() + 1);
    for (auto& s : env_strings) envp.push_back(s.data());
    envp.push_back(nullptr);
    std::vector<char*> argv;
    argv.reserve(opt.args.size() + 2);
    argv.push_back(const_cast<char*>(program.c_str()));
    for (auto& a : opt.args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);
    static std::once_flag pipe_flag;
    std::call_once(pipe_flag, []() { signal(SIGPIPE, SIG_IGN); });
    pid_t pid = fork();
    if (pid < 0) {
        err = "proc.popen could not fork child process.";
        handle_close(in_p[0]);
        handle_close(in_p[1]);
        handle_close(out_p[0]);
        handle_close(out_p[1]);
        handle_close(err_p[0]);
        handle_close(err_p[1]);
        handle_close(failures[0]);
        handle_close(failures[1]);
        return false;
    }
    if (pid == 0) {
        signal(SIGPIPE, SIG_DFL);
        close(in_p[1]);
        close(out_p[0]);
        close(err_p[0]);
        close(failures[0]);
        dup2(in_p[0], STDIN_FILENO);
        dup2(out_p[1], STDOUT_FILENO);
        dup2(err_p[1], STDERR_FILENO);
        close(in_p[0]);
        close(out_p[1]);
        close(err_p[1]);
        if (!opt.cwd.empty() && chdir(opt.cwd.c_str()) != 0) {
            int code = errno;
            ssize_t w = write(failures[1], &code, sizeof(code));
            (void)w;
            _exit(127);
        }
        execvpe(program.c_str(), argv.data(), envp.data());
        int code = errno;
        ssize_t w = write(failures[1], &code, sizeof(code));
        (void)w;
        _exit(127);
    }
    close(in_p[0]);
    close(out_p[1]);
    close(err_p[1]);
    close(failures[1]);
    int child_errno = 0;
    size_t got = 0;
    while (got < sizeof(child_errno)) {
        ssize_t r = read(failures[0], reinterpret_cast<char*>(&child_errno) + got, sizeof(child_errno) - got);
        if (r > 0) {
            got += static_cast<size_t>(r);
        } else if (r == 0) {
            break;
        } else if (errno != EINTR) {
            break;
        }
    }
    close(failures[0]);
    if (got != 0) {
        int st = 0;
        while (waitpid(pid, &st, 0) < 0 && errno == EINTR) {
        }
        handle_close(in_p[1]);
        handle_close(out_p[0]);
        handle_close(err_p[0]);
        if (got == sizeof(child_errno)) {
            err = "proc.popen could not start '" + program + "': " + strerror(child_errno) + ".";
        } else {
            err = "proc.popen could not start '" + program + "'.";
        }
        return false;
    }
    Child* c = new Child();
    c->stdin_h = in_p[1];
    c->stdout_h = out_p[0];
    c->stderr_h = err_p[0];
    c->pid = pid;
    out = c;
    return true;
}
#endif

bool read_input_data(VM& vm, const Value& v, std::string& out, const char* what) {
    if (v.tag == Value::Tag::StringRef) {
        out = str_of(vm, v);
        return true;
    }
    if (v.tag == Value::Tag::BytesRef) {
        const BytesType& b = bytes_of(vm, v);
        out.assign(b.begin(), b.end());
        return true;
    }
    vm.runtime_error(RuntimeError::ArgumentError, what);
    return false;
}

bool parse_timeout(VM& vm, const Value& v, int64_t& out, const char* what) {
    if (v.tag == Value::Tag::Int) {
        out = v.as_int;
    } else if (v.tag == Value::Tag::Float) {
        out = static_cast<int64_t>(v.as_float);
    } else {
        vm.runtime_error(RuntimeError::ArgumentError, what);
        return false;
    }
    if (out < -1) {
        vm.runtime_error(RuntimeError::ArgumentError, what);
        return false;
    }
    return true;
}

bool parse_spawn_args(VM& vm, ArgView args, const char* name, std::string& program, SpawnOptions& opt) {
    if (args.size() < 1 || args.size() > 3 || args[0].tag != Value::Tag::StringRef) {
        vm.runtime_error(RuntimeError::ArgumentError,
                         std::string(name) + " expects (program: string, args = [], options = {}).");
        return false;
    }
    program = str_of(vm, args[0]);
    if (program.empty()) {
        vm.runtime_error(RuntimeError::ArgumentError, std::string(name) + " program must not be empty.");
        return false;
    }
    if (args.size() >= 2) {
        if (args[1].tag != Value::Tag::ArrayRef) {
            vm.runtime_error(RuntimeError::ArgumentError, std::string(name) + " args expects an array of strings.");
            return false;
        }
        const auto& arr = std::get<ArrayType>(vm.get_heap_object(args[1].as_ref).data);
        for (const auto& v : arr) {
            if (v.tag != Value::Tag::StringRef) {
                vm.runtime_error(RuntimeError::ArgumentError,
                                 std::string(name) + " args expects an array of strings.");
                return false;
            }
            opt.args.push_back(str_of(vm, v));
        }
    }
    if (args.size() >= 3) {
        if (args[2].tag != Value::Tag::MapRef) {
            vm.runtime_error(RuntimeError::ArgumentError, std::string(name) + " options expects a map.");
            return false;
        }
        const auto& opts = std::get<MapObject>(vm.get_heap_object(args[2].as_ref).data).entries;
        for (const auto& kv : opts) {
            if (kv.first.tag != Value::Tag::StringRef) {
                vm.runtime_error(RuntimeError::ArgumentError, std::string(name) + " options expects string keys.");
                return false;
            }
            const std::string& key = str_of(vm, kv.first);
            const Value& val = kv.second;
            if (key == "cwd") {
                if (val.tag != Value::Tag::StringRef) {
                    vm.runtime_error(RuntimeError::ArgumentError,
                                     std::string(name) + " option 'cwd' expects a string.");
                    return false;
                }
                opt.cwd = str_of(vm, val);
            } else if (key == "env") {
                if (val.tag != Value::Tag::MapRef) {
                    vm.runtime_error(RuntimeError::ArgumentError,
                                     std::string(name) + " option 'env' expects a map.");
                    return false;
                }
                const auto& env = std::get<MapObject>(vm.get_heap_object(val.as_ref).data).entries;
                for (const auto& entry : env) {
                    if (entry.first.tag != Value::Tag::StringRef) {
                        vm.runtime_error(RuntimeError::ArgumentError,
                                         std::string(name) + " option 'env' expects string keys.");
                        return false;
                    }
                    std::string ename = str_of(vm, entry.first);
                    if (ename.empty() || ename.find('=') != std::string::npos) {
                        vm.runtime_error(RuntimeError::ArgumentError,
                                         std::string(name) + " option 'env' has an invalid variable name.");
                        return false;
                    }
                    if (entry.second.tag == Value::Tag::StringRef) {
                        opt.env.emplace_back(std::move(ename), str_of(vm, entry.second));
                    } else if (entry.second.tag == Value::Tag::None) {
                        opt.env.emplace_back(std::move(ename), std::nullopt);
                    } else {
                        vm.runtime_error(RuntimeError::ArgumentError,
                                         std::string(name) + " option 'env' values expect a string or none.");
                        return false;
                    }
                }
            } else if (key == "clear_env") {
                if (val.tag != Value::Tag::Bool) {
                    vm.runtime_error(RuntimeError::ArgumentError,
                                     std::string(name) + " option 'clear_env' expects a bool.");
                    return false;
                }
                opt.clear_env = val.as_bool;
            } else if (key == "stdin") {
                if (!read_input_data(vm, val, opt.input,
                                     (std::string(name) + " option 'stdin' expects a string or bytes.").c_str())) {
                    return false;
                }
                opt.has_input = true;
            } else if (key == "timeout_ms") {
                if (!parse_timeout(vm, val, opt.timeout_ms,
                                   (std::string(name) + " option 'timeout_ms' expects an int >= -1.").c_str())) {
                    return false;
                }
            } else {
                vm.runtime_error(RuntimeError::ArgumentError,
                                 std::string(name) + " has no option '" + key + "'.");
                return false;
            }
        }
    }
    return true;
}

bool spawn_parsed(VM& vm, const char* name, const std::string& program, const SpawnOptions& opt,
                  std::shared_ptr<Child>& out) {
    Child* raw = nullptr;
    std::string err;
    if (!spawn_child_process(program, opt, raw, err)) {
        vm.runtime_error(RuntimeError::Runtime, err);
        return false;
    }
    out = std::shared_ptr<Child>(raw);
    return true;
}

Value result_value(VM& vm, const CommResult& res) {
    MapType m;
    m[map_key(vm, "code")] = res.code ? Value(*res.code) : Value();
    m[map_key(vm, "stdout")] = bytes_value(vm, res.out);
    m[map_key(vm, "stderr")] = bytes_value(vm, res.err);
    m[map_key(vm, "timed_out")] = Value(res.timed_out);
    return Value(Value::Tag::MapRef, vm.alloc(Object(std::move(m))));
}

Value proc_run(VM& vm, ArgView args) {
    std::string program;
    SpawnOptions opt;
    if (!parse_spawn_args(vm, args, "proc.run", program, opt)) return Value();
    std::shared_ptr<Child> child;
    if (!spawn_parsed(vm, "proc.run", program, opt, child)) return Value();
    const std::string* input = opt.has_input ? &opt.input : nullptr;
    CommResult res = communicate_with(*child, input, opt.timeout_ms);
    return result_value(vm, res);
}

Value proc_popen(VM& vm, ArgView args) {
    std::string program;
    SpawnOptions opt;
    if (!parse_spawn_args(vm, args, "proc.popen", program, opt)) return Value();
    if (opt.has_input || opt.timeout_ms >= 0) {
        vm.runtime_error(RuntimeError::ArgumentError,
                         "proc.popen takes no 'stdin' or 'timeout_ms' option, use Child.communicate.");
        return Value();
    }
    std::shared_ptr<Child> child;
    if (!spawn_parsed(vm, "proc.popen", program, opt, child)) return Value();
    return to_value_owned(vm, new std::shared_ptr<Child>(child));
}

struct PendingRun {
    VM* vm = nullptr;
    std::shared_ptr<Future> fut;
    std::shared_ptr<Child> child;
    std::string input;
    bool has_input = false;
    int64_t timeout_ms = -1;
    CommResult result;
    std::atomic<bool> done{false};
};

std::mutex& run_mutex() {
    static std::mutex m;
    return m;
}

std::vector<std::shared_ptr<PendingRun>>& pending_runs() {
    static std::vector<std::shared_ptr<PendingRun>> v;
    return v;
}

Value run_tick(VM& vm, ArgView) {
    std::vector<std::shared_ptr<PendingRun>> ready;
    {
        std::lock_guard<std::mutex> lock(run_mutex());
        auto& v = pending_runs();
        for (auto it = v.begin(); it != v.end();) {
            if ((*it)->vm == &vm && (*it)->done.load()) {
                ready.push_back(std::move(*it));
                it = v.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto& run : ready) run->fut->resolve(result_value(vm, run->result));
    return Value();
}

Value proc_run_async(VM& vm, ArgView args) {
    std::string program;
    SpawnOptions opt;
    if (!parse_spawn_args(vm, args, "proc.run_async", program, opt)) return Value();
    std::shared_ptr<Child> child;
    if (!spawn_parsed(vm, "proc.run_async", program, opt, child)) return Value();
    auto [fval, fut] = Future::create(vm);
    auto run = std::make_shared<PendingRun>();
    run->vm = &vm;
    run->fut = fut;
    run->child = child;
    run->input = std::move(opt.input);
    run->has_input = opt.has_input;
    run->timeout_ms = opt.timeout_ms;
    fut->set_cancel_hook([child]() { child->force_kill(); });
    {
        std::lock_guard<std::mutex> lock(run_mutex());
        pending_runs().push_back(run);
    }
    std::thread([run]() {
        const std::string* input = run->has_input ? &run->input : nullptr;
        run->result = communicate_with(*run->child, input, run->timeout_ms);
        run->done.store(true);
    }).detach();
    return fval;
}

Child* child_of(VM& m, HeapIdx self) {
    auto* sp = static_cast<std::shared_ptr<Child>*>(std::get<NativeObject>(m.get_heap_object(self).data).ptr);
    return sp->get();
}

bool parse_read_args(VM& m, ArgView args, const char* name, size_t& max_bytes, int64_t& timeout_ms) {
    max_bytes = static_cast<size_t>(kDefaultReadMax);
    timeout_ms = -1;
    if (args.size() > 2) {
        m.runtime_error(RuntimeError::ArgumentError,
                        std::string(name) + " expects (max_bytes = 65536, timeout_ms = -1).");
        return false;
    }
    if (args.size() >= 1) {
        if (args[0].tag != Value::Tag::Int && args[0].tag != Value::Tag::Float) {
            m.runtime_error(RuntimeError::ArgumentError, std::string(name) + " max_bytes expects an int.");
            return false;
        }
        int64_t v = args[0].tag == Value::Tag::Int ? args[0].as_int : static_cast<int64_t>(args[0].as_float);
        if (v <= 0) {
            m.runtime_error(RuntimeError::ArgumentError, std::string(name) + " max_bytes expects a positive int.");
            return false;
        }
        max_bytes = static_cast<size_t>(v);
    }
    if (args.size() >= 2 && !parse_timeout(m, args[1], timeout_ms,
                                           (std::string(name) + " timeout_ms expects an int >= -1.").c_str())) {
        return false;
    }
    return !m.is_panicked();
}

Value read_result(VM& vm, const Child::ReadOut& out) {
    if (out.timed_out) return Value();
    return bytes_value(vm, out.data);
}

}  // namespace

Value proc_module_factory(VM& vm) {
    NativeModule mod(vm, "proc");
    mod.raw_function("run", proc_run);
    mod.raw_function("popen", proc_popen);
    mod.raw_function("run_async", proc_run_async);

    static std::atomic<bool> tick_registered{false};
    if (!tick_registered.exchange(true)) background_ticks().push_back(run_tick);

    SharedClassBinder<Child> binder(vm, "Child");
    binder.custom_constructor([](VM& m, ArgView) -> Value {
        m.runtime_error(RuntimeError::ArgumentError, "Child cannot be constructed directly, use proc.popen.");
        return Value();
    });
    binder.custom_method("pid", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() != 0) {
            m.runtime_error(RuntimeError::ArgumentError, "pid takes 0 arguments.");
            return Value();
        }
        return Value(child_of(m, self)->get_pid());
    });
    binder.custom_method("alive", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() != 0) {
            m.runtime_error(RuntimeError::ArgumentError, "alive takes 0 arguments.");
            return Value();
        }
        return Value(!child_of(m, self)->poll().has_value());
    });
    binder.custom_method("poll", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() != 0) {
            m.runtime_error(RuntimeError::ArgumentError, "poll takes 0 arguments.");
            return Value();
        }
        std::optional<int64_t> code = child_of(m, self)->poll();
        if (!code) return Value();
        return Value(*code);
    });
    binder.custom_method("wait", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() > 1) {
            m.runtime_error(RuntimeError::ArgumentError, "wait expects (timeout_ms = -1).");
            return Value();
        }
        int64_t timeout_ms = -1;
        if (args.size() != 0 && !parse_timeout(m, args[0], timeout_ms, "wait timeout_ms expects an int >= -1.")) {
            return Value();
        }
        if (m.is_panicked()) return Value();
        std::optional<int64_t> code = child_of(m, self)->wait_ms(timeout_ms);
        if (!code) return Value();
        return Value(*code);
    });
    binder.custom_method("terminate", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() != 0) {
            m.runtime_error(RuntimeError::ArgumentError, "terminate takes 0 arguments.");
            return Value();
        }
        return Value(child_of(m, self)->terminate());
    });
    binder.custom_method("kill", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() != 0) {
            m.runtime_error(RuntimeError::ArgumentError, "kill takes 0 arguments.");
            return Value();
        }
        return Value(child_of(m, self)->force_kill());
    });
    binder.custom_method("stdin_write", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() != 1) {
            m.runtime_error(RuntimeError::ArgumentError, "stdin_write expects (data: string | bytes).");
            return Value();
        }
        std::string data;
        if (!read_input_data(m, args[0], data, "stdin_write expects (data: string | bytes).")) return Value();
        return Value(child_of(m, self)->write_stdin(data.data(), data.size()));
    });
    binder.custom_method("stdin_close", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() != 0) {
            m.runtime_error(RuntimeError::ArgumentError, "stdin_close takes 0 arguments.");
            return Value();
        }
        return Value(child_of(m, self)->close_stdin());
    });
    binder.custom_method("stdout_read", [](VM& m, HeapIdx self, ArgView args) -> Value {
        size_t max_bytes = 0;
        int64_t timeout_ms = -1;
        if (!parse_read_args(m, args, "stdout_read", max_bytes, timeout_ms)) return Value();
        return read_result(m, child_of(m, self)->read_stream(false, max_bytes, timeout_ms));
    });
    binder.custom_method("stderr_read", [](VM& m, HeapIdx self, ArgView args) -> Value {
        size_t max_bytes = 0;
        int64_t timeout_ms = -1;
        if (!parse_read_args(m, args, "stderr_read", max_bytes, timeout_ms)) return Value();
        return read_result(m, child_of(m, self)->read_stream(true, max_bytes, timeout_ms));
    });
    binder.custom_method("communicate", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() > 2) {
            m.runtime_error(RuntimeError::ArgumentError, "communicate expects (input = none, timeout_ms = -1).");
            return Value();
        }
        const std::string* input = nullptr;
        std::string owned;
        if (args.size() != 0 && args[0].tag != Value::Tag::None) {
            if (!read_input_data(m, args[0], owned, "communicate input expects a string, bytes, or none.")) {
                return Value();
            }
            input = &owned;
        }
        int64_t timeout_ms = -1;
        if (args.size() == 2 && !parse_timeout(m, args[1], timeout_ms, "communicate timeout_ms expects an int >= -1.")) {
            return Value();
        }
        if (m.is_panicked()) return Value();
        return result_value(m, communicate_with(*child_of(m, self), input, timeout_ms));
    });
    mod.class_binder(binder);
    return mod.build();
}

}  // namespace proc

void register_proc_module(VM& vm) {
    register_module(vm, "proc", proc::proc_module_factory);
}

}  // namespace pyle
