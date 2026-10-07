#pragma once
#include <string>

namespace pyle {
class VM;
class NativeModule;
namespace proc {
int cpu_count();
bool worker_init_from_env(VM& vm);
void bind_to_os(VM& vm, NativeModule& mod);
}
}
