#include <exception>
#include <iostream>
#include <fmt/printf.h>
#include "pyle/pyle.hpp"
#include "pyle/std/std_core.hpp" 
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <argparse/argparse.hpp>
#include <string>
#include "pyle/binder.hpp"

std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filepath);
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

void print_assertion_status() {
    #ifdef NDEBUG
        puts("Assertions disabled");
    #else
        puts("Assertions enabled");
    #endif
}

class Player {
public:
    std::string name;
    int health;

    Player(const std::string& name, int health) : name(name), health(health) {
        std::cout << "[C++] Player " << name << " allocated.\n";
    }

    ~Player() {
        std::cout << "[C++] Player " << name << " destructed.\n";
    }

    void damageBy(int damage) {
        health -= damage;
        std::cout << "[C++] Player " << name << " damaged by " << damage << ". Health left: " << health << "\n";
    }

    std::string getStatus() {
        return name + " holds " + std::to_string(health) + " hp.";
    }
};



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
        fmt::print("Pyle version 0.1.0\n");
        return 0;
    }

    std::string script_path = program.get<std::string>("script");
    pyle::Pyle pyle;
    pyle::register_core_natives(pyle.vm);

    pyle::ClassBinder<Player>(pyle.vm, "Player")
        .constructor<std::string, int>()                    
        .member<std::string, &Player::name>("name")          
        .member<int, &Player::health>("health")              
        .method<&Player::damageBy>("damageBy")              
        .method<&Player::getStatus>("getStatus");         


    try {
        std::string source = read_file(script_path);
        pyle.execute(source, program.get<bool>("--dissassamble"), script_path);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;

}