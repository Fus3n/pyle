struct Response(status: int, body: string, headers: map, ok: bool) { }

struct Request(method: string, path: string, query: map, headers: map, body: string, captures: array, remote_addr: string) { }

struct Client(base_url: string) {
    fn set_header(name: string, value: string) { }
    fn set_timeout(seconds: float) { }
    fn get(path: string, options: map): Response { }
    fn post(path: string, options: map): Response { }
    fn put(path: string, options: map): Response { }
    fn patch(path: string, options: map): Response { }
    fn delete(path: string, options: map): Response { }
    fn head(path: string, options: map): Response { }
    fn options(path: string, options: map): Response { }
    fn get_async(path: string, options: map): Future { }
    fn post_async(path: string, options: map): Future { }
    fn put_async(path: string, options: map): Future { }
    fn patch_async(path: string, options: map): Future { }
    fn delete_async(path: string, options: map): Future { }
    fn head_async(path: string, options: map): Future { }
    fn options_async(path: string, options: map): Future { }
}

struct Server(host: string, port: int) {
    fn get(pattern: string, handler) { }
    fn post(pattern: string, handler) { }
    fn put(pattern: string, handler) { }
    fn patch(pattern: string, handler) { }
    fn delete(pattern: string, handler) { }
    fn head(pattern: string, handler) { }
    fn options(pattern: string, handler) { }
    fn mount(prefix: string, directory: string) { }
    fn run() { }
    fn serve_async(): Future { }
    fn stop() { }
    fn port(): int { }
}

fn request(method: string, url: string, options: map): Response { }
fn get(url: string, options: map): Response { }
fn post(url: string, options: map): Response { }
fn put(url: string, options: map): Response { }
fn patch(url: string, options: map): Response { }
fn delete(url: string, options: map): Response { }
fn head(url: string, options: map): Response { }
fn get_async(url: string, options: map): Future { }
fn post_async(url: string, options: map): Future { }
fn put_async(url: string, options: map): Future { }
fn patch_async(url: string, options: map): Future { }
fn delete_async(url: string, options: map): Future { }
fn head_async(url: string, options: map): Future { }
fn url_encode(text: string): string { }
fn url_decode(text: string): string { }
fn mime(extension: string, content_type: string) { }
fn html(body: string, status: int): map { }
