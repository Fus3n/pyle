#include <iostream>
#include <string>
#include "pyle/pyle.hpp"
#include "pyle/std/std_core.hpp"
#include "pyle/binder.hpp"

class Player {
public:
    std::string name;
    int health;

    Player(const std::string& name, int health) : name(name), health(health) {
        std::cout << "[C++ Constructor] Player " << name << " allocated.\n";
    }

    ~Player() {
        std::cout << "[C++ Destructor] Player " << name << " destructed.\n";
    }

    void damageBy(int damage) {
        health -= damage;
        std::cout << "[C++ Method] Player " << name << " damaged by " << damage 
                  << ". Health left: " << health << "\n";
    }

    std::string getStatus() const {
        return name + " holds " + std::to_string(health) + " HP.";
    }
};

class Vector2 {
public:
    double x;
    double y;

    Vector2(double x, double y) : x(x), y(y) {}

    void print_coords() const {
        std::cout << "[C++ Method] Vector2(" << x << ", " << y << ")\n";
    }
};

int main() {
    pyle::Pyle interpreter;
    pyle::register_core_natives(interpreter.vm);

    // Strategy A: Bind "Player" class to a custom native module "game" (NOT globally)
    auto player_binder = pyle::ClassBinder<Player>(interpreter.vm, "Player")
        .constructor<std::string, int>()                     
        .member<std::string, &Player::name>("name")          
        .member<int, &Player::health>("health")              
        .method<&Player::damageBy>("damageBy")               
        .method<&Player::getStatus>("getStatus"); // Notice: No .register_globally() called

    // Register native module "game" and export the Player class binder
    pyle::register_module(interpreter.vm, "game", [&player_binder](pyle::VM& vm) -> pyle::Value {
        return pyle::NativeModule(vm, "game")
            .class_binder(player_binder)
            .build();
    });

    // Strategy B: Bind "Vector2" class directly to the global scope
    pyle::ClassBinder<Vector2>(interpreter.vm, "Vector2")
        .constructor<double, double>()                     
        .member<double, &Vector2::x>("x")          
        .member<double, &Vector2::y>("y")              
        .method<&Vector2::print_coords>("print_coords")
        .register_globally(); // Available globally in all scripts without imports

    std::string code = R"(
        let pos = Vector2(10.5, -4.2)
        pos.print_coords()

        let game = import("game")
        
        let hero = game.Player("Arthur", 100)
        hero.damageBy(35)
        print(hero.getStatus())

        print("Finished script execution block.")
    )";

    std::cout << "Running Pyle Class and Module Binding Example\n\n";

    interpreter.execute(code, false, "class_binding.pyl");

    std::cout << "\n--- Triggering Manual Garbage Collection Sweep ---\n";
    interpreter.vm.gc_collect_now();

    std::cout << "\nClass Binding Example Finished\n";
    return 0;
}