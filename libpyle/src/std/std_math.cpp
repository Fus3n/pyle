#include "pyle/std/std_core_modules.hpp"
#include "pyle/binder.hpp"
#include "pyle/value.hpp"
#include <cmath>
#include <algorithm>

namespace pyle {

    // ---- single-arg: number -> float ----
    static double math_sqrt(double x)   { return std::sqrt(x); }
    static double math_cbrt(double x)   { return std::cbrt(x); }
    static double math_exp(double x)    { return std::exp(x); }
    static double math_log(double x)    { return std::log(x); }
    static double math_log2(double x)   { return std::log2(x); }
    static double math_log10(double x)  { return std::log10(x); }
    static double math_sin(double x)    { return std::sin(x); }
    static double math_cos(double x)    { return std::cos(x); }
    static double math_tan(double x)    { return std::tan(x); }
    static double math_asin(double x)   { return std::asin(x); }
    static double math_acos(double x)   { return std::acos(x); }
    static double math_atan(double x)   { return std::atan(x); }
    static double math_sinh(double x)   { return std::sinh(x); }
    static double math_cosh(double x)   { return std::cosh(x); }
    static double math_tanh(double x)   { return std::tanh(x); }
    static double math_radians(double x){ return x * 0.017453292519943295; }
    static double math_degrees(double x){ return x * 57.29577951308232; }

    // ---- rounding: number -> int ----
    static int64_t math_floor(double x) { return static_cast<int64_t>(std::floor(x)); }
    static int64_t math_ceil(double x)  { return static_cast<int64_t>(std::ceil(x)); }
    static int64_t math_round(double x) { return static_cast<int64_t>(std::llround(x)); }
    static int64_t math_trunc(double x) { return static_cast<int64_t>(std::trunc(x)); }
    static int64_t math_sign(double x)  { return (x > 0.0) ? 1 : (x < 0.0) ? -1 : 0; }

    // ---- two-arg: (number, number) -> float ----
    static double math_pow(double b, double e)  { return std::pow(b, e); }
    static double math_atan2(double y, double x){ return std::atan2(y, x); }
    static double math_hypot(double x, double y){ return std::hypot(x, y); }

    // ---- type-aware: preserve int when all inputs are int ----
    static Value math_abs(VM& vm, ArgView args) {
        if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "math.abs expects 1 argument."); return Value(); }
        const Value& a = args[0];
        if (a.tag == Value::Tag::Int) return Value(std::abs(a.as_int));
        double x = (a.tag == Value::Tag::Int) ? static_cast<double>(a.as_int) : a.as_float;
        return Value(std::fabs(x));
    }

    static Value math_min(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "math.min expects 2 arguments."); return Value(); }
        const Value& a = args[0]; const Value& b = args[1];
        if (a.tag == Value::Tag::Int && b.tag == Value::Tag::Int) return Value(std::min(a.as_int, b.as_int));
        double da = (a.tag == Value::Tag::Int) ? static_cast<double>(a.as_int) : a.as_float;
        double db = (b.tag == Value::Tag::Int) ? static_cast<double>(b.as_int) : b.as_float;
        return Value(std::min(da, db));
    }

    static Value math_max(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "math.max expects 2 arguments."); return Value(); }
        const Value& a = args[0]; const Value& b = args[1];
        if (a.tag == Value::Tag::Int && b.tag == Value::Tag::Int) return Value(std::max(a.as_int, b.as_int));
        double da = (a.tag == Value::Tag::Int) ? static_cast<double>(a.as_int) : a.as_float;
        double db = (b.tag == Value::Tag::Int) ? static_cast<double>(b.as_int) : b.as_float;
        return Value(std::max(da, db));
    }

    static Value math_clamp(VM& vm, ArgView args) {
        if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "math.clamp expects 3 arguments."); return Value(); }
        const Value& v = args[0]; const Value& lo = args[1]; const Value& hi = args[2];
        if (v.tag == Value::Tag::Int && lo.tag == Value::Tag::Int && hi.tag == Value::Tag::Int) {
            int64_t x = v.as_int, a = lo.as_int, b = hi.as_int;
            return Value(x < a ? a : (x > b ? b : x));
        }
        double x = (v.tag == Value::Tag::Int) ? static_cast<double>(v.as_int) : v.as_float;
        double a = (lo.tag == Value::Tag::Int) ? static_cast<double>(lo.as_int) : lo.as_float;
        double b = (hi.tag == Value::Tag::Int) ? static_cast<double>(hi.as_int) : hi.as_float;
        return Value(x < a ? a : (x > b ? b : x));
    }

    Value math_module_factory(VM& vm) {
        NativeModule m(vm, "math");
        m.constant("PI",   3.141592653589793)
         .constant("TAU",  6.283185307179586)
         .constant("E",    2.718281828459045)
         .function<math_sqrt>("sqrt")
         .function<math_cbrt>("cbrt")
         .function<math_exp>("exp")
         .function<math_log>("log")
         .function<math_log2>("log2")
         .function<math_log10>("log10")
         .function<math_sin>("sin")
         .function<math_cos>("cos")
         .function<math_tan>("tan")
         .function<math_asin>("asin")
         .function<math_acos>("acos")
         .function<math_atan>("atan")
         .function<math_sinh>("sinh")
         .function<math_cosh>("cosh")
         .function<math_tanh>("tanh")
         .function<math_radians>("radians")
         .function<math_degrees>("degrees")
         .function<math_floor>("floor")
         .function<math_ceil>("ceil")
         .function<math_round>("round")
         .function<math_trunc>("trunc")
         .function<math_sign>("sign")
         .function<math_pow>("pow")
         .function<math_atan2>("atan2")
         .function<math_hypot>("hypot")
         .raw_function("abs",   math_abs)
         .raw_function("min",   math_min)
         .raw_function("max",   math_max)
         .raw_function("clamp", math_clamp);
        return m.build();
    }

}
