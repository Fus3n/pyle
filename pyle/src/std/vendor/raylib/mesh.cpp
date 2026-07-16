#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    void register_mesh(NativeModule& mod) {
        VM& vm = mod.get_vm();

        ClassBinder<Mesh> mesh(vm, "Mesh");
        mesh.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<Mesh>(vm, new Mesh{});
        })
            .member<int, &Mesh::vertexCount>("vertexCount")
            .member<int, &Mesh::triangleCount>("triangleCount");
        mod.class_type(mesh);

        mod.raw_function("GenMeshCube", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "GenMeshCube expects (width, height, length)."); return Value(); }
            float w = from_value<float>(vm, args[0]);
            float h = from_value<float>(vm, args[1]);
            float l = from_value<float>(vm, args[2]);
            Mesh m = GenMeshCube(w, h, l);
            return to_value_owned<Mesh>(vm, new Mesh(m));
        });
        
        mod.raw_function("GenMeshPlane", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "GenMeshPlane expects (width, length, resX, resZ)."); return Value(); }
            float w = from_value<float>(vm, args[0]);
            float l = from_value<float>(vm, args[1]);
            int rx = from_value<int64_t>(vm, args[2]);
            int rz = from_value<int64_t>(vm, args[3]);
            Mesh m = GenMeshPlane(w, l, rx, rz);
            return to_value_owned<Mesh>(vm, new Mesh(m));
        });
        
        mod.raw_function("GenMeshSphere", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "GenMeshSphere expects (radius, rings, slices)."); return Value(); }
            float r = from_value<float>(vm, args[0]);
            int rings = from_value<int64_t>(vm, args[1]);
            int slices = from_value<int64_t>(vm, args[2]);
            Mesh m = GenMeshSphere(r, rings, slices);
            return to_value_owned<Mesh>(vm, new Mesh(m));
        });
        
        mod.raw_function("GenMeshCylinder", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "GenMeshCylinder expects (radius, height, slices)."); return Value(); }
            float r = from_value<float>(vm, args[0]);
            float h = from_value<float>(vm, args[1]);
            int sl = from_value<int64_t>(vm, args[2]);
            Mesh m = GenMeshCylinder(r, h, sl);
            return to_value_owned<Mesh>(vm, new Mesh(m));
        });
        
        mod.raw_function("GenMeshTorus", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "GenMeshTorus expects (radius, size, radSeg, sides)."); return Value(); }
            float r = from_value<float>(vm, args[0]);
            float sz = from_value<float>(vm, args[1]);
            int rs = from_value<int64_t>(vm, args[2]);
            int sd = from_value<int64_t>(vm, args[3]);
            Mesh m = GenMeshTorus(r, sz, rs, sd);
            return to_value_owned<Mesh>(vm, new Mesh(m));
        });
        
        mod.raw_function("GenMeshHeightmap", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "GenMeshHeightmap expects (image, size)."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            Vector3* sz = as_native<Vector3>(vm, args[1], "Vector3");
            if (!img || !sz) return Value();
            Mesh m = GenMeshHeightmap(*img, *sz);
            return to_value_owned<Mesh>(vm, new Mesh(m));
        });
        
        mod.raw_function("GenMeshCubicmap", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "GenMeshCubicmap expects (cubicmap, cubeSize)."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            Vector3* sz = as_native<Vector3>(vm, args[1], "Vector3");
            if (!img || !sz) return Value();
            Mesh m = GenMeshCubicmap(*img, *sz);
            return to_value_owned<Mesh>(vm, new Mesh(m));
        });
        
        mod.raw_function("UploadMesh", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "UploadMesh expects (mesh, dynamic)."); return Value(); }
            Mesh* m = as_native<Mesh>(vm, args[0], "Mesh");
            bool dyn = from_value<bool>(vm, args[1]);
            if (!m) return Value();
            UploadMesh(m, dyn);
            return Value();
        });
        
        mod.raw_function("UnloadMesh", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadMesh expects 1 Mesh."); return Value(); }
            Mesh* m = as_native<Mesh>(vm, args[0], "Mesh");
            if (!m) return Value();
            UnloadMesh(*m);
            return Value();
        });
        
        mod.raw_function("DrawMesh", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawMesh expects (mesh, material, transform)."); return Value(); }
            Mesh* m = as_native<Mesh>(vm, args[0], "Mesh");
            Material* mat = as_native<Material>(vm, args[1], "Material");
            Matrix* t = as_native<Matrix>(vm, args[2], "Matrix");
            if (!m || !mat || !t) return Value();
            DrawMesh(*m, *mat, *t);
            return Value();
        });
        
        mod.raw_function("DrawMeshInstanced", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawMeshInstanced expects (mesh, material, transforms)."); return Value(); }
            Mesh* m = as_native<Mesh>(vm, args[0], "Mesh");
            Material* mat = as_native<Material>(vm, args[1], "Material");
            if (!m || !mat) return Value();
            const Value& list_val = args[2];
            if (list_val.tag != Value::Tag::ArrayRef) {
                vm.runtime_error(RuntimeError::Type, "Expected list for transforms.");
                return Value();
            }
            Object& list_obj = vm.get_heap_object(list_val.as_ref);
            auto& items = std::get<ArrayType>(list_obj.data);
            int count = static_cast<int>(items.size());
            Matrix* transforms = new Matrix[count];
            for (int i = 0; i < count; ++i) {
                Matrix* t = as_native<Matrix>(vm, items[i], "Matrix");
                if (t) transforms[i] = *t;
                else transforms[i] = Matrix{ 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
            }
            DrawMeshInstanced(*m, *mat, transforms, count);
            delete[] transforms;
            return Value();
        });
        
        mod.raw_function("GetMeshBoundingBox", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetMeshBoundingBox expects 1 Mesh."); return Value(); }
            Mesh* m = as_native<Mesh>(vm, args[0], "Mesh");
            if (!m) return Value();
            BoundingBox bb = GetMeshBoundingBox(*m);
            return to_value_owned<BoundingBox>(vm, new BoundingBox{ bb.min, bb.max });
        });
        
        mod.raw_function("ExportMesh", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "ExportMesh expects (mesh, fileName)."); return Value(); }
            Mesh* m = as_native<Mesh>(vm, args[0], "Mesh");
            const std::string& name = from_value<std::string>(vm, args[1]);
            if (!m) return Value();
            return to_value(vm, ExportMesh(*m, name.c_str()));
        });
    }

}
}
