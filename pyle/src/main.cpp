#include <exception>
#include <iostream>
#include <fmt/printf.h>


#include <argparse/argparse.hpp>
#include <string>
#include <pyle/config.hpp>

#include "pyle/pyle.hpp"
#include "pyle/std/std_core.hpp" 
#include "pyle/std/std_process.hpp"
#include "pyle/binder.hpp"
#include "utils.hpp"


pyle::Value register_json_module(pyle::VM& vm);
pyle::Value register_http_module(pyle::VM& vm);
pyle::Value register_crypto_module(pyle::VM& vm);


int main(int argc, char* argv[]) {
    print_assertion_status();

    argparse::ArgumentParser program("pyle");

    program.add_argument("-v", "--version")
        .help("Prints version information")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("script")
        .help("Path to .pyle script (everything after it goes to os.args verbatim)")
        .required()
        .default_value(std::string(""));
    program.add_argument("-d", "--dissassamble")
        .help("Prints dissassambled bytecode of the given script")
        .default_value(false)
        .implicit_value(true);

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
            if (tok.size() > 1 && tok[0] == '-') {
                pyle_argv.push_back(tok);
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
        fmt::printf("Pyle version %s\n", PYLE_VERSION);
        return 0;
    }

    std::string script_path = program.get<std::string>("script");
    const std::string def_suffix = ".d.pyl";
    if (script_path.size() >= def_suffix.size() &&
        script_path.compare(script_path.size() - def_suffix.size(), def_suffix.size(), def_suffix) == 0) {
        fmt::print(stderr, "Error: '{}' is a type definition file, not a script.\n", script_path);
        return 1;
    }
    pyle::Pyle pyle;
    pyle.vm.executable_path = argv[0];
    pyle.vm.argv = script_argv;
    pyle::proc::worker_init_from_env(pyle.vm);
    pyle::register_core_natives(pyle.vm); 
    pyle::register_module(pyle.vm, "json", register_json_module);
    pyle::register_module(pyle.vm, "http", register_http_module);
    pyle::register_module(pyle.vm, "crypto", register_crypto_module);

    fs::path exe_dir = get_executable_directory();
    pyle.vm.add_import_path((exe_dir / "std").string());
    pyle.vm.add_import_path("./std/");

    try {
        std::string source = read_file(script_path);
        bool ok = pyle.execute(source, program.get<bool>("--dissassamble"), script_path);
        if (!ok || pyle.vm.is_panicked()) return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;

}