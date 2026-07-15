#include <exception>
#include <iostream>
#include <fmt/printf.h>


#include <argparse/argparse.hpp>
#include <string>
#include <pyle/config.hpp>

#include "pyle/pyle.hpp"
#include "pyle/std/std_core.hpp" 
#include "pyle/binder.hpp"
#include "utils.hpp"


pyle::Value register_json_module(pyle::VM& vm);


int main(int argc, char* argv[]) {
    print_assertion_status();

    argparse::ArgumentParser program("pyle");

    program.add_argument("-v", "--version")
        .help("Prints version information")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("script")
        .help("Path to .pyle script")
        .required()
        .default_value(std::string(""));
    program.add_argument("-d", "--dissassamble")
        .help("Prints dissassambled bytecode of the given script")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
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
    pyle::Pyle pyle;
    pyle::register_core_natives(pyle.vm); 
    pyle::register_module(pyle.vm, "json", register_json_module);

    fs::path exe_dir = get_executable_directory();
    pyle.vm.add_import_path((exe_dir / "std").string());
    pyle.vm.add_import_path("./std/");

    try {
        std::string source = read_file(script_path);
        pyle.execute(source, program.get<bool>("--dissassamble"), script_path);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;

}