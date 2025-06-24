from .pyle_bytecode import PyleFunction

import time

def native_echo(vm, *args):
    print(*(str(arg) for arg in args))
    return None

def native_len(vm, arg):
    return len(arg)

def native_scan(vm, *arg):
    if arg:
        return input(arg[0])
    return input()

def native_perf_counter(vm):
    return time.perf_counter()

def native_import_py(vm, arg):
    return __import__(arg)

def native_get_attr(vm, obj, attr):
    return getattr(obj, attr)


BUILTINS = {
    "echo": PyleFunction(name="echo", arity=-1, start_ip=None, native_fn=native_echo),
    "len":  PyleFunction(name="len", arity=1, start_ip=None, native_fn=native_len),
    "scan": PyleFunction(name="scan", arity=-1, start_ip=None, native_fn=native_scan),
    "perf_counter": PyleFunction(name="perf_counter", arity=0, start_ip=None, native_fn=native_perf_counter),
    "importpy": PyleFunction(name="importpy", arity=1, start_ip=None, native_fn=native_import_py),
    "get_attr": PyleFunction(name="get_attr", arity=2, start_ip=None, native_fn=native_get_attr),
}