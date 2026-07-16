#include "raylib_binding.hpp"
#include <cstring>

namespace pyle {
namespace raylib_binding {

    void register_3d(NativeModule& mod) {
        VM& vm = mod.get_vm();

        ClassBinder<RayCollision> rcol(vm, "RayCollision");
        rcol.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<RayCollision>(vm, new RayCollision{});
        })
            .member<bool, &RayCollision::hit>("hit")
            .member<float, &RayCollision::distance>("distance")
            .custom_getter("point", +[](VM& vm, HeapIdx o, ArgView) -> Value {
                auto* r = native_at<RayCollision>(vm, o, "RayCollision");
                if (!r) return Value();
                return to_value_owned<Vector3>(vm, new Vector3{ r->point.x, r->point.y, r->point.z });
            })
            .custom_getter("normal", +[](VM& vm, HeapIdx o, ArgView) -> Value {
                auto* r = native_at<RayCollision>(vm, o, "RayCollision");
                if (!r) return Value();
                return to_value_owned<Vector3>(vm, new Vector3{ r->normal.x, r->normal.y, r->normal.z });
            });
        mod.class_type(rcol);

        ClassBinder<Camera3D> cam3(vm, "Camera3D");
        cam3.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<Camera3D>(vm, new Camera3D{});
        })
            .custom_getter("position", +[](VM& vm, HeapIdx o, ArgView) -> Value {
                auto* c = native_at<Camera3D>(vm, o, "Camera3D");
                if (!c) return Value();
                return to_value_owned<Vector3>(vm, new Vector3{ c->position.x, c->position.y, c->position.z });
            })
            .custom_setter("position", +[](VM& vm, HeapIdx o, ArgView a) -> Value {
                auto* c = native_at<Camera3D>(vm, o, "Camera3D");
                auto* v = as_native<Vector3>(vm, a[0], "Vector3");
                if (!c || !v) return Value();
                c->position = *v;
                return a[0];
            })
            .custom_getter("target", +[](VM& vm, HeapIdx o, ArgView) -> Value {
                auto* c = native_at<Camera3D>(vm, o, "Camera3D");
                if (!c) return Value();
                return to_value_owned<Vector3>(vm, new Vector3{ c->target.x, c->target.y, c->target.z });
            })
            .custom_setter("target", +[](VM& vm, HeapIdx o, ArgView a) -> Value {
                auto* c = native_at<Camera3D>(vm, o, "Camera3D");
                auto* v = as_native<Vector3>(vm, a[0], "Vector3");
                if (!c || !v) return Value();
                c->target = *v;
                return a[0];
            })
            .custom_getter("up", +[](VM& vm, HeapIdx o, ArgView) -> Value {
                auto* c = native_at<Camera3D>(vm, o, "Camera3D");
                if (!c) return Value();
                return to_value_owned<Vector3>(vm, new Vector3{ c->up.x, c->up.y, c->up.z });
            })
            .custom_setter("up", +[](VM& vm, HeapIdx o, ArgView a) -> Value {
                auto* c = native_at<Camera3D>(vm, o, "Camera3D");
                auto* v = as_native<Vector3>(vm, a[0], "Vector3");
                if (!c || !v) return Value();
                c->up = *v;
                return a[0];
            })
            .member<float, &Camera3D::fovy>("fovy")
            .member<int, &Camera3D::projection>("projection");
        mod.class_type(cam3);

        ClassBinder<BoundingBox> bb(vm, "BoundingBox");
        bb.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<BoundingBox>(vm, new BoundingBox{});
        })
            .custom_getter("min", +[](VM& vm, HeapIdx o, ArgView) -> Value {
                auto* b = native_at<BoundingBox>(vm, o, "BoundingBox");
                if (!b) return Value();
                return to_value_owned<Vector3>(vm, new Vector3{ b->min.x, b->min.y, b->min.z });
            })
            .custom_setter("min", +[](VM& vm, HeapIdx o, ArgView a) -> Value {
                auto* b = native_at<BoundingBox>(vm, o, "BoundingBox");
                auto* v = as_native<Vector3>(vm, a[0], "Vector3");
                if (!b || !v) return Value();
                b->min = *v;
                return a[0];
            })
            .custom_getter("max", +[](VM& vm, HeapIdx o, ArgView) -> Value {
                auto* b = native_at<BoundingBox>(vm, o, "BoundingBox");
                if (!b) return Value();
                return to_value_owned<Vector3>(vm, new Vector3{ b->max.x, b->max.y, b->max.z });
            })
            .custom_setter("max", +[](VM& vm, HeapIdx o, ArgView a) -> Value {
                auto* b = native_at<BoundingBox>(vm, o, "BoundingBox");
                auto* v = as_native<Vector3>(vm, a[0], "Vector3");
                if (!b || !v) return Value();
                b->max = *v;
                return a[0];
            });
        mod.class_type(bb);

        ClassBinder<Ray> ray(vm, "Ray");
        ray.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<Ray>(vm, new Ray{});
        })
            .custom_getter("position", +[](VM& vm, HeapIdx o, ArgView) -> Value {
                auto* r = native_at<Ray>(vm, o, "Ray");
                if (!r) return Value();
                return to_value_owned<Vector3>(vm, new Vector3{ r->position.x, r->position.y, r->position.z });
            })
            .custom_setter("position", +[](VM& vm, HeapIdx o, ArgView a) -> Value {
                auto* r = native_at<Ray>(vm, o, "Ray");
                auto* v = as_native<Vector3>(vm, a[0], "Vector3");
                if (!r || !v) return Value();
                r->position = *v;
                return a[0];
            })
            .custom_getter("direction", +[](VM& vm, HeapIdx o, ArgView) -> Value {
                auto* r = native_at<Ray>(vm, o, "Ray");
                if (!r) return Value();
                return to_value_owned<Vector3>(vm, new Vector3{ r->direction.x, r->direction.y, r->direction.z });
            })
            .custom_setter("direction", +[](VM& vm, HeapIdx o, ArgView a) -> Value {
                auto* r = native_at<Ray>(vm, o, "Ray");
                auto* v = as_native<Vector3>(vm, a[0], "Vector3");
                if (!r || !v) return Value();
                r->direction = *v;
                return a[0];
            });
        mod.class_type(ray);

        ClassBinder<Model> model(vm, "Model");
        model.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<Model>(vm, new Model{});
        });
        mod.class_type(model);

        mod.raw_function("BeginMode3D", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "BeginMode3D expects (Camera3D)."); return Value(); }
            Camera3D* c = as_native<Camera3D>(vm, args[0], "Camera3D");
            if (!c) return Value();
            BeginMode3D(*c);
            return Value();
        });
        
        mod.function<EndMode3D>("EndMode3D");

        mod.raw_function("DrawLine3D", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawLine3D expects (start, end, color)."); return Value(); }
            Vector3* s = as_native<Vector3>(vm, args[0], "Vector3");
            Vector3* e = as_native<Vector3>(vm, args[1], "Vector3");
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!s || !e || !c) return Value();
            DrawLine3D(*s, *e, *c);
            return Value();
        });
        mod.raw_function("DrawPoint3D", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "DrawPoint3D expects (pos, color)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            Color* c = as_native<Color>(vm, args[1], "Color");
            if (!p || !c) return Value();
            DrawPoint3D(*p, *c);
            return Value();
        });
        mod.raw_function("DrawCube", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCube expects (pos, w, h, l, color)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            float w = from_value<float>(vm, args[1]);
            float h = from_value<float>(vm, args[2]);
            float l = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!p || !c) return Value();
            DrawCube(*p, w, h, l, *c);
            return Value();
        });
        mod.raw_function("DrawCubeV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCubeV expects (pos, size, color)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            Vector3* s = as_native<Vector3>(vm, args[1], "Vector3");
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!p || !s || !c) return Value();
            DrawCubeV(*p, *s, *c);
            return Value();
        });
        mod.raw_function("DrawCubeWires", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCubeWires expects (pos, w, h, l, color)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            float w = from_value<float>(vm, args[1]);
            float h = from_value<float>(vm, args[2]);
            float l = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!p || !c) return Value();
            DrawCubeWires(*p, w, h, l, *c);
            return Value();
        });
        mod.raw_function("DrawCubeWiresV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCubeWiresV expects (pos, size, color)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            Vector3* s = as_native<Vector3>(vm, args[1], "Vector3");
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!p || !s || !c) return Value();
            DrawCubeWiresV(*p, *s, *c);
            return Value();
        });
        mod.raw_function("DrawSphere", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawSphere expects (center, radius, color)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            float r = from_value<float>(vm, args[1]);
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!p || !c) return Value();
            DrawSphere(*p, r, *c);
            return Value();
        });
        mod.raw_function("DrawSphereEx", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawSphereEx expects (center, radius, rings, slices, color)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            float r = from_value<float>(vm, args[1]);
            int rings = from_value<int64_t>(vm, args[2]);
            int slices = from_value<int64_t>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!p || !c) return Value();
            DrawSphereEx(*p, r, rings, slices, *c);
            return Value();
        });
        mod.raw_function("DrawSphereWires", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawSphereWires expects (center, radius, rings, slices, color)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            float r = from_value<float>(vm, args[1]);
            int rings = from_value<int64_t>(vm, args[2]);
            int slices = from_value<int64_t>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!p || !c) return Value();
            DrawSphereWires(*p, r, rings, slices, *c);
            return Value();
        });
        mod.raw_function("DrawCylinder", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCylinder expects (pos, rTop, rBot, height, slices, color)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            float rt = from_value<float>(vm, args[1]);
            float rb = from_value<float>(vm, args[2]);
            float h = from_value<float>(vm, args[3]);
            int sl = from_value<int64_t>(vm, args[4]);
            Color* c = as_native<Color>(vm, args[5], "Color");
            if (!p || !c) return Value();
            DrawCylinder(*p, rt, rb, h, sl, *c);
            return Value();
        });
        mod.raw_function("DrawCylinderWires", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCylinderWires expects (pos, rTop, rBot, height, slices, color)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            float rt = from_value<float>(vm, args[1]);
            float rb = from_value<float>(vm, args[2]);
            float h = from_value<float>(vm, args[3]);
            int sl = from_value<int64_t>(vm, args[4]);
            Color* c = as_native<Color>(vm, args[5], "Color");
            if (!p || !c) return Value();
            DrawCylinderWires(*p, rt, rb, h, sl, *c);
            return Value();
        });
        mod.raw_function("DrawPlane", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawPlane expects (center, size, color)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            Vector2* s = as_native<Vector2>(vm, args[1], "Vector2");
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!p || !s || !c) return Value();
            DrawPlane(*p, *s, *c);
            return Value();
        });
        mod.raw_function("DrawRay", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRay expects (ray, color)."); return Value(); }
            Ray* r = as_native<Ray>(vm, args[0], "Ray");
            Color* c = as_native<Color>(vm, args[1], "Color");
            if (!r || !c) return Value();
            DrawRay(*r, *c);
            return Value();
        });
        mod.raw_function("DrawGrid", +[](VM& vm, ArgView args) -> Value {
            int slices = 10; float spacing = 1.0f;
            if (args.size() > 0) slices = from_value<int64_t>(vm, args[0]);
            if (args.size() > 1) spacing = from_value<float>(vm, args[1]);
            DrawGrid(slices, spacing);
            return Value();
        });
        mod.raw_function("DrawBoundingBox", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "DrawBoundingBox expects (box, color)."); return Value(); }
            BoundingBox* b = as_native<BoundingBox>(vm, args[0], "BoundingBox");
            Color* c = as_native<Color>(vm, args[1], "Color");
            if (!b || !c) return Value();
            DrawBoundingBox(*b, *c);
            return Value();
        });
        mod.raw_function("DrawCircle3D", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCircle3D expects (center, radius, rotAxis, rotAngle, color)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            float r = from_value<float>(vm, args[1]);
            Vector3* a = as_native<Vector3>(vm, args[2], "Vector3");
            float ang = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!p || !a || !c) return Value();
            DrawCircle3D(*p, r, *a, ang, *c);
            return Value();
        });
        mod.raw_function("DrawTriangle3D", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTriangle3D expects (v1, v2, v3, color)."); return Value(); }
            Vector3* v1 = as_native<Vector3>(vm, args[0], "Vector3");
            Vector3* v2 = as_native<Vector3>(vm, args[1], "Vector3");
            Vector3* v3 = as_native<Vector3>(vm, args[2], "Vector3");
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!v1 || !v2 || !v3 || !c) return Value();
            DrawTriangle3D(*v1, *v2, *v3, *c);
            return Value();
        });

        mod.raw_function("CheckCollisionSpheres", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionSpheres expects (c1, r1, c2, r2)."); return Value(); }
            Vector3* c1 = as_native<Vector3>(vm, args[0], "Vector3");
            float r1 = from_value<float>(vm, args[1]);
            Vector3* c2 = as_native<Vector3>(vm, args[2], "Vector3");
            float r2 = from_value<float>(vm, args[3]);
            if (!c1 || !c2) return Value();
            return to_value(vm, CheckCollisionSpheres(*c1, r1, *c2, r2));
        });
        mod.raw_function("CheckCollisionBoxes", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionBoxes expects (b1, b2)."); return Value(); }
            BoundingBox* b1 = as_native<BoundingBox>(vm, args[0], "BoundingBox");
            BoundingBox* b2 = as_native<BoundingBox>(vm, args[1], "BoundingBox");
            if (!b1 || !b2) return Value();
            return to_value(vm, CheckCollisionBoxes(*b1, *b2));
        });
        mod.raw_function("CheckCollisionBoxSphere", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionBoxSphere expects (box, center, radius)."); return Value(); }
            BoundingBox* b = as_native<BoundingBox>(vm, args[0], "BoundingBox");
            Vector3* c = as_native<Vector3>(vm, args[1], "Vector3");
            float r = from_value<float>(vm, args[2]);
            if (!b || !c) return Value();
            return to_value(vm, CheckCollisionBoxSphere(*b, *c, r));
        });
        mod.raw_function("GetRayCollisionSphere", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "GetRayCollisionSphere expects (ray, center, radius)."); return Value(); }
            Ray* r = as_native<Ray>(vm, args[0], "Ray");
            Vector3* c = as_native<Vector3>(vm, args[1], "Vector3");
            float rad = from_value<float>(vm, args[2]);
            if (!r || !c) return Value();
            auto rc = GetRayCollisionSphere(*r, *c, rad);
            return to_value_owned<RayCollision>(vm, new RayCollision{ rc.hit, rc.distance, rc.point, rc.normal });
        });
        mod.raw_function("GetRayCollisionBox", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "GetRayCollisionBox expects (ray, box)."); return Value(); }
            Ray* r = as_native<Ray>(vm, args[0], "Ray");
            BoundingBox* b = as_native<BoundingBox>(vm, args[1], "BoundingBox");
            if (!r || !b) return Value();
            auto rc = GetRayCollisionBox(*r, *b);
            return to_value_owned<RayCollision>(vm, new RayCollision{ rc.hit, rc.distance, rc.point, rc.normal });
        });
        mod.raw_function("GetRayCollisionMesh", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "GetRayCollisionMesh expects (ray, mesh, transform)."); return Value(); }
            Ray* r = as_native<Ray>(vm, args[0], "Ray");
            Mesh* m = as_native<Mesh>(vm, args[1], "Mesh");
            Matrix* t = as_native<Matrix>(vm, args[2], "Matrix");
            if (!r || !m || !t) return Value();
            auto rc = GetRayCollisionMesh(*r, *m, *t);
            return to_value_owned<RayCollision>(vm, new RayCollision{ rc.hit, rc.distance, rc.point, rc.normal });
        });
        mod.raw_function("GetRayCollisionQuad", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "GetRayCollisionQuad expects (ray, p1, p2, p3, p4)."); return Value(); }
            Ray* r = as_native<Ray>(vm, args[0], "Ray");
            Vector3* p1 = as_native<Vector3>(vm, args[1], "Vector3");
            Vector3* p2 = as_native<Vector3>(vm, args[2], "Vector3");
            Vector3* p3 = as_native<Vector3>(vm, args[3], "Vector3");
            Vector3* p4 = as_native<Vector3>(vm, args[4], "Vector3");
            if (!r || !p1 || !p2 || !p3 || !p4) return Value();
            auto rc = GetRayCollisionQuad(*r, *p1, *p2, *p3, *p4);
            return to_value_owned<RayCollision>(vm, new RayCollision{ rc.hit, rc.distance, rc.point, rc.normal });
        });

        mod.raw_function("GetScreenToWorldRay", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "GetScreenToWorldRay expects (mousePos, camera)."); return Value(); }
            Vector2* m = as_native<Vector2>(vm, args[0], "Vector2");
            Camera3D* c = as_native<Camera3D>(vm, args[1], "Camera3D");
            if (!m || !c) return Value();
            Ray r = GetScreenToWorldRay(*m, *c);
            return to_value_owned<Ray>(vm, new Ray{ r.position, r.direction });
        });
        mod.raw_function("GetWorldToScreen", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "GetWorldToScreen expects (pos, camera)."); return Value(); }
            Vector3* p = as_native<Vector3>(vm, args[0], "Vector3");
            Camera3D* c = as_native<Camera3D>(vm, args[1], "Camera3D");
            if (!p || !c) return Value();
            Vector2 v = GetWorldToScreen(*p, *c);
            return to_value_owned<Vector2>(vm, new Vector2{ v.x, v.y });
        });
        mod.raw_function("GetScreenToWorld2D", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "GetScreenToWorld2D expects (pos, camera)."); return Value(); }
            Vector2* p = as_native<Vector2>(vm, args[0], "Vector2");
            Camera2D* c = as_native<Camera2D>(vm, args[1], "Camera2D");
            if (!p || !c) return Value();
            Vector2 v = GetScreenToWorld2D(*p, *c);
            return to_value_owned<Vector2>(vm, new Vector2{ v.x, v.y });
        });
        mod.raw_function("GetWorldToScreen2D", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "GetWorldToScreen2D expects (pos, camera)."); return Value(); }
            Vector2* p = as_native<Vector2>(vm, args[0], "Vector2");
            Camera2D* c = as_native<Camera2D>(vm, args[1], "Camera2D");
            if (!p || !c) return Value();
            Vector2 v = GetWorldToScreen2D(*p, *c);
            return to_value_owned<Vector2>(vm, new Vector2{ v.x, v.y });
        });
        mod.raw_function("GetCameraMatrix", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetCameraMatrix expects (camera)."); return Value(); }
            Camera3D* c = as_native<Camera3D>(vm, args[0], "Camera3D");
            if (!c) return Value();
            return to_value_owned<Matrix>(vm, new Matrix(GetCameraMatrix(*c)));
        });
        mod.raw_function("GetCameraMatrix2D", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetCameraMatrix2D expects (camera)."); return Value(); }
            Camera2D* c = as_native<Camera2D>(vm, args[0], "Camera2D");
            if (!c) return Value();
            return to_value_owned<Matrix>(vm, new Matrix(GetCameraMatrix2D(*c)));
        });
        
        // Use w_ wrapper for UpdateCamera to extract ptr safely or pass it directly 
        mod.raw_function("UpdateCamera", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "UpdateCamera expects (camera, mode)."); return Value(); }
            Camera3D* c = as_native<Camera3D>(vm, args[0], "Camera3D");
            int mode = from_value<int64_t>(vm, args[1]);
            if (!c) return Value();
            UpdateCamera(c, mode);
            return Value();
        });
        mod.raw_function("UpdateCameraPro", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "UpdateCameraPro expects (camera, movement, rotation, zoom)."); return Value(); }
            Camera3D* c = as_native<Camera3D>(vm, args[0], "Camera3D");
            Vector3* mov = as_native<Vector3>(vm, args[1], "Vector3");
            Vector3* rot = as_native<Vector3>(vm, args[2], "Vector3");
            float zoom = from_value<float>(vm, args[3]);
            if (!c || !mov || !rot) return Value();
            UpdateCameraPro(c, *mov, *rot, zoom);
            return Value();
        });

        mod.raw_function("DrawModel", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawModel expects (model, pos, scale, tint)."); return Value(); }
            Model* m = as_native<Model>(vm, args[0], "Model");
            Vector3* p = as_native<Vector3>(vm, args[1], "Vector3");
            float s = from_value<float>(vm, args[2]);
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!m || !p || !c) return Value();
            DrawModel(*m, *p, s, *c);
            return Value();
        });
        mod.raw_function("DrawModelEx", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "DrawModelEx expects (model, pos, rotAxis, rotAngle, scale, tint)."); return Value(); }
            Model* m = as_native<Model>(vm, args[0], "Model");
            Vector3* p = as_native<Vector3>(vm, args[1], "Vector3");
            Vector3* a = as_native<Vector3>(vm, args[2], "Vector3");
            float ang = from_value<float>(vm, args[3]);
            Vector3* s = as_native<Vector3>(vm, args[4], "Vector3");
            Color* c = as_native<Color>(vm, args[5], "Color");
            if (!m || !p || !a || !s || !c) return Value();
            DrawModelEx(*m, *p, *a, ang, *s, *c);
            return Value();
        });
        mod.raw_function("DrawBillboard", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawBillboard expects (camera, tex, pos, scale, tint)."); return Value(); }
            Camera3D* cam = as_native<Camera3D>(vm, args[0], "Camera3D");
            Texture2D* t = as_native<Texture2D>(vm, args[1], "Texture");
            Vector3* p = as_native<Vector3>(vm, args[2], "Vector3");
            float s = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!cam || !t || !p || !c) return Value();
            DrawBillboard(*cam, *t, *p, s, *c);
            return Value();
        });

        mod.raw_function("LoadModel", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadModel expects 1 string."); return Value(); }
            const std::string& path = from_value<std::string>(vm, args[0]);
            Model m = LoadModel(path.c_str());
            return to_value_owned<Model>(vm, new Model(m));
        });
        mod.raw_function("IsModelValid", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "IsModelValid expects 1 Model."); return Value(); }
            Model* m = as_native<Model>(vm, args[0], "Model");
            if (!m) return Value();
            return to_value(vm, IsModelValid(*m));
        });
        mod.raw_function("UnloadModel", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadModel expects 1 Model."); return Value(); }
            Model* m = as_native<Model>(vm, args[0], "Model");
            if (!m) return Value();
            UnloadModel(*m);
            return Value();
        });

        // ---------------------------------------------------------------- ModelAnimation
        ClassBinder<ModelAnimation> anim(vm, "ModelAnimation");
        anim.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<ModelAnimation>(vm, new ModelAnimation{});
        })
            .member<int, &ModelAnimation::boneCount>("boneCount")
            .member<int, &ModelAnimation::frameCount>("frameCount")
            .custom_getter("name", +[](VM& vm, HeapIdx o, ArgView) -> Value {
                auto* a = native_at<ModelAnimation>(vm, o, "ModelAnimation");
                if (!a) return Value();
                return to_value(vm, std::string(a->name));
            });
        mod.class_type(anim);

        mod.raw_function("LoadModelAnimations", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadModelAnimations expects 1 string (fileName)."); return Value(); }
            const std::string& path = from_value<std::string>(vm, args[0]);
            int animCount = 0;
            ModelAnimation* anims = LoadModelAnimations(path.c_str(), &animCount);
            Object arr(ArrayType{});
            auto& items = std::get<ArrayType>(arr.data);
            for (int i = 0; i < animCount; ++i) {
                ModelAnimation* a = new ModelAnimation{ anims[i].boneCount, anims[i].frameCount, anims[i].bones, anims[i].framePoses };
                memcpy(a->name, anims[i].name, 32);
                items.push_back(to_value_owned<ModelAnimation>(vm, a));
            }
            RL_FREE(anims);
            return Value(Value::Tag::ArrayRef, vm.alloc(std::move(arr)));
        });

        mod.raw_function("UpdateModelAnimation", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "UpdateModelAnimation expects (model, anim, frame)."); return Value(); }
            Model* m = as_native<Model>(vm, args[0], "Model");
            ModelAnimation* a = as_native<ModelAnimation>(vm, args[1], "ModelAnimation");
            int frame = from_value<int64_t>(vm, args[2]);
            if (!m || !a) return Value();
            UpdateModelAnimation(*m, *a, frame);
            return Value();
        });

        mod.raw_function("IsModelAnimationValid", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "IsModelAnimationValid expects (model, anim)."); return Value(); }
            Model* m = as_native<Model>(vm, args[0], "Model");
            ModelAnimation* a = as_native<ModelAnimation>(vm, args[1], "ModelAnimation");
            if (!m || !a) return Value();
            return to_value(vm, IsModelAnimationValid(*m, *a));
        });

        mod.raw_function("UnloadModelAnimation", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadModelAnimation expects (anim)."); return Value(); }
            ModelAnimation* a = as_native<ModelAnimation>(vm, args[0], "ModelAnimation");
            if (!a) return Value();
            UnloadModelAnimation(*a);
            return Value();
        });
    }

}
}
