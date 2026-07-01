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
    add_cxflags("-fsanitize=address,undefined", "-fno-omit-frame-pointer", {force = true})
    add_ldflags("-fsanitize=address,undefined", {force = true})
end

add_requires("fmt 12.2.0", {configs = {header_only = true}})
add_requires("unordered_dense 4.8.1", "argparse 3.2", "simdjson 4.2.4")
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
    add_files("pyle/src/**.cpp")
    add_packages("argparse", "simdjson")
    add_deps("libpyle")
    set_rundir("$(projectdir)")
    if is_mode("release") then
        set_policy("build.optimization.lto", true)
    end
    after_build(function (target)
        import("core.project.project")
        os.cp("$(projectdir)/std", target:targetdir())
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