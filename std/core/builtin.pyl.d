struct TraceFrame(function: string, file: string, line: int) { }

struct PCallError(type: string, message: string, trace: array[TraceFrame]) { }

struct PCallResult(ok: bool, value: any, error: PCallError) { }

fn pcall(f: function): PCallResult { }
fn assert(cond: any, msg: any): any { }
fn print_trace(err: any): none { }
fn raise(err: any): none { }

fn print() { }
fn printf(fmt: string) { }
fn format(fmt: string): string { }
fn input(prompt: string): string { }
fn import(name: string): map { }
fn add_import_path(path: string) { }
fn typeof(v): string { }
fn Coro(f: function): coro { }
fn Bytes(arr: array[int]): bytes { }
fn waitfor(task): any { }
