#include "pyle/binder.hpp"
#include <mbedtls/base64.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/md.h>
#include <mbedtls/md5.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <cstdint>
#include <string>
#include <vector>

namespace pyle {

    namespace {
        using Bytes = std::vector<uint8_t>;

        bool take_bytes(VM& vm, const Value& v, std::vector<uint8_t>& out) {
            if (v.tag == Value::Tag::StringRef && vm.heap_valid(v.as_ref)) {
                const std::string& s = std::get<std::string>(vm.get_heap_object(v.as_ref).data);
                out.assign(s.begin(), s.end());
                return true;
            }
            if (v.tag == Value::Tag::BytesRef && vm.heap_valid(v.as_ref)) {
                out = std::get<BytesType>(vm.get_heap_object(v.as_ref).data);
                return true;
            }
            return false;
        }

        Value bytes_result(VM& vm, Bytes&& bytes) {
            HeapIdx idx = vm.alloc(Object(std::move(bytes)));
            return Value(Value::Tag::BytesRef, idx);
        }

        std::string to_hex(const uint8_t* data, size_t len) {
            static const char* digits = "0123456789abcdef";
            std::string out;
            out.resize(len * 2);
            for (size_t i = 0; i < len; ++i) {
                out[i * 2] = digits[(data[i] >> 4) & 0xF];
                out[i * 2 + 1] = digits[data[i] & 0xF];
            }
            return out;
        }
    }

    Value build_crypto_module(VM& vm) {
        NativeModule mod(vm, "crypto");

        mod.raw_function("sha256", [](VM& m, ArgView args) -> Value {
            if (args.size() != 1) {
                m.runtime_error(RuntimeError::ArgumentError, "sha256 expects (data).");
                return Value();
            }
            std::vector<uint8_t> data;
            if (!take_bytes(m, args[0], data)) {
                m.runtime_error(RuntimeError::ArgumentError, "sha256 expects a string or bytes.");
                return Value();
            }
            Bytes out(32);
            if (mbedtls_sha256(data.data(), data.size(), out.data(), 0) != 0) {
                m.runtime_error(RuntimeError::Runtime, "sha256 failed.");
                return Value();
            }
            return bytes_result(m, std::move(out));
        });

        mod.raw_function("sha256_hex", [](VM& m, ArgView args) -> Value {
            if (args.size() != 1) {
                m.runtime_error(RuntimeError::ArgumentError, "sha256_hex expects (data).");
                return Value();
            }
            std::vector<uint8_t> data;
            if (!take_bytes(m, args[0], data)) {
                m.runtime_error(RuntimeError::ArgumentError, "sha256_hex expects a string or bytes.");
                return Value();
            }
            uint8_t out[32];
            if (mbedtls_sha256(data.data(), data.size(), out, 0) != 0) {
                m.runtime_error(RuntimeError::Runtime, "sha256_hex failed.");
                return Value();
            }
            return to_transient_string(m, to_hex(out, sizeof(out)));
        });

        mod.raw_function("sha1", [](VM& m, ArgView args) -> Value {
            if (args.size() != 1) {
                m.runtime_error(RuntimeError::ArgumentError, "sha1 expects (data).");
                return Value();
            }
            std::vector<uint8_t> data;
            if (!take_bytes(m, args[0], data)) {
                m.runtime_error(RuntimeError::ArgumentError, "sha1 expects a string or bytes.");
                return Value();
            }
            Bytes out(20);
            if (mbedtls_sha1(data.data(), data.size(), out.data()) != 0) {
                m.runtime_error(RuntimeError::Runtime, "sha1 failed.");
                return Value();
            }
            return bytes_result(m, std::move(out));
        });

        mod.raw_function("sha1_hex", [](VM& m, ArgView args) -> Value {
            if (args.size() != 1) {
                m.runtime_error(RuntimeError::ArgumentError, "sha1_hex expects (data).");
                return Value();
            }
            std::vector<uint8_t> data;
            if (!take_bytes(m, args[0], data)) {
                m.runtime_error(RuntimeError::ArgumentError, "sha1_hex expects a string or bytes.");
                return Value();
            }
            uint8_t out[20];
            if (mbedtls_sha1(data.data(), data.size(), out) != 0) {
                m.runtime_error(RuntimeError::Runtime, "sha1_hex failed.");
                return Value();
            }
            return to_transient_string(m, to_hex(out, sizeof(out)));
        });

        mod.raw_function("md5", [](VM& m, ArgView args) -> Value {
            if (args.size() != 1) {
                m.runtime_error(RuntimeError::ArgumentError, "md5 expects (data).");
                return Value();
            }
            std::vector<uint8_t> data;
            if (!take_bytes(m, args[0], data)) {
                m.runtime_error(RuntimeError::ArgumentError, "md5 expects a string or bytes.");
                return Value();
            }
            Bytes out(16);
            if (mbedtls_md5(data.data(), data.size(), out.data()) != 0) {
                m.runtime_error(RuntimeError::Runtime, "md5 failed.");
                return Value();
            }
            return bytes_result(m, std::move(out));
        });

        mod.raw_function("md5_hex", [](VM& m, ArgView args) -> Value {
            if (args.size() != 1) {
                m.runtime_error(RuntimeError::ArgumentError, "md5_hex expects (data).");
                return Value();
            }
            std::vector<uint8_t> data;
            if (!take_bytes(m, args[0], data)) {
                m.runtime_error(RuntimeError::ArgumentError, "md5_hex expects a string or bytes.");
                return Value();
            }
            uint8_t out[16];
            if (mbedtls_md5(data.data(), data.size(), out) != 0) {
                m.runtime_error(RuntimeError::Runtime, "md5_hex failed.");
                return Value();
            }
            return to_transient_string(m, to_hex(out, sizeof(out)));
        });

        mod.raw_function("hmac_sha256", [](VM& m, ArgView args) -> Value {
            if (args.size() != 2) {
                m.runtime_error(RuntimeError::ArgumentError, "hmac_sha256 expects (key, data).");
                return Value();
            }
            std::vector<uint8_t> key, data;
            if (!take_bytes(m, args[0], key) || !take_bytes(m, args[1], data)) {
                m.runtime_error(RuntimeError::ArgumentError, "hmac_sha256 expects a string or bytes key and data.");
                return Value();
            }
            const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
            if (!info) {
                m.runtime_error(RuntimeError::Runtime, "hmac_sha256 unavailable.");
                return Value();
            }
            Bytes out(32);
            if (mbedtls_md_hmac(info, key.data(), key.size(), data.data(), data.size(), out.data()) != 0) {
                m.runtime_error(RuntimeError::Runtime, "hmac_sha256 failed.");
                return Value();
            }
            return bytes_result(m, std::move(out));
        });

        mod.raw_function("hmac_sha256_hex", [](VM& m, ArgView args) -> Value {
            if (args.size() != 2) {
                m.runtime_error(RuntimeError::ArgumentError, "hmac_sha256_hex expects (key, data).");
                return Value();
            }
            std::vector<uint8_t> key, data;
            if (!take_bytes(m, args[0], key) || !take_bytes(m, args[1], data)) {
                m.runtime_error(RuntimeError::ArgumentError, "hmac_sha256_hex expects a string or bytes key and data.");
                return Value();
            }
            const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
            if (!info) {
                m.runtime_error(RuntimeError::Runtime, "hmac_sha256_hex unavailable.");
                return Value();
            }
            uint8_t out[32];
            if (mbedtls_md_hmac(info, key.data(), key.size(), data.data(), data.size(), out) != 0) {
                m.runtime_error(RuntimeError::Runtime, "hmac_sha256_hex failed.");
                return Value();
            }
            return to_transient_string(m, to_hex(out, sizeof(out)));
        });

        mod.raw_function("base64_encode", [](VM& m, ArgView args) -> Value {
            if (args.size() != 1) {
                m.runtime_error(RuntimeError::ArgumentError, "base64_encode expects (data).");
                return Value();
            }
            std::vector<uint8_t> data;
            if (!take_bytes(m, args[0], data)) {
                m.runtime_error(RuntimeError::ArgumentError, "base64_encode expects a string or bytes.");
                return Value();
            }
            size_t need = 4 * ((data.size() + 2) / 3) + 1;
            std::string out(need, '\0');
            size_t olen = 0;
            if (mbedtls_base64_encode(reinterpret_cast<unsigned char*>(out.data()), need, &olen,
                                       data.data(), data.size()) != 0) {
                m.runtime_error(RuntimeError::Runtime, "base64_encode failed.");
                return Value();
            }
            out.resize(olen);
            return to_transient_string(m, out);
        });

        mod.raw_function("base64_decode", [](VM& m, ArgView args) -> Value {
            if (args.size() != 1 || args[0].tag != Value::Tag::StringRef || !m.heap_valid(args[0].as_ref)) {
                m.runtime_error(RuntimeError::ArgumentError, "base64_decode expects (string).");
                return Value();
            }
            const std::string& s = std::get<std::string>(m.get_heap_object(args[0].as_ref).data);
            Bytes out(3 * (s.size() / 4) + 4);
            size_t olen = 0;
            if (mbedtls_base64_decode(out.data(), out.size(), &olen,
                                       reinterpret_cast<const unsigned char*>(s.data()), s.size()) != 0) {
                m.runtime_error(RuntimeError::ArgumentError, "base64_decode got invalid input.");
                return Value();
            }
            out.resize(olen);
            return bytes_result(m, std::move(out));
        });

        mod.raw_function("base64url_encode", [](VM& m, ArgView args) -> Value {
            if (args.size() != 1) {
                m.runtime_error(RuntimeError::ArgumentError, "base64url_encode expects (data).");
                return Value();
            }
            std::vector<uint8_t> data;
            if (!take_bytes(m, args[0], data)) {
                m.runtime_error(RuntimeError::ArgumentError, "base64url_encode expects a string or bytes.");
                return Value();
            }
            size_t need = 4 * ((data.size() + 2) / 3) + 1;
            std::string out(need, '\0');
            size_t olen = 0;
            if (mbedtls_base64_encode(reinterpret_cast<unsigned char*>(out.data()), need, &olen,
                                       data.data(), data.size()) != 0) {
                m.runtime_error(RuntimeError::Runtime, "base64url_encode failed.");
                return Value();
            }
            out.resize(olen);
            for (char& c : out) {
                if (c == '+') c = '-';
                else if (c == '/') c = '_';
            }
            while (!out.empty() && out.back() == '=') out.pop_back();
            return to_transient_string(m, out);
        });

        mod.raw_function("base64url_decode", [](VM& m, ArgView args) -> Value {
            if (args.size() != 1 || args[0].tag != Value::Tag::StringRef || !m.heap_valid(args[0].as_ref)) {
                m.runtime_error(RuntimeError::ArgumentError, "base64url_decode expects (string).");
                return Value();
            }
            std::string s = std::get<std::string>(m.get_heap_object(args[0].as_ref).data);
            for (char& c : s) {
                if (c == '-') c = '+';
                else if (c == '_') c = '/';
            }
            while (s.size() % 4 != 0) s.push_back('=');
            Bytes out(3 * (s.size() / 4) + 4);
            size_t olen = 0;
            if (mbedtls_base64_decode(out.data(), out.size(), &olen,
                                       reinterpret_cast<const unsigned char*>(s.data()), s.size()) != 0) {
                m.runtime_error(RuntimeError::ArgumentError, "base64url_decode got invalid input.");
                return Value();
            }
            out.resize(olen);
            return bytes_result(m, std::move(out));
        });

        mod.raw_function("random", [](VM& m, ArgView args) -> Value {
            if (args.size() != 1 || args[0].tag != Value::Tag::Int) {
                m.runtime_error(RuntimeError::ArgumentError, "random expects (byte_count: int).");
                return Value();
            }
            int64_t n = args[0].as_int;
            if (n < 0 || n > 65536) {
                m.runtime_error(RuntimeError::ArgumentError, "random byte_count out of range.");
                return Value();
            }
            static mbedtls_entropy_context entropy;
            static mbedtls_ctr_drbg_context drbg;
            static bool seeded = false;
            if (!seeded) {
                mbedtls_entropy_init(&entropy);
                mbedtls_ctr_drbg_init(&drbg);
                static const char pers[] = "pyle_crypto";
                if (mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy,
                                           reinterpret_cast<const unsigned char*>(pers), sizeof(pers) - 1) != 0) {
                    m.runtime_error(RuntimeError::Runtime, "random failed to seed.");
                    return Value();
                }
                seeded = true;
            }
            Bytes out(static_cast<size_t>(n));
            if (n > 0 && mbedtls_ctr_drbg_random(&drbg, out.data(), out.size()) != 0) {
                m.runtime_error(RuntimeError::Runtime, "random failed.");
                return Value();
            }
            return bytes_result(m, std::move(out));
        });

        mod.raw_function("to_hex", [](VM& m, ArgView args) -> Value {
            if (args.size() != 1) {
                m.runtime_error(RuntimeError::ArgumentError, "to_hex expects (data).");
                return Value();
            }
            std::vector<uint8_t> data;
            if (!take_bytes(m, args[0], data)) {
                m.runtime_error(RuntimeError::ArgumentError, "to_hex expects a string or bytes.");
                return Value();
            }
            return to_transient_string(m, to_hex(data.data(), data.size()));
        });

        mod.raw_function("pbkdf2", [](VM& m, ArgView args) -> Value {
            if (args.size() != 4) {
                m.runtime_error(RuntimeError::ArgumentError, "pbkdf2 expects (pass, salt, iterations, key_len).");
                return Value();
            }
            std::vector<uint8_t> pass, salt;
            if (!take_bytes(m, args[0], pass) || !take_bytes(m, args[1], salt)) {
                m.runtime_error(RuntimeError::ArgumentError, "pbkdf2 pass and salt must be a string or bytes.");
                return Value();
            }
            if (args[2].tag != Value::Tag::Int || args[3].tag != Value::Tag::Int) {
                m.runtime_error(RuntimeError::ArgumentError, "pbkdf2 iterations and key_len must be ints.");
                return Value();
            }
            int64_t iters = args[2].as_int;
            int64_t key_len = args[3].as_int;
            if (iters < 1 || iters > 10000000 || key_len < 1 || key_len > 1024) {
                m.runtime_error(RuntimeError::ArgumentError, "pbkdf2 iterations or key_len out of range.");
                return Value();
            }
            Bytes out(static_cast<size_t>(key_len));
            if (mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,
                                              pass.data(), pass.size(), salt.data(), salt.size(),
                                              static_cast<unsigned int>(iters),
                                              static_cast<uint32_t>(key_len), out.data()) != 0) {
                m.runtime_error(RuntimeError::Runtime, "pbkdf2 failed.");
                return Value();
            }
            return bytes_result(m, std::move(out));
        });

        return mod.build();
    }

}

pyle::Value register_crypto_module(pyle::VM& vm) {
    return pyle::build_crypto_module(vm);
}
