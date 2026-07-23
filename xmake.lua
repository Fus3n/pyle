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

package("lspcpp")
    set_homepage("https://github.com/kuafuwang/LspCpp")
    set_description("A Language Server Protocol implementation in C++")
    
    add_urls("https://github.com/kuafuwang/LspCpp.git")
    add_versions("master", "master")
    
    add_deps("cmake")

    if is_plat("windows") then
        add_syslinks("ws2_32", "userenv", "bcrypt")
    elseif is_plat("linux") then
        add_syslinks("pthread")
    end

    on_install(function (package)
        -- PATCH: Fix MinGW build failing on MSVC-only ppltasks.h
        if package:is_plat("mingw", "msys") then
            for _, file in ipairs(os.files("include/**/*.h")) do
                local content = io.readfile(file)
                if content and content:find("ppltasks.h", 1, true) then
                    content = content:gsub("_WIN32", "_MSC_VER")
                    io.writefile(file, content)
                end
            end
            for _, file in ipairs(os.files("src/**/*.cpp")) do
                local content = io.readfile(file)
                if content and content:find("ppltasks.h", 1, true) then
                    content = content:gsub("_WIN32", "_MSC_VER")
                    io.writefile(file, content)
                end
            end
        end

        local configs = {
            "-DLSPCPP_BUILD_TESTS=OFF",
            "-DLSPCPP_BUILD_EXAMPLES=OFF",
            "-DLSPCPP_USE_CPP17=ON",
            "-DUSE_TLS=OFF",
            "-DUSE_ZLIB=OFF",
            "-DLSPCPP_INSTALL=ON" -- CRITICAL: Tells CMake to actually output the headers/lib
        }
        
        if package:is_plat("mingw", "msys", "linux", "macosx") then
            table.insert(configs, "-DCMAKE_CXX_FLAGS=-Wno-error -Wno-shorten-64-to-32")
        end
        
        if package:is_plat("windows") and package:config("vs_runtime") == "MT" then
            table.insert(configs, "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded")
        end
        
        import("package.tools.cmake").install(package, configs)
        
        -- LspCpp's CMake install step forgets to install the 3rd party headers it relies on.
        -- We manually merge them into the package's include directory so your app can find them.
        local incdir = package:installdir("include")
        os.cp("third_party/rapidjson/include/*", incdir)
        os.cp("third_party/utfcpp/source/*", incdir)
        os.cp("third_party/asio/asio/include/*", incdir)
    end)
package_end()

add_requires("fmt 12.2.0", {configs = {header_only = true}})
add_requires("unordered_dense 4.8.1", "argparse 3.2", "simdjson 4.2.4")
add_requires("raylib 5.5")
add_requires("nlohmann_json 3.11.3")
add_requires("lspcpp") 
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
    
target("pyle-lsp")
    set_kind("binary")
    add_files("pyle-lsp/src/main.cpp")
    add_packages("nlohmann_json", "lspcpp")
    add_deps("libpyle")
    set_rundir("$(projectdir)")
    set_runtimes("MT")
    
    if is_plat("mingw", "msys") then
        add_ldflags("-static", {force = true})
    end

target("example_async_binding")
    set_kind("binary")
    set_languages("c++17")
    add_files("examples_cpp/04_async_binding.cpp")
    add_deps("libpyle")
