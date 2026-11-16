set_project("pyle")
set_version("0.2.0")
set_languages("c++17")

add_rules("mode.debug", "mode.release")

option("with_cli")
    set_default(true)
    set_showmenu(true)
    set_description("Build the pyle command-line interpreter and its bundled modules")
option_end()

option("with_lsp")
    set_default(true)
    set_showmenu(true)
    set_description("Build the pyle language server")
option_end()

option("with_raylib")
    set_default(true)
    set_showmenu(true)
    set_description("Build the raylib native module")
option_end()

option("with_tests")
    set_default(true)
    set_showmenu(true)
    set_description("Build the test suite")
option_end()

option("with_examples")
    set_default(true)
    set_showmenu(true)
    set_description("Build the C++ embedding examples")
option_end()

if is_mode("release") then
    set_optimize("fastest")
    set_symbols("hidden")
    add_ldflags("-flto=thin", {force = true})
end

if is_plat("mingw", "msys") then
    add_ldflags("-Wl,-Bstatic", "-lc++", "-Wl,-Bdynamic", {force = true})
end

if is_mode("debug") then
    set_policy("build.sanitizer.address", true)      
    set_policy("build.sanitizer.undefined", true)    
end

if has_config("with_cli") and not is_plat("wasm") then
    add_requires("argparse 3.2", "simdjson 4.2.4")
    add_requires("cpp-httplib 0.50.1")
    add_requires("mbedtls 3.6.1")
end

if has_config("with_lsp") and not is_plat("wasm") then
    add_requires("nlohmann_json 3.11.3")
end

if has_config("with_raylib") and not is_plat("wasm") then
    add_requires("raylib 5.5")
end

add_cxxflags("/utf-8", {tools = "cl"})
add_rules("plugin.compile_commands.autoupdate")

includes("libpyle")

if has_config("with_cli") and not is_plat("wasm") then
    target("pyle")
        set_kind("binary")
        add_files("pyle/src/main.cpp")
        add_files("pyle/src/std/std_json.cpp")
        add_files("pyle/src/std/http/**.cpp")
        add_files("pyle/src/std/crypto/module.cpp")
        add_packages("argparse", "simdjson", "cpp-httplib", "mbedtls")
        add_defines("CPPHTTPLIB_MBEDTLS_SUPPORT")
        if is_plat("mingw", "msys", "windows") then
            add_syslinks("ws2_32", "crypt32", "bcrypt")
        end
        add_deps("libpyle")
        set_rundir("$(projectdir)")
        if is_mode("release") then
            set_policy("build.optimization.lto", true)
        end
        on_load(function (target)
            local hash = "unknown"
            local dirty = 0
            local suffix = ""
            local out = try {function () return os.iorunv("git", {"rev-parse", "--short", "HEAD"}) end}
            if out then
                hash = out:trim()
                local status = try {function () return os.iorunv("git", {"status", "--porcelain"}) end}
                if status and #status:trim() > 0 then
                    dirty = 1
                    suffix = "-dirty"
                end
            end
            local content = string.format(
                "#pragma once\n\n#define PYLE_GIT_HASH \"%s\"\n#define PYLE_GIT_DIRTY %d\n#define PYLE_GIT_SUFFIX \"%s\"\n",
                hash, dirty, suffix)
            local header = path.join(os.projectdir(), "pyle", "src", "build_info.hpp")
            local existing = os.isfile(header) and io.readfile(header) or nil
            if existing ~= content then
                io.writefile(header, content)
            end
        end)
        after_build(function (target)
            import("core.project.project")
            os.cp("$(projectdir)/std", target:targetdir())
        end)
end

if has_config("with_raylib") and not is_plat("wasm") then
    target("raylib")
        set_kind("shared")
        set_filename("raylib.pyled")
        add_files("pyle/src/std/vendor/raylib/**.cpp")
        add_packages("raylib")
        add_deps("libpyle")
        after_build(function (target)
            local outdir = target:targetdir()
            local stddir = path.join(outdir, "std")
            os.mkdir(stddir)
            os.cp(target:targetfile(), path.join(stddir, "raylib.pyled"))
            os.cp(target:targetfile(), path.join("$(projectdir)/std", "raylib.pyled"))
        end)
end

if has_config("with_examples") and not is_plat("wasm") then
    target("example_basic_embedding")
        set_kind("binary")
        set_languages("c++17")
        add_files("examples_cpp/01_basic_embedding.cpp")
        add_deps("libpyle")

    target("example_function_binding")
        set_kind("binary")
        set_languages("c++17")
        add_files("examples_cpp/02_function_binding.cpp")
        add_deps("libpyle")

    target("example_class_binding")
        set_kind("binary")
        set_languages("c++17")
        add_files("examples_cpp/03_class_binding.cpp")
        add_deps("libpyle")

    target("example_async_binding")
        set_kind("binary")
        set_languages("c++17")
        add_files("examples_cpp/04_async_binding.cpp")
        add_deps("libpyle")
end

if has_config("with_tests") and not is_plat("wasm") then
    target("pyle_tests")
        set_kind("binary")
        set_group("tests")
        add_files("tests/*.cpp")
        add_deps("libpyle")
        set_rundir("$(projectdir)")
        add_tests("suite")
end

if has_config("with_lsp") and not is_plat("wasm") then
    target("pyle-lsp")
        set_kind("binary")
        add_files("pyle-lsp/src/main.cpp")
        add_packages("nlohmann_json")
        add_deps("libpyle")
        set_rundir("$(projectdir)")
        set_runtimes("MT")
        
        if is_plat("mingw", "msys") then
            add_ldflags("-static", {force = true})
        end

        -- Ship layout: <build>/pyle-lsp/pyle-lsp.exe so the extension and manual
        -- installs can pick it up next to the interpreter.
        after_build(function (target)
            import("core.project.project")
            local outdir = path.join(target:targetdir(), "pyle-lsp")
            os.mkdir(outdir)
            os.cp(target:targetfile(), path.join(outdir, "pyle-lsp.exe"))
        end)
end

if is_plat("wasm") then
    if has_config("with_tests") then
        target("pyle_tests")
            set_kind("binary")
            set_group("tests")
            set_filename("pyle_tests.js")
            add_files("tests/*.cpp")
            remove_files("tests/test_proc.cpp")
            add_deps("libpyle")
            add_cxxflags("-fwasm-exceptions")
            add_ldflags(
                "-fwasm-exceptions",
                "-sEXIT_RUNTIME=1",
                "-sENVIRONMENT=node",
                {force = true}
            )
        target_end()
    end
end
