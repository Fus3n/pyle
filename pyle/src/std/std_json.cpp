#include <simdjson.h>
#include <string>
#include <string_view>
#include <charconv>
#include <cstdio>
#include "pyle/binder.hpp"

using json_element = simdjson::dom::element;

pyle::Value simdjson_to_pyle(pyle::VM& vm, json_element element) {
    switch (element.type()) {
        case simdjson::dom::element_type::NULL_VALUE:
            return pyle::Value();
            
        case simdjson::dom::element_type::BOOL:
            return pyle::Value(bool(element));
            
        case simdjson::dom::element_type::INT64:
            return pyle::Value(int64_t(element));
            
        case simdjson::dom::element_type::UINT64:
            return pyle::Value(static_cast<int64_t>(uint64_t(element)));
            
        case simdjson::dom::element_type::DOUBLE:
            return pyle::Value(double(element));
            
        case simdjson::dom::element_type::STRING: {
            std::string_view sv = element.get_string();
            return pyle::to_value(vm, sv);
        }
        
        case simdjson::dom::element_type::ARRAY: {
            pyle::ArrayType arr;
            simdjson::dom::array ja = element.get_array();
            arr.reserve(ja.size());
            for (auto item : ja) {
                arr.push_back(simdjson_to_pyle(vm, item));
            }
            return pyle::Value(pyle::Value::Tag::ArrayRef, vm.alloc(pyle::Object(std::move(arr))));
        }
        
        case simdjson::dom::element_type::OBJECT: {
            pyle::MapType map;
            simdjson::dom::object jo = element.get_object();
            for (auto field : jo) {
                pyle::Value key = pyle::to_value(vm, std::string_view(field.key));
                map[key] = simdjson_to_pyle(vm, field.value);
            }
            return pyle::Value(pyle::Value::Tag::MapRef, vm.alloc(pyle::Object(std::move(map))));
        }
        
        default:
            return pyle::Value();
    }
}

pyle::Value native_json_parse(pyle::VM& vm, pyle::ArgView args) {
    if (args.size() != 1 || args[0].tag != pyle::Value::Tag::StringRef) {
        vm.runtime_error(pyle::RuntimeError::ArgumentError, "json.parse expects 1 string argument.");
        return pyle::Value();
    }

    const std::string& raw_str = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
    thread_local simdjson::dom::parser parser;
    json_element doc;

    auto error = parser.parse(raw_str).get(doc);

    if (error) {
        vm.runtime_error(pyle::RuntimeError::Runtime, std::string("simdjson Parse error: ") + simdjson::error_message(error));
        return pyle::Value();
    }

    bool was_enabled = vm.is_gc_enabled();
    vm.set_gc_enabled(false);
    pyle::Value result = simdjson_to_pyle(vm, doc);
    vm.set_gc_enabled(was_enabled);

    return result;
}

void escape_json_string(const std::string& input, std::string& out) {
    out += "\"";
    for (char c : input) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<int>(c));
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    out += "\"";
}

bool pyle_to_json_string(pyle::VM& vm, const pyle::Value& val, std::string& out) {
    switch (val.tag) {
        case pyle::Value::Tag::None:
            out += "null";
            return true;
            
        case pyle::Value::Tag::Bool:
            out += val.as_bool ? "true" : "false";
            return true;
            
        case pyle::Value::Tag::Int: {
            char buf[24];
            auto res = std::to_chars(buf, buf + sizeof(buf), val.as_int);
            out.append(buf, res.ptr);
            return true;
        }

        case pyle::Value::Tag::Float: {
            char buf[32];
            auto res = std::to_chars(buf, buf + sizeof(buf), val.as_float);
            out.append(buf, res.ptr);
            return true;
        }
            
        case pyle::Value::Tag::StringRef: {
            escape_json_string(std::get<std::string>(vm.get_heap_object(val.as_ref).data), out);
            return true;
        }
        
        case pyle::Value::Tag::ArrayRef: {
            out += "[";
            const auto& vec = std::get<pyle::ArrayType>(vm.get_heap_object(val.as_ref).data);
            for (size_t i = 0; i < vec.size(); ++i) {
                if (!pyle_to_json_string(vm, vec[i], out)) {
                    return false; 
                }
                if (i < vec.size() - 1) out += ",";
            }
            out += "]";
            return true;
        }
        
        case pyle::Value::Tag::MapRef: {
            out += "{";
            const auto& map = std::get<pyle::MapType>(vm.get_heap_object(val.as_ref).data);
            size_t i = 0;
            for (const auto& [k, v] : map) {
                escape_json_string(vm.value_to_string(k), out);
                out += ":";
                if (!pyle_to_json_string(vm, v, out)) {
                    return false; 
                }
                if (++i < map.size()) out += ",";
            }
            out += "}";
            return true;
        }
        
        default:
            vm.runtime_error(pyle::RuntimeError::Type, 
                "Cannot serialize type '" + val.tag_to_string() + "' to JSON.");
            return false;
    }
}

pyle::Value native_json_stringify(pyle::VM& vm, pyle::ArgView args) {
    if (args.size() != 1) {
        vm.runtime_error(pyle::RuntimeError::ArgumentError, "json.stringify expects exactly 1 argument.");
        return pyle::Value();
    }

    std::string out;
    if (!pyle_to_json_string(vm, args[0], out)) {
        return pyle::Value(); 
    }
    
    return pyle::to_value(vm, out);
}


pyle::Value register_json_module(pyle::VM& vm) {
    return pyle::NativeModule(vm, "json")
        .raw_function("parse", native_json_parse)
        .raw_function("stringify", native_json_stringify)
        .build();
}