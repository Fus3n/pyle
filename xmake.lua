set_project("pyle")
set_version("0.1.0")
set_languages("c++17")

add_rules("mode.debug", "mode.release")

if is_mode("release") then
    set_optimize("fastest")
    set_symbols("hidden")
    add_ldflags("-flto=thin", {force = true})
end

if is_mode("debug") then
    set_policy("build.sanitizer.address", true)      
    set_policy("build.sanitizer.undefined", true)    
end

package("unordered_dense")
    set_homepage("https://github.com/martinus/unordered_dense")
    add_urls("https://github.com/martinus/unordered_dense/archive/refs/tags/v$(version).tar.gz")
    add_versions("4.11.0", "a232f7433b45872d43e4dc74a25cbd58effc0be76e3d704b34e5de3c637eed77")
    on_install(function (package)
        os.cp("include/*", package:installdir("include"))
    end)
package_end()

add_requires("fmt 12.2.0", {configs = {header_only = true}})
add_requires("unordered_dense 4.11.0", "argparse 3.2", "simdjson 4.2.4")
add_requires("raylib 5.5")
add_requires("nlohmann_json 3.11.3")
add_requires("cpp-httplib 0.50.1")
add_requires("mbedtls 3.6.1")
add_cxxflags("/utf-8", {tools = "cl"})
add_rules("plugin.compile_commands.autoupdate")

target("libpyle")
    set_kind("$(kind)")
    add_files("libpyle/src/**.cpp")
    add_includedirs("libpyle/include", {public = true})
    add_packages("fmt", "unordered_dense", {public = true})
    if is_mode("release") then
        set_policy("build.optimization.lto", true)
    end

target("pyle")
    set_kind("binary")
    add_files("pyle/src/main.cpp")
    add_files("pyle/src/std/std_json.cpp")
    add_files("pyle/src/std/http/**.cpp")
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
    after_build(function (target)
        import("core.project.project")
        os.cp("$(projectdir)/std", target:targetdir())
    end)

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



