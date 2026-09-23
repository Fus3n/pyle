#include "http_binding.hpp"

pyle::Value register_http_module(pyle::VM& vm) {
    pyle::NativeModule mod(vm, "http");
    pyle::http_binding::register_client(mod);
    pyle::http_binding::register_server(mod);
    return mod.build();
}
