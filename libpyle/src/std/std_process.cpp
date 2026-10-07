#include "pyle/std/std_process.hpp"
#include "pyle/binder.hpp"
#include "pyle/value.hpp"
#include "pyle/vm.hpp"
#include "pyle/std/std_future.hpp"
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <errno.h>
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
namespace {

const uint32_t kMaxFrame = 64u << 20;

#ifdef _WIN32
using RawHandle = HANDLE;
const RawHandle kBadHandle = nullptr;
bool handle_valid(RawHandle h) { return h != nullptr && h != INVALID_HANDLE_VALUE; }
void handle_close(RawHandle h) { if (handle_valid(h)) CloseHandle(h); }
#else
using RawHandle = int;
const RawHandle kBadHandle = -1;
bool handle_valid(RawHandle h) { return h >= 0; }
void handle_close(RawHandle h) { if (h >= 0) close(h); }
#endif

bool write_all(RawHandle h, const char* src, size_t n) {
#ifdef _WIN32
    while (n > 0) {
        DWORD chunk = n > 0x7fffffff ? 0x7fffffff : static_cast<DWORD>(n);
        DWORD done = 0;
        if (!WriteFile(h, src, chunk, &done, nullptr) || done == 0) return false;
        src += done;
        n -= done;
    }
    return true;
#else
    while (n > 0) {
        ssize_t r = write(h, src, n);
        if (r > 0) {
            src += r;
            n -= static_cast<size_t>(r);
        } else if (r < 0 && errno != EINTR) {
            return false;
        }
    }
    return true;
#endif
}

bool read_exact(RawHandle h, char* dst, size_t n) {
#ifdef _WIN32
    while (n > 0) {
        DWORD chunk = n > 0x7fffffff ? 0x7fffffff : static_cast<DWORD>(n);
        DWORD got = 0;
        if (!ReadFile(h, dst, chunk, &got, nullptr) || got == 0) return false;
        dst += got;
        n -= got;
    }
    return true;
#else
    while (n > 0) {
        ssize_t r = read(h, dst, n);
        if (r > 0) {
            dst += r;
            n -= static_cast<size_t>(r);
        } else if (r == 0) {
            return false;
        } else if (errno != EINTR) {
            return false;
        }
    }
    return true;
#endif
}

bool await_readable(RawHandle h, int64_t timeout_ms) {
#ifdef _WIN32
    auto pipe_state = [&]() -> int {
        DWORD avail = 0;
        if (PeekNamedPipe(h, nullptr, 0, nullptr, &avail, nullptr)) return avail > 0 ? 1 : 0;
        return GetLastError() == ERROR_BROKEN_PIPE ? 2 : 0;
    };
    if (timeout_ms < 0) return true;
    if (timeout_ms == 0) return pipe_state() != 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    for (;;) {
        int st = pipe_state();
        if (st != 0) return true;
        if (std::chrono::steady_clock::now() >= deadline) return false;
        Sleep(1);
    }
#else
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms < 0 ? 0 : timeout_ms);
    for (;;) {
        int t = -1;
        if (timeout_ms >= 0) {
            auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
            if (left <= 0 && timeout_ms > 0) {
                pollfd p{h, POLLIN, 0};
                return poll(&p, 1, 0) > 0;
            }
            t = left < 0 ? 0 : (left > 2000000000 ? 2000000000 : static_cast<int>(left));
        }
        pollfd p{h, POLLIN, 0};
        int r = poll(&p, 1, t);
        if (r > 0) return true;
        if (r == 0) return false;
        if (errno != EINTR) return false;
        if (timeout_ms == 0) return false;
    }
#endif
}

bool frame_send(RawHandle h, const std::string& s) {
    if (!handle_valid(h) || s.size() > kMaxFrame) return false;
    uint32_t len = static_cast<uint32_t>(s.size());
    char hdr[4] = {
        static_cast<char>(len),
        static_cast<char>(len >> 8),
        static_cast<char>(len >> 16),
        static_cast<char>(len >> 24),
    };
    if (!write_all(h, hdr, 4)) return false;
    return s.empty() || write_all(h, s.data(), s.size());
}

bool frame_recv(RawHandle h, std::string& out, int64_t timeout_ms) {
    if (!handle_valid(h)) return false;
    if (!await_readable(h, timeout_ms)) return false;
    char hdr[4];
    if (!read_exact(h, hdr, 4)) return false;
    uint32_t len = static_cast<uint8_t>(hdr[0]) | (static_cast<uint32_t>(static_cast<uint8_t>(hdr[1])) << 8) |
                   (static_cast<uint32_t>(static_cast<uint8_t>(hdr[2])) << 16) |
                   (static_cast<uint32_t>(static_cast<uint8_t>(hdr[3])) << 24);
    if (len > kMaxFrame) return false;
    out.resize(len);
    return len == 0 || read_exact(h, out.data(), len);
}

std::string encode_string_array(const std::vector<std::string>& items) {
    std::string o = "[";
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) o += ',';
        o += '"';
        for (char c : items[i]) {
            switch (c) {
                case '"': o += "\\\""; break;
                case '\\': o += "\\\\"; break;
                case '\n': o += "\\n"; break;
                case '\r': o += "\\r"; break;
                case '\t': o += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char b[7];
                        snprintf(b, sizeof(b), "\\u%04x", c);
                        o += b;
                    } else {
                        o += c;
                    }
            }
        }
        o += '"';
    }
    o += ']';
    return o;
}

bool decode_string_array(const std::string& s, std::vector<std::string>& out) {
    out.clear();
    size_t i = 0;
    auto skip = [&]() { while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i; };
    skip();
    if (i >= s.size() || s[i] != '[') return false;
    ++i;
    skip();
    if (i < s.size() && s[i] == ']') {
        ++i;
        skip();
        return i == s.size();
    }
    for (;;) {
        skip();
        if (i >= s.size() || s[i] != '"') return false;
        ++i;
        std::string cur;
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i + 1 < s.size()) {
                char e = s[i + 1];
                if (e == '"' || e == '\\' || e == '/') cur += e;
                else if (e == 'n') cur += '\n';
                else if (e == 'r') cur += '\r';
                else if (e == 't') cur += '\t';
                else return false;
                i += 2;
            } else if (s[i] == '\\') {
                return false;
            } else {
                cur += s[i++];
            }
        }
        if (i >= s.size()) return false;
        ++i;
        out.push_back(std::move(cur));
        skip();
        if (i < s.size() && s[i] == ',') {
            ++i;
            continue;
        }
        if (i < s.size() && s[i] == ']') {
            ++i;
            skip();
            return i == s.size();
        }
        return false;
    }
}

#ifdef _WIN32
std::string last_error_string(const char* what) {
    DWORD code = GetLastError();
    char* msg = nullptr;
    DWORD n = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                             nullptr, code, 0, reinterpret_cast<LPSTR>(&msg), 0, nullptr);
    std::string out = what;
    out += " (code ";
    out += std::to_string(code);
    if (n > 0 && msg) {
        out += ": ";
        out += msg;
    }
    if (msg) LocalFree(msg);
    return out;
}

std::wstring widen(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring o(static_cast<size_t>(n > 0 ? n - 1 : 0), L'\0');
    if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, o.data(), n);
    return o;
}

std::string quote_arg(const std::string& s) {
    if (s.find_first_of(" \t\"") == std::string::npos) return s;
    std::string o = "\"";
    for (char c : s) {
        if (c == '"') o += '\\';
        o += c;
    }
    o += '"';
    return o;
}

std::vector<wchar_t> make_env_block(intptr_t r, intptr_t w, const std::string& args_json) {
    std::vector<wchar_t> block;
    LPWCH env = GetEnvironmentStringsW();
    if (env) {
        LPWCH p = env;
        while (*p) {
            size_t n = wcslen(p) + 1;
            block.insert(block.end(), p, p + n);
            p += n;
        }
        FreeEnvironmentStringsW(env);
    }
    wchar_t num[64];
    swprintf(num, 64, L"PYLE_WORKER=1");
    block.insert(block.end(), num, num + wcslen(num) + 1);
    swprintf(num, 64, L"PYLE_WORKER_READ=%lld", static_cast<long long>(r));
    block.insert(block.end(), num, num + wcslen(num) + 1);
    swprintf(num, 64, L"PYLE_WORKER_WRITE=%lld", static_cast<long long>(w));
    block.insert(block.end(), num, num + wcslen(num) + 1);
    std::wstring wargs = widen(args_json);
    std::wstring var = L"PYLE_WORKER_ARGS=" + wargs;
    block.insert(block.end(), var.c_str(), var.c_str() + var.size() + 1);
    block.push_back(L'\0');
    return block;
}
#endif

struct Process {
    std::mutex rmutex;
    std::mutex wmutex;
    RawHandle read_h = kBadHandle;
    RawHandle write_h = kBadHandle;
#ifdef _WIN32
    HANDLE proc = nullptr;
    int64_t pid_value = -1;
#else
    pid_t pid = -1;
    int exit_code = -1;
    bool exited = false;
#endif

    ~Process() {
        handle_close(read_h);
        handle_close(write_h);
#ifdef _WIN32
        if (proc) CloseHandle(proc);
#else
        if (pid > 0 && !exited) {
            int st = 0;
            if (waitpid(pid, &st, WNOHANG) > 0) {
                exited = true;
                exit_code = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
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

    bool send_msg(const std::string& s) {
        std::lock_guard<std::mutex> lock(wmutex);
        return frame_send(write_h, s);
    }

    bool recv_msg(std::string& out, int64_t timeout_ms) {
        std::lock_guard<std::mutex> lock(rmutex);
        return frame_recv(read_h, out, timeout_ms);
    }

    bool is_alive() {
#ifdef _WIN32
        if (!proc) return false;
        DWORD code = 0;
        if (!GetExitCodeProcess(proc, &code)) return false;
        return code == STILL_ACTIVE;
#else
        if (pid <= 0 || exited) return false;
        int st = 0;
        pid_t r = waitpid(pid, &st, WNOHANG);
        if (r == 0) return true;
        if (r < 0) return false;
        exited = true;
        exit_code = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
        return false;
#endif
    }

    int64_t join() {
#ifdef _WIN32
        if (!proc) return -1;
        WaitForSingleObject(proc, INFINITE);
        DWORD code = 0;
        GetExitCodeProcess(proc, &code);
        return static_cast<int64_t>(code);
#else
        if (pid <= 0) return -1;
        if (!exited) {
            int st = 0;
            while (waitpid(pid, &st, 0) < 0 && errno == EINTR) {
            }
            exited = true;
            exit_code = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
        }
        return static_cast<int64_t>(exit_code);
#endif
    }

    bool terminate() {
#ifdef _WIN32
        return proc && TerminateProcess(proc, 1);
#else
        return pid > 0 && kill(pid, SIGTERM) == 0;
#endif
    }
};

bool spawn_process(const std::string& exe, const std::string& script,
                   const std::string& args_json, Process*& out, std::string& err) {
    std::error_code ec;
    std::string abs_exe = std::filesystem::absolute(exe, ec).string();
    if (ec) abs_exe = exe;
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE p2c_r = nullptr, p2c_w = nullptr, c2p_r = nullptr, c2p_w = nullptr;
    if (!CreatePipe(&p2c_r, &p2c_w, &sa, 0) || !CreatePipe(&c2p_r, &c2p_w, &sa, 0)) {
        err = "os.spawn could not create pipes.";
        handle_close(p2c_r);
        handle_close(p2c_w);
        handle_close(c2p_r);
        handle_close(c2p_w);
        return false;
    }
    SetHandleInformation(p2c_w, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(c2p_r, HANDLE_FLAG_INHERIT, 0);
    std::string cmd = quote_arg(abs_exe) + " " + quote_arg(script);
    std::wstring wcmd = widen(cmd);
    std::vector<wchar_t> cmdbuf(wcmd.begin(), wcmd.end());
    cmdbuf.push_back(L'\0');
    std::vector<wchar_t> env = make_env_block(reinterpret_cast<intptr_t>(p2c_r), reinterpret_cast<intptr_t>(c2p_w), args_json);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessW(nullptr, cmdbuf.data(), nullptr, nullptr, TRUE, CREATE_UNICODE_ENVIRONMENT,
                             env.data(), nullptr, &si, &pi);
    handle_close(p2c_r);
    handle_close(c2p_w);
    if (!ok) {
        handle_close(p2c_w);
        handle_close(c2p_r);
        err = last_error_string("os.spawn could not start worker process.");
        return false;
    }
    CloseHandle(pi.hThread);
    Process* p = new Process();
    p->write_h = p2c_w;
    p->read_h = c2p_r;
    p->proc = pi.hProcess;
    p->pid_value = static_cast<int64_t>(pi.dwProcessId);
    out = p;
    return true;
#else
    int p2c[2] = {-1, -1};
    int c2p[2] = {-1, -1};
    if (pipe(p2c) != 0 || pipe(c2p) != 0) {
        err = "os.spawn could not create pipes.";
        handle_close(p2c[0]);
        handle_close(p2c[1]);
        handle_close(c2p[0]);
        handle_close(c2p[1]);
        return false;
    }
    pid_t pid = fork();
    if (pid < 0) {
        err = "os.spawn could not fork worker process.";
        handle_close(p2c[0]);
        handle_close(p2c[1]);
        handle_close(c2p[0]);
        handle_close(c2p[1]);
        return false;
    }
    if (pid == 0) {
        close(p2c[1]);
        close(c2p[0]);
        char rb[32];
        char wb[32];
        snprintf(rb, sizeof(rb), "%d", p2c[0]);
        snprintf(wb, sizeof(wb), "%d", c2p[1]);
        setenv("PYLE_WORKER", "1", 1);
        setenv("PYLE_WORKER_READ", rb, 1);
        setenv("PYLE_WORKER_WRITE", wb, 1);
        setenv("PYLE_WORKER_ARGS", args_json.c_str(), 1);
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(exe.c_str()));
        argv.push_back(const_cast<char*>(script.c_str()));
        argv.push_back(nullptr);
        execvp(exe.c_str(), argv.data());
        _exit(127);
    }
    close(p2c[0]);
    close(c2p[1]);
    Process* p = new Process();
    p->write_h = p2c[1];
    p->read_h = c2p[0];
    p->pid = pid;
    out = p;
    return true;
#endif
}

RawHandle g_worker_in = kBadHandle;
RawHandle g_worker_out = kBadHandle;

int64_t opt_timeout_ms(VM& vm, ArgView args, size_t idx) {
    if (idx >= args.size()) return -1;
    if (args[idx].tag == Value::Tag::Int) return args[idx].as_int;
    if (args[idx].tag == Value::Tag::Float) return static_cast<int64_t>(args[idx].as_float);
    vm.runtime_error(RuntimeError::ArgumentError, "timeout_ms expects an int.");
    return -1;
}

Value recv_result(VM& vm, const std::string& s, bool ok) {
    if (!ok) return Value();
    return to_transient_string(vm, s);
}

struct PendingRecv {
    VM* vm = nullptr;
    std::shared_ptr<Future> fut;
    std::string bytes;
    std::atomic<bool> done{false};
    bool ok = false;
};

std::mutex& pending_mutex() {
    static std::mutex m;
    return m;
}

std::vector<std::shared_ptr<PendingRecv>>& pending_recvs() {
    static std::vector<std::shared_ptr<PendingRecv>> v;
    return v;
}

Value recv_tick(VM& vm, ArgView) {
    std::vector<std::shared_ptr<PendingRecv>> ready;
    {
        std::lock_guard<std::mutex> lock(pending_mutex());
        auto& v = pending_recvs();
        for (auto it = v.begin(); it != v.end();) {
            if ((*it)->vm == &vm && (*it)->done.load()) {
                ready.push_back(std::move(*it));
                it = v.erase(it);
            } else {
                ++it;
            }
        }
    }
    for (auto& rec : ready) {
        if (rec->ok) {
            rec->fut->resolve(to_transient_string(vm, rec->bytes));
        } else {
            rec->fut->failed = true;
            rec->fut->error = "process channel closed.";
            rec->fut->finished.store(true);
        }
    }
    return Value();
}

}  // namespace

int cpu_count() {
    unsigned n = std::thread::hardware_concurrency();
    return n == 0 ? 1 : static_cast<int>(n);
}

bool worker_init_from_env(VM& vm) {
    const char* flag = std::getenv("PYLE_WORKER");
    if (!flag || flag[0] == '\0' || flag[0] == '0') return false;
    const char* r = std::getenv("PYLE_WORKER_READ");
    const char* w = std::getenv("PYLE_WORKER_WRITE");
    if (!r || !w) return false;
#ifdef _WIN32
    g_worker_in = reinterpret_cast<HANDLE>(static_cast<intptr_t>(_strtoi64(r, nullptr, 10)));
    g_worker_out = reinterpret_cast<HANDLE>(static_cast<intptr_t>(_strtoi64(w, nullptr, 10)));
#else
    g_worker_in = atoi(r);
    g_worker_out = atoi(w);
#endif
    if (!handle_valid(g_worker_in) || !handle_valid(g_worker_out)) {
        g_worker_in = kBadHandle;
        g_worker_out = kBadHandle;
        return false;
    }
    vm.is_worker = true;
    return true;
}

Value os_spawn(VM& vm, ArgView args) {
    if (args.size() < 1 || args.size() > 2 || args[0].tag != Value::Tag::StringRef) {
        vm.runtime_error(RuntimeError::ArgumentError, "os.spawn expects (script: string, args = []).");
        return Value();
    }
    std::vector<std::string> spargs;
    if (args.size() == 2) {
        if (args[1].tag != Value::Tag::ArrayRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.spawn args expects an array of strings.");
            return Value();
        }
        const auto& arr = std::get<ArrayType>(vm.get_heap_object(args[1].as_ref).data);
        for (const auto& v : arr) {
            if (v.tag != Value::Tag::StringRef) {
                vm.runtime_error(RuntimeError::ArgumentError, "os.spawn args expects an array of strings.");
                return Value();
            }
            spargs.push_back(std::get<std::string>(vm.get_heap_object(v.as_ref).data));
        }
    }
    if (vm.executable_path.empty()) {
        vm.runtime_error(RuntimeError::Runtime, "os.spawn needs the interpreter path, which the embedder did not set.");
        return Value();
    }
    std::string script = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
    Process* p = nullptr;
    std::string err;
    if (!spawn_process(vm.executable_path, script, encode_string_array(spargs), p, err)) {
        vm.runtime_error(RuntimeError::Runtime, err);
        return Value();
    }
    return to_value_owned(vm, new std::shared_ptr<Process>(p));
}

Value os_is_worker(VM& vm, ArgView args) {
    if (args.size() != 0) {
        vm.runtime_error(RuntimeError::ArgumentError, "os.is_worker takes 0 arguments.");
        return Value();
    }
    return Value(vm.is_worker);
}

Value os_worker_send(VM& vm, ArgView args) {
    if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
        vm.runtime_error(RuntimeError::ArgumentError, "os.worker_send expects (msg: string).");
        return Value();
    }
    if (!vm.is_worker) {
        vm.runtime_error(RuntimeError::Runtime, "os.worker_send called outside a spawned worker.");
        return Value();
    }
    const std::string& s = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
    return Value(frame_send(g_worker_out, s));
}

Value os_worker_recv(VM& vm, ArgView args) {
    if (args.size() > 1) {
        vm.runtime_error(RuntimeError::ArgumentError, "os.worker_recv expects (timeout_ms = -1).");
        return Value();
    }
    if (!vm.is_worker) {
        vm.runtime_error(RuntimeError::Runtime, "os.worker_recv called outside a spawned worker.");
        return Value();
    }
    int64_t t = opt_timeout_ms(vm, args, 0);
    if (vm.is_panicked()) return Value();
    std::string out;
    return recv_result(vm, out, frame_recv(g_worker_in, out, t));
}

Value os_worker_args(VM& vm, ArgView args) {
    if (args.size() != 0) {
        vm.runtime_error(RuntimeError::ArgumentError, "os.worker_args takes 0 arguments.");
        return Value();
    }
    const char* raw = std::getenv("PYLE_WORKER_ARGS");
    std::vector<std::string> items;
    if (raw && !decode_string_array(raw, items)) {
        vm.runtime_error(RuntimeError::Runtime, "os.worker_args could not parse worker arguments.");
        return Value();
    }
    ArrayType arr;
    for (auto& s : items) arr.push_back(Value(Value::Tag::StringRef, vm.intern_string(s)));
    return Value(Value::Tag::ArrayRef, vm.alloc(Object(std::move(arr))));
}

int64_t proc_cpu_count() {
    return static_cast<int64_t>(cpu_count());
}

Process* proc_of(VM& m, HeapIdx self) {
    auto* sp = static_cast<std::shared_ptr<Process>*>(std::get<NativeObject>(m.get_heap_object(self).data).ptr);
    return sp->get();
}

void bind_to_os(VM& vm, NativeModule& mod) {
    static std::atomic<bool> tick_registered{false};
    if (!tick_registered.exchange(true)) background_ticks().push_back(recv_tick);
    mod.function<proc_cpu_count>("cpu_count");
    mod.raw_function("spawn", os_spawn);
    mod.raw_function("is_worker", os_is_worker);
    mod.raw_function("worker_send", os_worker_send);
    mod.raw_function("worker_recv", os_worker_recv);
    mod.raw_function("worker_args", os_worker_args);

    SharedClassBinder<Process> binder(vm, "Process");
    binder.custom_constructor([](VM& m, ArgView) -> Value {
        m.runtime_error(RuntimeError::ArgumentError, "Process cannot be constructed directly, use os.spawn.");
        return Value();
    });
    binder.custom_method("pid", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() != 0) {
            m.runtime_error(RuntimeError::ArgumentError, "pid takes 0 arguments.");
            return Value();
        }
        return Value(proc_of(m, self)->get_pid());
    });
    binder.custom_method("send", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            m.runtime_error(RuntimeError::ArgumentError, "send expects (msg: string).");
            return Value();
        }
        const std::string& s = std::get<std::string>(m.get_heap_object(args[0].as_ref).data);
        return Value(proc_of(m, self)->send_msg(s));
    });
    binder.custom_method("recv", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() > 1) {
            m.runtime_error(RuntimeError::ArgumentError, "recv expects (timeout_ms = -1).");
            return Value();
        }
        int64_t t = opt_timeout_ms(m, args, 0);
        if (m.is_panicked()) return Value();
        std::string out;
        return recv_result(m, out, proc_of(m, self)->recv_msg(out, t));
    });
    binder.custom_method("recv_async", [](VM& m, HeapIdx self, ArgView) -> Value {
        auto* sp = static_cast<std::shared_ptr<Process>*>(std::get<NativeObject>(m.get_heap_object(self).data).ptr);
        std::shared_ptr<Process> held = *sp;
        auto [fval, fut] = Future::create(m);
        auto rec = std::make_shared<PendingRecv>();
        rec->vm = &m;
        rec->fut = fut;
        {
            std::lock_guard<std::mutex> lock(pending_mutex());
            pending_recvs().push_back(rec);
        }
        std::thread([held, rec]() {
            rec->ok = held->recv_msg(rec->bytes, -1);
            rec->done.store(true);
        }).detach();
        return fval;
    });
    binder.custom_method("wait", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() != 0) {
            m.runtime_error(RuntimeError::ArgumentError, "wait takes 0 arguments.");
            return Value();
        }
        return Value(proc_of(m, self)->join());
    });
    binder.custom_method("kill", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() != 0) {
            m.runtime_error(RuntimeError::ArgumentError, "kill takes 0 arguments.");
            return Value();
        }
        return Value(proc_of(m, self)->terminate());
    });
    binder.custom_method("alive", [](VM& m, HeapIdx self, ArgView args) -> Value {
        if (args.size() != 0) {
            m.runtime_error(RuntimeError::ArgumentError, "alive takes 0 arguments.");
            return Value();
        }
        return Value(proc_of(m, self)->is_alive());
    });
    mod.class_binder(binder);
}

}  // namespace proc
}  // namespace pyle
