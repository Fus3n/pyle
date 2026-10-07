fn time(): float { }
fn date(): string { }
fn strftime(format: string, timestamp) { }
fn system(cmd: string) { }
fn file_exists(path: string) { }
fn remove(path: string) { }
fn mkdir(path: string) { }
fn listdir(path: string) { }
fn rename(src: string, dst: string) { }
fn getenv(name: string) { }
fn sleep(ms: int) { }
fn sleep_async(ms: int) { }
fn script_path() { }
fn script_dir() { }
fn cpu_count(): int { }
fn spawn(script: string, args: array[string]): Process { }
fn is_worker(): bool { }
fn worker_send(msg: string): bool { }
fn worker_recv(timeout_ms: int) { }
fn worker_args(): array[string] { }

struct Process {
    fn pid(): int { }
    fn send(msg: string): bool { }
    fn recv(timeout_ms: int) { }
    fn recv_async(): Future { }
    fn wait(): int { }
    fn kill(): bool { }
    fn alive(): bool { }
}
