struct Vector2(x: float, y: float) { }
struct Rectangle(x: float, y: float, width: float, height: float) { }
struct Color(r: int, g: int, b: int, a: int) { }
struct Camera2D(offset: Vector2, target: Vector2, rotation: float, zoom: float) { }
struct Vector3(x: float, y: float, z: float) { }
struct Matrix(m0: float, m1: float, m2: float, m3: float, m4: float, m5: float, m6: float, m7: float, m8: float, m9: float, m10: float, m11: float, m12: float, m13: float, m14: float, m15: float) { }
struct Image(width: int, height: int, mipmaps: int, format: int) { }
struct Texture(id: int, width: int, height: int, mipmaps: int, format: int) { }
struct Font(texture: Texture) { }
struct Sound { }
struct Music(looping: bool) { }
struct Camera3D(position: Vector3, target: Vector3, up: Vector3, fovy: float, projection: int) { }
struct BoundingBox(min: Vector3, max: Vector3) { }
struct Ray(position: Vector3, direction: Vector3) { }
struct RayCollision(hit: bool, distance: float, point: Vector3, normal: Vector3) { }
struct Model { }
struct ModelAnimation(boneCount: int, frameCount: int, name: string) { }
struct Shader { }
struct RenderTexture(id: int) { }
struct Mesh(vertexCount: int, triangleCount: int) { }
struct Material { }

fn Vector2Add(v1: Vector2, v2: Vector2): Vector2 { }
fn Vector2Subtract(v1: Vector2, v2: Vector2): Vector2 { }
fn Vector2Scale(v: Vector2, scale: float): Vector2 { }
fn Vector2Length(v: Vector2): float { }
fn Vector2Distance(v1: Vector2, v2: Vector2): float { }
fn Vector2Normalize(v: Vector2): Vector2 { }
fn Vector2Zero(): Vector2 { }
fn Vector2Angle(v1: Vector2, v2: Vector2): float { }
fn Vector3Add(v1: Vector3, v2: Vector3): Vector3 { }
fn Vector3Subtract(v1: Vector3, v2: Vector3): Vector3 { }
fn Vector3Scale(v: Vector3, scale: float): Vector3 { }
fn Vector3Length(v: Vector3): float { }
fn Vector3Distance(v1: Vector3, v2: Vector3): float { }
fn Vector3Normalize(v: Vector3): Vector3 { }
fn Vector3Zero(): Vector3 { }
fn Vector3DotProduct(v1: Vector3, v2: Vector3): float { }
fn Vector3CrossProduct(v1: Vector3, v2: Vector3): Vector3 { }
fn Vector3Lerp(v1: Vector3, v2: Vector3, t: float): Vector3 { }
fn Vector3Angle(v1: Vector3, v2: Vector3): float { }
fn Vector3Negate(v: Vector3): Vector3 { }
fn MatrixIdentity(): Matrix { }
fn MatrixMultiply(a: Matrix, b: Matrix): Matrix { }
fn MatrixTranslate(x: float, y: float, z: float): Matrix { }
fn MatrixRotateX(angle: float): Matrix { }
fn MatrixRotateY(angle: float): Matrix { }
fn MatrixRotateZ(angle: float): Matrix { }
fn MatrixScale(x: float, y: float, z: float): Matrix { }
fn MatrixOrtho(l: float, r: float, b: float, t: float, n: float, f: float): Matrix { }
fn MatrixPerspective(fovY: float, aspect: float, near: float, far: float): Matrix { }
fn MatrixLookAt(eye: Vector3, target: Vector3, up: Vector3): Matrix { }
fn ColorAlpha(color: Color, alpha: float): Color { }

fn InitWindow(w: int, h: int, t: string) { }
fn WindowShouldClose(): bool { }
fn CloseWindow() { }
fn SetTargetFPS(fps: int) { }
fn GetFrameTime(): float { }
fn GetTime(): float { }
fn GetRandomValue(min: int, max: int): int { }
fn SetConfigFlags(flags: int) { }
fn SetExitKey(key: int) { }
fn GetScreenWidth(): int { }
fn GetScreenHeight(): int { }
fn GetFPS(): int { }
fn IsWindowReady(): bool { }
fn IsWindowFullscreen(): bool { }
fn IsWindowMinimized(): bool { }
fn IsWindowMaximized(): bool { }
fn IsWindowFocused(): bool { }
fn IsWindowResized(): bool { }
fn ToggleFullscreen() { }
fn MaximizeWindow() { }
fn MinimizeWindow() { }
fn RestoreWindow() { }
fn GetRenderWidth(): int { }
fn GetRenderHeight(): int { }

fn ClearBackground(color: Color) { }
fn BeginDrawing() { }
fn EndDrawing() { }
fn BeginBlendMode(mode: int) { }
fn EndBlendMode() { }
fn BeginScissorMode(x: int, y: int, width: int, height: int) { }
fn EndScissorMode() { }
fn BeginMode2D(camera: Camera2D) { }
fn EndMode2D() { }
fn DrawPixel(posX: int, posY: int, color: Color) { }
fn DrawPixelV(position: Vector2, color: Color) { }
fn DrawLine(startPosX: int, startPosY: int, endPosX: int, endPosY: int, color: Color) { }
fn DrawLineV(startPos: Vector2, endPos: Vector2, color: Color) { }
fn DrawLineEx(startPos: Vector2, endPos: Vector2, thick: float, color: Color) { }
fn DrawLineBezier(startPos: Vector2, endPos: Vector2, thick: float, color: Color) { }
fn DrawCircle(centerX: int, centerY: int, radius: float, color: Color) { }
fn DrawCircleV(center: Vector2, radius: float, color: Color) { }
fn DrawCircleLines(centerX: int, centerY: int, radius: float, color: Color) { }
fn DrawCircleLinesV(center: Vector2, radius: float, color: Color) { }
fn DrawEllipse(centerX: int, centerY: int, radiusH: float, radiusV: float, color: Color) { }
fn DrawEllipseLines(centerX: int, centerY: int, radiusH: float, radiusV: float, color: Color) { }
fn DrawRing(center: Vector2, innerRadius: float, outerRadius: float, startAngle: float, endAngle: float, segments: int, color: Color) { }
fn DrawRingLines(center: Vector2, innerRadius: float, outerRadius: float, startAngle: float, endAngle: float, segments: int, color: Color) { }
fn DrawRectangle(posX: int, posY: int, width: int, height: int, color: Color) { }
fn DrawRectangleV(position: Vector2, size: Vector2, color: Color) { }
fn DrawRectangleRec(rec: Rectangle, color: Color) { }
fn DrawRectanglePro(rec: Rectangle, origin: Vector2, rotation: float, color: Color) { }
fn DrawRectangleGradientV(posX: int, posY: int, width: int, height: int, color1: Color, color2: Color) { }
fn DrawRectangleGradientH(posX: int, posY: int, width: int, height: int, color1: Color, color2: Color) { }
fn DrawRectangleGradientEx(rec: Rectangle, col1: Color, col2: Color, col3: Color, col4: Color) { }
fn DrawRectangleLines(posX: int, posY: int, width: int, height: int, color: Color) { }
fn DrawRectangleLinesEx(rec: Rectangle, lineThick: float, color: Color) { }
fn DrawRectangleRounded(rec: Rectangle, roundness: float, segments: int, color: Color) { }
fn DrawRectangleRoundedLines(rec: Rectangle, roundness: float, segments: int, lineThick: float, color: Color) { }
fn DrawRectangleRoundedLinesEx(rec: Rectangle, roundness: float, segments: int, lineThick: float, color: Color) { }
fn DrawTriangle(v1: Vector2, v2: Vector2, v3: Vector2, color: Color) { }
fn DrawTriangleLines(v1: Vector2, v2: Vector2, v3: Vector2, color: Color) { }
fn DrawPoly(center: Vector2, sides: int, radius: float, rotation: float, color: Color) { }
fn DrawPolyLines(center: Vector2, sides: int, radius: float, rotation: float, color: Color) { }
fn DrawPolyLinesEx(center: Vector2, sides: int, radius: float, rotation: float, lineThick: float, color: Color) { }
fn DrawFPS(posX: int, posY: int) { }
fn CheckCollisionRecs(rec1: Rectangle, rec2: Rectangle): bool { }
fn CheckCollisionCircles(center1: Vector2, radius1: float, center2: Vector2, radius2: float): bool { }
fn CheckCollisionCircleRec(center: Vector2, radius: float, rec: Rectangle): bool { }
fn CheckCollisionPointRec(point: Vector2, rec: Rectangle): bool { }
fn CheckCollisionPointCircle(point: Vector2, center: Vector2, radius: float): bool { }
fn CheckCollisionPointTriangle(point: Vector2, p1: Vector2, p2: Vector2, p3: Vector2): bool { }
fn CheckCollisionPointPoly(point: Vector2, points: array, pointCount: int): bool { }
fn CheckCollisionLines(startPos1: Vector2, endPos1: Vector2, startPos2: Vector2, endPos2: Vector2): map { }
fn CheckCollisionPointLine(point: Vector2, p1: Vector2, p2: Vector2, threshold: int): bool { }
fn GetCollisionRec(rec1: Rectangle, rec2: Rectangle): Rectangle { }

fn IsKeyDown(key: int): bool { }
fn IsKeyPressed(key: int): bool { }
fn IsKeyReleased(key: int): bool { }
fn IsKeyUp(key: int): bool { }
fn IsKeyPressedRepeat(key: int): bool { }
fn GetKeyPressed(): int { }
fn GetCharPressed(): int { }
fn GetMouseX(): int { }
fn GetMouseY(): int { }
fn IsMouseButtonPressed(button: int): bool { }
fn IsMouseButtonDown(button: int): bool { }
fn IsMouseButtonReleased(button: int): bool { }
fn IsMouseButtonUp(button: int): bool { }
fn GetMouseWheelMove(): float { }
fn SetMouseCursor(cursor: int) { }
fn ShowCursor() { }
fn HideCursor() { }
fn EnableCursor() { }
fn DisableCursor() { }
fn IsCursorHidden(): bool { }
fn GetMousePosition(): Vector2 { }
fn GetMouseDelta(): Vector2 { }
fn GetMouseWheelMoveV(): Vector2 { }
fn IsGamepadAvailable(gamepad: int): bool { }
fn IsGamepadButtonPressed(gamepad: int, button: int): bool { }
fn IsGamepadButtonDown(gamepad: int, button: int): bool { }
fn IsGamepadButtonReleased(gamepad: int, button: int): bool { }
fn IsGamepadButtonUp(gamepad: int, button: int): bool { }
fn GetGamepadAxisMovement(gamepad: int, axis: int): float { }
fn GetGamepadAxisCount(gamepad: int): int { }
fn GetGamepadName(gamepad: int): string { }
fn SetGamepadMappings(mappings: string) { }

fn LoadImage(fileName: string): Image { }
fn UnloadImage(image: Image) { }
fn LoadTexture(fileName: string): Texture { }
fn LoadTextureFromImage(image: Image): Texture { }
fn SetTextureFilter(texture: Texture, filter: int) { }
fn UnloadTexture(texture: Texture) { }
fn DrawTexture(texture: Texture, posX: int, posY: int, tint: Color) { }
fn DrawTextureV(texture: Texture, position: Vector2, tint: Color) { }
fn DrawTextureEx(texture: Texture, position: Vector2, rotation: float, scale: float, tint: Color) { }
fn DrawTextureRec(texture: Texture, source: Rectangle, position: Vector2, tint: Color) { }
fn DrawTexturePro(texture: Texture, source: Rectangle, dest: Rectangle, origin: Vector2, rotation: float, tint: Color) { }
fn GenImageChecked(width: int, height: int, checksX: int, checksY: int, col1: Color, col2: Color): Image { }
fn ImageCrop(image: Image, crop: Rectangle) { }
fn ImageResize(image: Image, newWidth: int, newHeight: int) { }
fn ImageResizeNN(image: Image, newWidth: int, newHeight: int) { }
fn ImageResizeCanvas(image: Image, newWidth: int, newHeight: int, offsetX: int, offsetY: int, fill: Color) { }
fn ImageFormat(image: Image, newFormat: int) { }
fn ImageDraw(dst: Image, src: Image, srcRec: Rectangle, dstRec: Rectangle, tint: Color) { }
fn ImageFlipHorizontal(image: Image) { }
fn ImageFlipVertical(image: Image) { }
fn ExportImage(image: Image, fileName: string): bool { }

fn LoadFont(fileName: string): Font { }
fn LoadFontEx(path: string, font_size: int, codepoints: array, codepoint_count: int): Font { }
fn UnloadFont(font: Font) { }
fn DrawTextEx(font: Font, text: string, position: Vector2, size: float, spacing: float, color: Color) { }
fn MeasureTextEx(font: Font, text: string, size: float, spacing: float): Vector2 { }
fn DrawText(text: string, posX: int, posY: int, fontSize: int, color: Color) { }
fn MeasureText(text: string, fontSize: int): int { }
fn DrawTextPro(font: Font, text: string, position: Vector2, origin: Vector2, rotation: float, size: float, spacing: float, tint: Color) { }

fn InitAudioDevice() { }
fn CloseAudioDevice() { }
fn LoadSound(fileName: string): Sound { }
fn PlaySound(sound: Sound) { }
fn StopSound(sound: Sound) { }
fn PauseSound(sound: Sound) { }
fn ResumeSound(sound: Sound) { }
fn UnloadSound(sound: Sound) { }
fn IsSoundPlaying(sound: Sound): bool { }
fn SetSoundVolume(sound: Sound, volume: float) { }
fn SetSoundPitch(sound: Sound, pitch: float) { }
fn LoadMusicStream(fileName: string): Music { }
fn UnloadMusicStream(music: Music) { }
fn PlayMusicStream(music: Music) { }
fn UpdateMusicStream(music: Music) { }
fn StopMusicStream(music: Music) { }
fn PauseMusicStream(music: Music) { }
fn ResumeMusicStream(music: Music) { }
fn IsMusicStreamPlaying(music: Music): bool { }
fn SetMusicVolume(music: Music, volume: float) { }
fn SetMusicPitch(music: Music, pitch: float) { }
fn SetMusicPan(music: Music, pan: float) { }
fn GetMusicTimeLength(music: Music): float { }
fn GetMusicTimePlayed(music: Music): float { }

fn BeginMode3D(camera: Camera3D) { }
fn EndMode3D() { }
fn DrawLine3D(start: Vector3, end: Vector3, color: Color) { }
fn DrawPoint3D(pos: Vector3, color: Color) { }
fn DrawCube(pos: Vector3, w: float, h: float, l: float, color: Color) { }
fn DrawCubeV(pos: Vector3, size: Vector3, color: Color) { }
fn DrawCubeWires(pos: Vector3, w: float, h: float, l: float, color: Color) { }
fn DrawCubeWiresV(pos: Vector3, size: Vector3, color: Color) { }
fn DrawSphere(center: Vector3, radius: float, color: Color) { }
fn DrawSphereEx(center: Vector3, radius: float, rings: int, slices: int, color: Color) { }
fn DrawSphereWires(center: Vector3, radius: float, rings: int, slices: int, color: Color) { }
fn DrawCylinder(pos: Vector3, rTop: float, rBot: float, height: float, slices: int, color: Color) { }
fn DrawCylinderWires(pos: Vector3, rTop: float, rBot: float, height: float, slices: int, color: Color) { }
fn DrawPlane(center: Vector3, size: Vector2, color: Color) { }
fn DrawRay(ray: Ray, color: Color) { }
fn DrawGrid(slices: int, spacing: float) { }
fn DrawBoundingBox(box: BoundingBox, color: Color) { }
fn DrawCircle3D(center: Vector3, radius: float, rotAxis: Vector3, rotAngle: float, color: Color) { }
fn DrawTriangle3D(v1: Vector3, v2: Vector3, v3: Vector3, color: Color) { }
fn CheckCollisionSpheres(c1: Vector3, r1: float, c2: Vector3, r2: float): bool { }
fn CheckCollisionBoxes(b1: BoundingBox, b2: BoundingBox): bool { }
fn CheckCollisionBoxSphere(box: BoundingBox, center: Vector3, radius: float): bool { }
fn GetRayCollisionSphere(ray: Ray, center: Vector3, radius: float): RayCollision { }
fn GetRayCollisionBox(ray: Ray, box: BoundingBox): RayCollision { }
fn GetRayCollisionMesh(ray: Ray, mesh: Mesh, transform: Matrix): RayCollision { }
fn GetRayCollisionQuad(ray: Ray, p1: Vector3, p2: Vector3, p3: Vector3, p4: Vector3): RayCollision { }
fn GetScreenToWorldRay(mousePos: Vector2, camera: Camera3D): Ray { }
fn GetWorldToScreen(pos: Vector3, camera: Camera3D): Vector2 { }
fn GetScreenToWorld2D(pos: Vector2, camera: Camera2D): Vector2 { }
fn GetWorldToScreen2D(pos: Vector2, camera: Camera2D): Vector2 { }
fn GetCameraMatrix(camera: Camera3D): Matrix { }
fn GetCameraMatrix2D(camera: Camera2D): Matrix { }
fn UpdateCamera(camera: Camera3D, mode: int) { }
fn UpdateCameraPro(camera: Camera3D, movement: Vector3, rotation: Vector3, zoom: float) { }
fn DrawModel(model: Model, pos: Vector3, scale: float, tint: Color) { }
fn DrawModelEx(model: Model, pos: Vector3, rotAxis: Vector3, rotAngle: float, scale: Vector3, tint: Color) { }
fn DrawBillboard(camera: Camera3D, tex: Texture, pos: Vector3, scale: float, tint: Color) { }
fn LoadModel(fileName: string): Model { }
fn IsModelValid(model: Model): bool { }
fn UnloadModel(model: Model) { }
fn LoadModelAnimations(fileName: string): array { }
fn UpdateModelAnimation(model: Model, anim: ModelAnimation, frame: int) { }
fn IsModelAnimationValid(model: Model, anim: ModelAnimation): bool { }
fn UnloadModelAnimation(anim: ModelAnimation) { }

fn LoadShader(vs: string, fs: string): Shader { }
fn LoadShaderFromMemory(vs: string, fs: string): Shader { }
fn IsShaderValid(shader: Shader): bool { }
fn UnloadShader(shader: Shader) { }
fn BeginShaderMode(shader: Shader) { }
fn EndShaderMode() { }
fn GetShaderLocation(shader: Shader, uniformName: string): int { }
fn GetShaderLocationAttrib(shader: Shader, attribName: string): int { }
fn SetShaderValue(shader: Shader, locIndex: int, value, uniformType: int) { }
fn SetShaderValueMatrix(shader: Shader, locIndex: int, matrix: Matrix) { }
fn SetShaderValueTexture(shader: Shader, locIndex: int, texture: Texture) { }

fn LoadRenderTexture(width: int, height: int): RenderTexture { }
fn UnloadRenderTexture(target: RenderTexture) { }
fn BeginTextureMode(target: RenderTexture) { }
fn EndTextureMode() { }

fn GenMeshCube(width: float, height: float, length: float): Mesh { }
fn GenMeshPlane(width: float, length: float, resX: int, resZ: int): Mesh { }
fn GenMeshSphere(radius: float, rings: int, slices: int): Mesh { }
fn GenMeshCylinder(radius: float, height: float, slices: int): Mesh { }
fn GenMeshTorus(radius: float, size: float, radSeg: int, sides: int): Mesh { }
fn GenMeshHeightmap(image: Image, size: Vector3): Mesh { }
fn GenMeshCubicmap(cubicmap: Image, cubeSize: Vector3): Mesh { }
fn UploadMesh(mesh: Mesh, dynamic: bool) { }
fn UnloadMesh(mesh: Mesh) { }
fn DrawMesh(mesh: Mesh, material: Material, transform: Matrix) { }
fn DrawMeshInstanced(mesh: Mesh, material: Material, transforms: array) { }
fn GetMeshBoundingBox(mesh: Mesh): BoundingBox { }
fn ExportMesh(mesh: Mesh, fileName: string): bool { }

fn LoadMaterialDefault(): Material { }
fn IsMaterialValid(material: Material): bool { }
fn UnloadMaterial(material: Material) { }
fn SetMaterialTexture(material: Material, mapType: int, texture: Texture) { }

fn FileExists(fileName: string): bool { }
fn DirectoryExists(dirPath: string): bool { }
fn IsFileExtension(fileName: string, ext: string): bool { }
fn GetFileExtension(fileName: string): string { }
fn GetFileName(filePath: string): string { }
fn GetFileNameWithoutExt(filePath: string): string { }
fn GetDirectoryPath(filePath: string): string { }
fn GetWorkingDirectory(): string { }
fn GetApplicationDirectory(): string { }
fn GetMonitorCount(): int { }
fn GetCurrentMonitor(): int { }
fn GetMonitorWidth(monitor: int): int { }
fn GetMonitorHeight(monitor: int): int { }
fn GetMonitorRefreshRate(monitor: int): int { }
fn SetWindowTitle(title: string) { }
fn SetWindowPosition(x: int, y: int) { }
fn SetWindowSize(width: int, height: int) { }
fn SetWindowMinSize(width: int, height: int) { }
fn SetWindowOpacity(opacity: float) { }
fn SetWindowState(flags: int) { }
fn ClearWindowState(flags: int) { }
fn IsWindowState(flags: int): bool { }
fn OpenURL(url: string) { }
fn SetClipboardText(text: string) { }
fn GetClipboardText(): string { }
fn TakeScreenshot(fileName: string) { }
fn SetRandomSeed(seed: int) { }
fn LoadFileData(fileName: string): bytes { }
fn SetTraceLogLevel(logLevel: int) { }
fn TraceLog(logLevel: int, message: string) { }
fn SaveFileData(fileName: string, data: bytes): bool { }

fn rlSetTexture(id: int) { }
fn rlBegin(mode: int) { }
fn rlEnd() { }
fn rlTexCoord2f(u: float, v: float) { }
fn rlVertex3f(x: float, y: float, z: float) { }
fn rlColor4ub(r: int, g: int, b: int, a: int) { }

let LIGHTGRAY: Color
let GRAY: Color
let DARKGRAY: Color
let YELLOW: Color
let GOLD: Color
let ORANGE: Color
let PINK: Color
let RED: Color
let MAROON: Color
let GREEN: Color
let LIME: Color
let DARKGREEN: Color
let SKYBLUE: Color
let BLUE: Color
let DARKBLUE: Color
let PURPLE: Color
let VIOLET: Color
let DARKPURPLE: Color
let BEIGE: Color
let BROWN: Color
let DARKBROWN: Color
let WHITE: Color
let BLACK: Color
let BLANK: Color
let MAGENTA: Color
let RAYWHITE: Color

let KEY_NULL: int
let KEY_SPACE: int
let KEY_APOSTROPHE: int
let KEY_COMMA: int
let KEY_MINUS: int
let KEY_PERIOD: int
let KEY_SLASH: int
let KEY_0: int
let KEY_1: int
let KEY_2: int
let KEY_3: int
let KEY_4: int
let KEY_5: int
let KEY_6: int
let KEY_7: int
let KEY_8: int
let KEY_9: int
let KEY_SEMICOLON: int
let KEY_EQUAL: int
let KEY_A: int
let KEY_B: int
let KEY_C: int
let KEY_D: int
let KEY_E: int
let KEY_F: int
let KEY_G: int
let KEY_H: int
let KEY_I: int
let KEY_J: int
let KEY_K: int
let KEY_L: int
let KEY_M: int
let KEY_N: int
let KEY_O: int
let KEY_P: int
let KEY_Q: int
let KEY_R: int
let KEY_S: int
let KEY_T: int
let KEY_U: int
let KEY_V: int
let KEY_W: int
let KEY_X: int
let KEY_Y: int
let KEY_Z: int
let KEY_LEFT_BRACKET: int
let KEY_BACKSLASH: int
let KEY_RIGHT_BRACKET: int
let KEY_GRAVE: int
let KEY_ESCAPE: int
let KEY_ENTER: int
let KEY_TAB: int
let KEY_BACKSPACE: int
let KEY_INSERT: int
let KEY_DELETE: int
let KEY_RIGHT: int
let KEY_LEFT: int
let KEY_DOWN: int
let KEY_UP: int
let KEY_PAGEUP: int
let KEY_PAGEDOWN: int
let KEY_HOME: int
let KEY_END: int
let KEY_CAPS_LOCK: int
let KEY_SCROLL_LOCK: int
let KEY_NUM_LOCK: int
let KEY_PRINT_SCREEN: int
let KEY_PAUSE: int
let KEY_F1: int
let KEY_F2: int
let KEY_F3: int
let KEY_F4: int
let KEY_F5: int
let KEY_F6: int
let KEY_F7: int
let KEY_F8: int
let KEY_F9: int
let KEY_F10: int
let KEY_F11: int
let KEY_F12: int
let KEY_LEFT_SHIFT: int
let KEY_LEFT_CONTROL: int
let KEY_LEFT_ALT: int
let KEY_LEFT_SUPER: int
let KEY_RIGHT_SHIFT: int
let KEY_RIGHT_CONTROL: int
let KEY_RIGHT_ALT: int
let KEY_RIGHT_SUPER: int
let KEY_KB_MENU: int
let KEY_KP_0: int
let KEY_KP_1: int
let KEY_KP_2: int
let KEY_KP_3: int
let KEY_KP_4: int
let KEY_KP_5: int
let KEY_KP_6: int
let KEY_KP_7: int
let KEY_KP_8: int
let KEY_KP_9: int
let KEY_KP_DECIMAL: int
let KEY_KP_DIVIDE: int
let KEY_KP_MULTIPLY: int
let KEY_KP_SUBTRACT: int
let KEY_KP_ADD: int
let KEY_KP_ENTER: int
let KEY_KP_EQUAL: int
let KEY_BACK: int
let KEY_MENU: int
let KEY_VOLUME_UP: int
let KEY_VOLUME_DOWN: int

let MOUSE_LEFT_BUTTON: int
let MOUSE_RIGHT_BUTTON: int
let MOUSE_MIDDLE_BUTTON: int
let MOUSE_BUTTON_LEFT: int
let MOUSE_BUTTON_RIGHT: int
let MOUSE_BUTTON_MIDDLE: int
let MOUSE_BUTTON_SIDE: int
let MOUSE_BUTTON_EXTRA: int
let MOUSE_BUTTON_FORWARD: int
let MOUSE_BUTTON_BACK: int
let MOUSE_CURSOR_DEFAULT: int
let MOUSE_CURSOR_ARROW: int
let MOUSE_CURSOR_IBEAM: int
let MOUSE_CURSOR_CROSSHAIR: int
let MOUSE_CURSOR_POINTING_HAND: int
let MOUSE_CURSOR_RESIZE_EW: int
let MOUSE_CURSOR_RESIZE_NS: int
let MOUSE_CURSOR_RESIZE_NWSE: int
let MOUSE_CURSOR_RESIZE_NESW: int
let MOUSE_CURSOR_RESIZE_ALL: int
let MOUSE_CURSOR_NOT_ALLOWED: int

let FLAG_VSYNC_HINT: int
let FLAG_FULLSCREEN_MODE: int
let FLAG_WINDOW_RESIZABLE: int
let FLAG_WINDOW_UNDECORATED: int
let FLAG_WINDOW_TRANSPARENT: int
let FLAG_WINDOW_HIDDEN: int
let FLAG_WINDOW_MINIMIZED: int
let FLAG_WINDOW_MAXIMIZED: int
let FLAG_WINDOW_UNFOCUSED: int
let FLAG_WINDOW_TOPMOST: int
let FLAG_WINDOW_ALWAYS_RUN: int
let FLAG_WINDOW_HIGHDPI: int
let FLAG_WINDOW_MOUSE_PASSTHROUGH: int
let FLAG_BORDERLESS_WINDOWED_MODE: int
let FLAG_MSAA_4X_HINT: int
let FLAG_INTERLACED_HINT: int

let CAMERA_CUSTOM: int
let CAMERA_FREE: int
let CAMERA_ORBITAL: int
let CAMERA_FIRST_PERSON: int
let CAMERA_THIRD_PERSON: int
let CAMERA_PERSPECTIVE: int
let CAMERA_ORTHOGRAPHIC: int

let SHADER_UNIFORM_FLOAT: int
let SHADER_UNIFORM_VEC2: int
let SHADER_UNIFORM_VEC3: int
let SHADER_UNIFORM_VEC4: int
let SHADER_UNIFORM_INT: int
let SHADER_UNIFORM_IVEC2: int
let SHADER_UNIFORM_IVEC3: int
let SHADER_UNIFORM_IVEC4: int
let SHADER_UNIFORM_SAMPLER2D: int

let TEXTURE_FILTER_POINT: int
let TEXTURE_FILTER_BILINEAR: int
let TEXTURE_FILTER_TRILINEAR: int
let TEXTURE_FILTER_ANISOTROPIC_4X: int
let TEXTURE_FILTER_ANISOTROPIC_8X: int
let TEXTURE_FILTER_ANISOTROPIC_16X: int
let TEXTURE_WRAP_REPEAT: int
let TEXTURE_WRAP_CLAMP: int
let TEXTURE_WRAP_MIRROR_REPEAT: int
let TEXTURE_WRAP_MIRROR_CLAMP: int

let BLEND_ALPHA: int
let BLEND_ADDITIVE: int
let BLEND_MULTIPLIED: int
let BLEND_ADD_COLORS: int
let BLEND_SUBTRACT_COLORS: int
let BLEND_ALPHA_PREMULTIPLY: int
let BLEND_CUSTOM: int
let BLEND_CUSTOM_SEPARATE: int

let LOG_ALL: int
let LOG_TRACE: int
let LOG_DEBUG: int
let LOG_INFO: int
let LOG_WARNING: int
let LOG_ERROR: int
let LOG_FATAL: int
let LOG_NONE: int

let GAMEPAD_BUTTON_UNKNOWN: int
let GAMEPAD_BUTTON_LEFT_FACE_UP: int
let GAMEPAD_BUTTON_LEFT_FACE_RIGHT: int
let GAMEPAD_BUTTON_LEFT_FACE_DOWN: int
let GAMEPAD_BUTTON_LEFT_FACE_LEFT: int
let GAMEPAD_BUTTON_RIGHT_FACE_UP: int
let GAMEPAD_BUTTON_RIGHT_FACE_RIGHT: int
let GAMEPAD_BUTTON_RIGHT_FACE_DOWN: int
let GAMEPAD_BUTTON_RIGHT_FACE_LEFT: int
let GAMEPAD_BUTTON_LEFT_TRIGGER_1: int
let GAMEPAD_BUTTON_LEFT_TRIGGER_2: int
let GAMEPAD_BUTTON_RIGHT_TRIGGER_1: int
let GAMEPAD_BUTTON_RIGHT_TRIGGER_2: int
let GAMEPAD_BUTTON_MIDDLE_LEFT: int
let GAMEPAD_BUTTON_MIDDLE: int
let GAMEPAD_BUTTON_MIDDLE_RIGHT: int
let GAMEPAD_BUTTON_LEFT_THUMB: int
let GAMEPAD_BUTTON_RIGHT_THUMB: int

let GAMEPAD_AXIS_LEFT_X: int
let GAMEPAD_AXIS_LEFT_Y: int
let GAMEPAD_AXIS_RIGHT_X: int
let GAMEPAD_AXIS_RIGHT_Y: int
let GAMEPAD_AXIS_LEFT_TRIGGER: int
let GAMEPAD_AXIS_RIGHT_TRIGGER: int

let MATERIAL_MAP_ALBEDO: int
let MATERIAL_MAP_METALNESS: int
let MATERIAL_MAP_NORMAL: int
let MATERIAL_MAP_ROUGHNESS: int
let MATERIAL_MAP_OCCLUSION: int
let MATERIAL_MAP_EMISSION: int
let MATERIAL_MAP_HEIGHT: int
let MATERIAL_MAP_CUBEMAP: int
let MATERIAL_MAP_IRRADIANCE: int
let MATERIAL_MAP_PREFILTER: int
let MATERIAL_MAP_BRDF: int

let SHADER_LOC_VERTEX_POSITION: int
let SHADER_LOC_VERTEX_TEXCOORD01: int
let SHADER_LOC_VERTEX_TEXCOORD02: int
let SHADER_LOC_VERTEX_NORMAL: int
let SHADER_LOC_VERTEX_TANGENT: int
let SHADER_LOC_VERTEX_COLOR: int
let SHADER_LOC_MATRIX_MVP: int
let SHADER_LOC_MATRIX_VIEW: int
let SHADER_LOC_MATRIX_PROJECTION: int
let SHADER_LOC_MATRIX_MODEL: int
let SHADER_LOC_MATRIX_NORMAL: int
let SHADER_LOC_VECTOR_VIEW: int
let SHADER_LOC_COLOR_DIFFUSE: int
let SHADER_LOC_COLOR_SPECULAR: int
let SHADER_LOC_COLOR_AMBIENT: int
let SHADER_LOC_MAP_ALBEDO: int
let SHADER_LOC_MAP_METALNESS: int
let SHADER_LOC_MAP_NORMAL: int
let SHADER_LOC_MAP_ROUGHNESS: int
let SHADER_LOC_MAP_OCCLUSION: int
let SHADER_LOC_MAP_EMISSION: int
let SHADER_LOC_MAP_HEIGHT: int
let SHADER_LOC_MAP_CUBEMAP: int
let SHADER_LOC_MAP_IRRADIANCE: int
let SHADER_LOC_MAP_PREFILTER: int
let SHADER_LOC_MAP_BRDF: int
let SHADER_LOC_VERTEX_BONEIDS: int
let SHADER_LOC_VERTEX_BONEWEIGHTS: int
let SHADER_LOC_BONE_MATRICES: int

let SHADER_ATTRIB_FLOAT: int
let SHADER_ATTRIB_VEC2: int
let SHADER_ATTRIB_VEC3: int
let SHADER_ATTRIB_VEC4: int

let PIXELFORMAT_UNCOMPRESSED_GRAYSCALE: int
let PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA: int
let PIXELFORMAT_UNCOMPRESSED_R5G6B5: int
let PIXELFORMAT_UNCOMPRESSED_R8G8B8: int
let PIXELFORMAT_UNCOMPRESSED_R5G5B5A1: int
let PIXELFORMAT_UNCOMPRESSED_R4G4B4A4: int
let PIXELFORMAT_UNCOMPRESSED_R8G8B8A8: int
let PIXELFORMAT_UNCOMPRESSED_R32: int
let PIXELFORMAT_UNCOMPRESSED_R32G32B32: int
let PIXELFORMAT_UNCOMPRESSED_R32G32B32A32: int
let PIXELFORMAT_UNCOMPRESSED_R16: int
let PIXELFORMAT_UNCOMPRESSED_R16G16B16: int
let PIXELFORMAT_UNCOMPRESSED_R16G16B16A16: int
let PIXELFORMAT_COMPRESSED_DXT1_RGB: int
let PIXELFORMAT_COMPRESSED_DXT1_RGBA: int
let PIXELFORMAT_COMPRESSED_DXT3_RGBA: int
let PIXELFORMAT_COMPRESSED_DXT5_RGBA: int
let PIXELFORMAT_COMPRESSED_ETC1_RGB: int
let PIXELFORMAT_COMPRESSED_ETC2_RGB: int
let PIXELFORMAT_COMPRESSED_ETC2_EAC_RGBA: int
let PIXELFORMAT_COMPRESSED_PVRT_RGB: int
let PIXELFORMAT_COMPRESSED_PVRT_RGBA: int
let PIXELFORMAT_COMPRESSED_ASTC_4x4_RGBA: int
let PIXELFORMAT_COMPRESSED_ASTC_8x8_RGBA: int

let CUBEMAP_LAYOUT_AUTO_DETECT: int
let CUBEMAP_LAYOUT_LINE_VERTICAL: int
let CUBEMAP_LAYOUT_LINE_HORIZONTAL: int
let CUBEMAP_LAYOUT_CROSS_THREE_BY_FOUR: int
let CUBEMAP_LAYOUT_CROSS_FOUR_BY_THREE: int

let FONT_DEFAULT: int
let FONT_BITMAP: int
let FONT_SDF: int

let GESTURE_NONE: int
let GESTURE_TAP: int
let GESTURE_DOUBLETAP: int
let GESTURE_HOLD: int
let GESTURE_DRAG: int
let GESTURE_SWIPE_RIGHT: int
let GESTURE_SWIPE_LEFT: int
let GESTURE_SWIPE_UP: int
let GESTURE_SWIPE_DOWN: int
let GESTURE_PINCH_IN: int
let GESTURE_PINCH_OUT: int

let NPATCH_NINE_PATCH: int
let NPATCH_THREE_PATCH_VERTICAL: int
let NPATCH_THREE_PATCH_HORIZONTAL: int
