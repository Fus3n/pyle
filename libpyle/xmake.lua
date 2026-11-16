option("profile_ops")
    set_default(false)
    set_showmenu(true)
    set_description("Enable per-opcode profiling counters (PYLE_PROFILE=1 at runtime)")
option_end()

package("unordered_dense")
    set_homepage("https://github.com/martinus/unordered_dense")
    add_urls("https://github.com/martinus/unordered_dense/archive/refs/tags/v$(version).tar.gz")
    add_versions("4.11.0", "a232f7433b45872d43e4dc74a25cbd58effc0be76e3d704b34e5de3c637eed77")
    on_install(function (package)
        os.cp("include/*", package:installdir("include"))
    end)
package_end()

add_requires("fmt 12.2.0", {configs = {header_only = true}})
add_requires("unordered_dense 4.11.0")

target("libpyle")
    set_kind("$(kind)")
    set_languages("c++17")
    add_files("src/**.cpp")
    if is_plat("wasm") then
        remove_files("src/std/std_proc.cpp", "src/std/std_process.cpp")
        add_cxxflags("-fwasm-exceptions")
    end
    add_includedirs("include", {public = true})
    add_packages("fmt", "unordered_dense", {public = true})
    if has_config("profile_ops") then
        add_defines("PYLE_PROFILE_OPS", {public = true})
    end
    if is_mode("release") then
        set_policy("build.optimization.lto", true)
    end
