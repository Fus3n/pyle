add_rules("mode.debug", "mode.release")

set_toolchains("mingw")

if is_mode("release") then
    set_optimize("fastest")
    set_symbols("hidden")
    set_strip("all")
end

if is_mode("debug") then
    if has_config("toolchain") and get_config("toolchain") == "msvc" then
        add_cxflags("/fsanitize=address", {force = true})
    else
        add_cxflags("-fsanitize=address,undefined", "-fno-omit-frame-pointer", {force = true})
        add_ldflags("-fsanitize=address,undefined", {force = true})
    end
end

add_requires("fmt 12.2.0", {configs = {header_only = true}})
add_requires("unordered_dense 4.8.1", "argparse 3.2")
add_cxxflags("/utf-8", {tools = "cl"})
add_rules("plugin.compile_commands.autoupdate")


target("pyle")
    set_kind("static")
    set_languages("c++17")
    add_files("pyle/src/**.cpp")
    add_includedirs("pyle/include", {public = true})
    add_packages("fmt", "unordered_dense", {public = true})
    if is_mode("release") then
        set_policy("build.optimization.lto", true)
        set_toolset("ar", "gcc-ar")
        set_toolset("ranlib", "gcc-ranlib")
    end

target("pyle_cli")
    set_kind("binary")
    set_languages("c++17")
    add_files("pyle_cli/src/**.cpp")
    add_packages("argparse")
    add_deps("pyle")
    set_rundir("$(projectdir)")
    if is_mode("release") then
        set_policy("build.optimization.lto", true)
    end