#include <exception>
#include <iostream>
#include <sstream>
#include <vector>
#include <fmt/printf.h>


#include <argparse/argparse.hpp>
#include <string>
#include <pyle/config.hpp>

#include "pyle/pyle.hpp"
#include "pyle/std/std_core.hpp" 
#include "pyle/std/std_process.hpp"
#include "pyle/binder.hpp"
#include "build_info.hpp"
#include "utils.hpp"


pyle::Value register_json_module(pyle::VM& vm);
pyle::Value register_http_module(pyle::VM& vm);
pyle::Value register_crypto_module(pyle::VM& vm);


static bool looks_like_flag(const std::string& tok) {
    return tok.size() > 1 && tok[0] == '-';
}

int main(int argc, char* argv[]) {
    print_assertion_status();

    argparse::ArgumentParser program("pyle", "", argparse::default_arguments::help);

    program.add_argument("-v", "--version")
        .help("Prints version information")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("script")
        .help("Path to a .pyl script, or - to read the script from stdin (everything after it goes to os.args verbatim)")
        .default_value(std::string(""));
    program.add_argument("-d", "--dissassamble")
        .help("Prints disassembled bytecode of the given script")
        .default_value(false)
        .implicit_value(true);

    const std::vector<std::string> value_flags = {};

    std::vector<std::string> pyle_argv = {argv[0]};
    std::vector<std::string> script_argv;
    bool found_script = false;
    bool dashdash = false;
    for (int i = 1; i < argc; ++i) {
        std::string tok = argv[i];
        if (!found_script && !dashdash) {
            if (tok == "--") {
                dashdash = true;
                continue;
            }
            if (looks_like_flag(tok)) {
                pyle_argv.push_back(tok);
                if (tok.find('=') == std::string::npos) {
                    for (const auto& vf : value_flags) {
                        if (tok == vf && i + 1 < argc) {
                            pyle_argv.push_back(argv[++i]);
                            break;
                        }
                    }
                }
                continue;
            }
        }
        if (!found_script) {
            found_script = true;
            pyle_argv.push_back(tok);
        } else {
            script_argv.push_back(tok);
        }
    }

    std::vector<char*> pargv;
    for (auto& s : pyle_argv) pargv.push_back(const_cast<char*>(s.c_str()));
    try {
        program.parse_args(static_cast<int>(pargv.size()), pargv.data());
    } catch (const std::exception& err) {
        std::cerr << err.what() << "\n";
        std::cerr << program;
        return 1;
    }
    if (program.get<bool>("--version")) {
        fmt::printf("Pyle version %s (%s%s)\n", PYLE_VERSION, PYLE_GIT_HASH, PYLE_GIT_SUFFIX);
        return 0;
    }

    std::string script_path = program.get<std::string>("script");
    if (script_path.empty()) {
        std::cout << program;
        return 1;
    }

    const std::string def_suffix = ".d.pyl";
    if (script_path != "-" && script_path.size() >= def_suffix.size() &&
        script_path.compare(script_path.size() - def_suffix.size(), def_suffix.size(), def_suffix) == 0) {
        fmt::print(stderr, "Error: '{}' is a type definition file, not a script.\n", script_path);
        return 1;
    }

    std::string source;
    std::string display_name = script_path;
    try {
        if (script_path == "-") {
            std::ostringstream buffer;
            buffer << std::cin.rdbuf();
            source = buffer.str();
            display_name = "<stdin>";
        } else {
            source = read_file(script_path);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    pyle::Pyle pyle;
    pyle.vm.executable_path = argv[0];
    pyle.vm.argv = script_argv;
    pyle::proc::worker_init_from_env(pyle.vm);
    pyle::register_core_natives(pyle.vm); 
    {
        pyle::HeapIdx name_idx = pyle.vm.intern_string("PYLE_GIT_HASH");
        pyle::HeapIdx val_idx = pyle.vm.intern_string(PYLE_GIT_HASH);
        int slot = pyle.vm.declare_global(name_idx);
        (*pyle.vm.global_slots)[slot] = pyle::Value(pyle::Value::Tag::StringRef, val_idx);
    }
    pyle::register_module(pyle.vm, "json", register_json_module);
    pyle::register_module(pyle.vm, "http", register_http_module);
    pyle::register_module(pyle.vm, "crypto", register_crypto_module);

    fs::path exe_dir = get_executable_directory();
    pyle.vm.add_import_path((exe_dir / "std").string());
    pyle.vm.add_import_path("./std/");

    try {
        bool ok = pyle.execute(source, program.get<bool>("--dissassamble"), display_name);
        if (!ok || pyle.vm.is_panicked()) return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;

}
