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

BUILTINS = {
    "echo": PyleFunction(name="echo", arity=-1, start_ip=None, native_fn=native_echo),
    "len":  PyleFunction(name="len", arity=1, start_ip=None, native_fn=native_len),
    "scan": PyleFunction(name="scan", arity=-1, start_ip=None, native_fn=native_scan),
    "perf_counter": PyleFunction(name="perf_counter", arity=0, start_ip=None, native_fn=native_perf_counter),
   
    # Add more as needed
}