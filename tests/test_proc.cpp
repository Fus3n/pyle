#include "test.hpp"
#include "pyle/std/std_core_modules.hpp"
#include <string>

#ifdef _WIN32
static const std::string kRunEcho = "proc.run(\"cmd\", [\"/C\", \"echo hi\"])";
static const std::string kCatProg = "\"findstr\"";
static const std::string kCatArgs = "[\"^\"]";
static const std::string kSleepProg = "\"ping\"";
static const std::string kSleepArgs = "[\"-n\", \"6\", \"127.0.0.1\"]";
#else
static const std::string kRunEcho = "proc.run(\"echo\", [\"hi\"])";
static const std::string kCatProg = "\"cat\"";
static const std::string kCatArgs = "[]";
static const std::string kSleepProg = "\"sleep\"";
static const std::string kSleepArgs = "[\"5\"]";
#endif

TEST(proc_selective_registration) {
    {
        pyle::Pyle p;
        pyle::register_core_natives(p.vm, false);
        p.execute("let proc = import(\"proc\")", false, "test.pyl");
        CHECK(p.vm.is_panicked());
    }
    {
        pyle::Pyle p;
        pyle::register_core_natives(p.vm, false);
        pyle::register_proc_module(p.vm);
        bool ok = p.execute("let proc = import(\"proc\")\nlet has_run = proc.has(\"run\")\nlet has_popen = proc.has(\"popen\")\nlet has_async = proc.has(\"run_async\")\nlet has_child = proc.has(\"Child\")\n",
                            false, "test.pyl");
        CHECK(ok);
        CHECK(!p.vm.is_panicked());
        CHECK(p.vm.value_to_string(p.vm.get_global("has_run")) == "true");
        CHECK(p.vm.value_to_string(p.vm.get_global("has_popen")) == "true");
        CHECK(p.vm.value_to_string(p.vm.get_global("has_async")) == "true");
        CHECK(p.vm.value_to_string(p.vm.get_global("has_child")) == "true");
    }
}

TEST(proc_run_captures_stdout) {
    Ctx t;
    CHECK_RUN(t, "let proc = import(\"proc\")\nlet r = " + kRunEcho +
                 "\nlet code = r.code\nlet out = r.stdout.to_string()\nlet tout = r.timed_out\n");
    CHECK_CALM(t);
    CHECK_STR(t, "code", "0");
    CHECK_STR(t, "tout", "false");
    CHECK(t.get("out").find("hi") != std::string::npos);
}

TEST(proc_run_missing_program_panics) {
    Ctx t;
    CHECK_RUN(t, "let proc = import(\"proc\")\nlet r = pcall(fn() { return proc.run(\"pyle_definitely_missing_xyz\") })\nlet ok = r.ok\n");
    CHECK_CALM(t);
    CHECK_STR(t, "ok", "false");
}

TEST(proc_run_bad_args_panic) {
    Ctx t;
    CHECK_RUN(t, "let proc = import(\"proc\")\nlet a = pcall(fn() { return proc.run(42) })\nlet b = pcall(fn() { return proc.run(\"echo\", \"hi\") })\nlet c = pcall(fn() { return proc.run(\"echo\", [], {nope: 1}) })\nlet ra = a.ok\nlet rb = b.ok\nlet rc = c.ok\n");
    CHECK_CALM(t);
    CHECK_STR(t, "ra", "false");
    CHECK_STR(t, "rb", "false");
    CHECK_STR(t, "rc", "false");
}

TEST(proc_run_with_env) {
#ifdef _WIN32
    Ctx t;
    CHECK_RUN(t, "let proc = import(\"proc\")\nlet r2 = proc.run(\"cmd\", [\"/C\", \"echo %PYLE_PROC_TEST%\"], {env: {PYLE_PROC_TEST: \"abc\"}})\nlet code = r2.code\nlet out = r2.stdout.to_string()\n");
    CHECK_CALM(t);
    CHECK_STR(t, "code", "0");
    CHECK(t.get("out").find("abc") != std::string::npos);
#else
    Ctx t;
    CHECK_RUN(t, "let proc = import(\"proc\")\nlet r2 = proc.run(\"printenv\", [\"PYLE_PROC_TEST\"], {env: {PYLE_PROC_TEST: \"abc\"}})\nlet code = r2.code\nlet out = r2.stdout.to_string()\n");
    CHECK_CALM(t);
    CHECK_STR(t, "code", "0");
    CHECK(t.get("out").find("abc") != std::string::npos);
#endif
}

TEST(proc_popen_interactive) {
    Ctx t;
    CHECK_RUN(t, "let proc = import(\"proc\")\nlet c = proc.popen(" + kCatProg + ", " + kCatArgs +
                 ")\nlet w = c.stdin_write(\"hi\\n\")\nlet cl = c.stdin_close()\nlet chunk = c.stdout_read(65536, 5000)\nlet out = chunk.to_string()\nlet code = c.wait()\nlet p = c.poll()\nlet al = c.alive()\n");
    CHECK_CALM(t);
    CHECK_STR(t, "w", "true");
    CHECK_STR(t, "cl", "true");
    CHECK(t.get("out").find("hi") != std::string::npos);
    CHECK_STR(t, "code", "0");
    CHECK_STR(t, "p", "0");
    CHECK_STR(t, "al", "false");
}

TEST(proc_popen_poll_and_kill) {
    Ctx t;
    CHECK_RUN(t, "let proc = import(\"proc\")\nlet c = proc.popen(" + kSleepProg + ", " + kSleepArgs +
                 ")\nlet al0 = c.alive()\nlet p0 = c.poll()\nlet k = c.kill()\nlet code = c.wait()\nlet al1 = c.alive()\n");
    CHECK_CALM(t);
    CHECK_STR(t, "al0", "true");
    CHECK_STR(t, "p0", "none");
    CHECK_STR(t, "k", "true");
    CHECK(t.get("code") != "0");
    CHECK_STR(t, "al1", "false");
}

TEST(proc_run_timeout_kills) {
    Ctx t;
    CHECK_RUN(t, "let proc = import(\"proc\")\nlet r = proc.run(" + kSleepProg + ", " + kSleepArgs +
                 ", {timeout_ms: 500})\nlet tout = r.timed_out\nlet code = r.code\n");
    CHECK_CALM(t);
    CHECK_STR(t, "tout", "true");
    CHECK_STR(t, "code", "none");
}

TEST(proc_run_with_stdin) {
    Ctx t;
    CHECK_RUN(t, "let proc = import(\"proc\")\nlet r = proc.popen(" + kCatProg + ", " + kCatArgs +
                 ")\nlet c = r.communicate(\"hey\\n\")\nlet code = c.code\nlet out = c.stdout.to_string()\n");
    CHECK_CALM(t);
    CHECK_STR(t, "code", "0");
    CHECK(t.get("out").find("hey") != std::string::npos);
}

TEST(proc_run_async_resolves) {
#ifdef _WIN32
    Ctx t;
    CHECK_RUN(t, "let proc = import(\"proc\")\nfn main() {\nlet f = proc.run_async(\"cmd\", [\"/C\", \"echo hi\"])\nlet r = waitfor(f)\nreturn r.code\n}\nlet code = async.run(main)\n");
    CHECK_CALM(t);
    CHECK_STR(t, "code", "0");
#else
    Ctx t;
    CHECK_RUN(t, "let proc = import(\"proc\")\nfn main() {\nlet f = proc.run_async(\"echo\", [\"hi\"])\nlet r = waitfor(f)\nreturn r.code\n}\nlet code = async.run(main)\n");
    CHECK_CALM(t);
    CHECK_STR(t, "code", "0");
#endif
}
