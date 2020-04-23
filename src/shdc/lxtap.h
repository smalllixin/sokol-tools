#pragma once

#include <sstream>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace lx { namespace m {

enum class attr_type: std::uint8_t {
    INVALID,
    FLOAT1 = 1,
    FLOAT2 = 2,
    FLOAT3 = 3,
    FLOAT4 = 4,
    MAT4 = 5,
};

using str_size = std::uint16_t;

template <typename T>
inline std::size_t write_binary(std::ostringstream& oss, T p) {
//    static_assert(std::is_fundamental<T>::value, "write_binary only works for primitive type");
    oss.write((char const*)&p, sizeof(T));
    return sizeof(T);
}

struct bytes {
    str_size size;
    std::string str;
    
    bytes(const std::string& s = ""): str(s) {
        size = s.size();
    }
    
    uint16_t struct_size() const {
        return str.size() + sizeof(size);
    }
    
    explicit operator std::string() const {
        return str;
    }
    
    void write(std::ostringstream& oss) {
        write_binary(oss, size);
        oss.write(str.c_str(), str.size());
    }
};


struct attr_t {
    uint16_t slot;
    bytes name;
    attr_type type;
    
    attr_t() = default;
    attr_t(uint16_t slot_, bytes name_, attr_type type_)
    : slot(slot_)
    , name(name_)
    , type(type_) {
    }
    
    uint16_t struct_size() const {
        assert(sizeof(type) == 1);
        return sizeof(slot) + name.struct_size() + sizeof(type);
    }
    
    void write(std::ostringstream& oss) {
        static_assert(sizeof(type) == 1, "enum class alignment issue");
        write_binary(oss, struct_size());
        write_binary(oss, slot);
        name.write(oss);
        write_binary(oss, type);
    }
};

template <typename T>
struct array {
    std::vector<T> arr;
    uint16_t struct_size() const {
        uint16_t s = 0;
        for (auto& t : arr) {
            s += t.struct_size();
        }
        return sizeof(uint16_t) + s;
    }
    
    explicit operator std::vector<T>& () {
        return arr;
    }
    
    void write(std::ostringstream& oss) {
        uint16_t size = arr.size();
        write_binary(oss, size);
        for (auto& a : arr) {
            a.write(oss);
        }
    }
};

struct uniform_t {
    bytes name;
    attr_type type;
    uint8_t array_count;
    uint8_t offset;
    
    uint16_t struct_size() const {
        return name.struct_size() + sizeof(type) + sizeof(array_count) + sizeof(offset);
    }
    
    void write(std::ostringstream& oss) {
        write_binary(oss, struct_size());
        name.write(oss);
        write_binary(oss, type);
        write_binary(oss, array_count);
        write_binary(oss, offset);
    }
};

struct uniform_block_t {
    uint8_t slot;
    bytes name;
    uint8_t size;
    array<uniform_t> params;
    
    uint16_t struct_size() const {
        return sizeof(slot) + name.struct_size() + sizeof(size) + params.struct_size();
    }
    
    void write(std::ostringstream& oss) {
        write_binary(oss, struct_size());
        write_binary(oss, slot);
        name.write(oss);
        write_binary(oss, size);
        params.write(oss);
    }
};

enum class texture_type: std::uint8_t {
  INVALID,
  IMAGE_2D,
  IMAGE_CUBE,
  IMAGE_3D,
  IMAGE_ARRAY
};

struct texture_t {
    uint8_t slot;
    bytes name;
    texture_type type;
    
    uint16_t struct_size() const {
        return sizeof(slot) + name.struct_size() + sizeof(type);
    }
    
    void write(std::ostringstream& oss) {
        write_binary(oss, struct_size());
        write_binary(oss, slot);
        name.write(oss);
        write_binary(oss, type);
        static_assert(sizeof(type) == 1, "enum class should be 1 byte");
    }
};

enum class lang_type: std::uint8_t {
    GLSL330 = 0,
    GLSL100,
    GLSL300ES,
    HLSL5,
    METAL_MACOS,
    METAL_IOS,
    METAL_SIM,
    NUM
};

enum class source_type: std::uint8_t {
    SourceCode = 0,
    ByteCode,
};

struct shader_t {
    source_type src_type;
    bytes source;
    
    uint16_t struct_size() const {
        return sizeof(src_type) + source.struct_size();
    }
    
    void write(std::ostringstream& oss) {
        write_binary(oss, struct_size());
        write_binary(oss, src_type);
        source.write(oss);
    }
};

struct program_t {
    lang_type lang;
    shader_t vs;
    shader_t fs;
    
    uint16_t struct_size() const {
        return sizeof(lang) + vs.struct_size() + fs.struct_size();
    }
    
    void write(std::ostringstream& oss) {
        write_binary(oss, struct_size());
        write_binary(oss, lang);
        vs.write(oss);
        fs.write(oss);
    }
};

struct header_t {
    uint8_t place0 = 0xCF;
    uint8_t place1 = 0xBA;
    uint16_t version = 1;
    
    void write(std::ostringstream& oss) {
        write_binary(oss, place0);
        write_binary(oss, place1);
        write_binary(oss, version);
    }
};

template<typename T>
struct optional {
    T value;
    bool is_empty = true;
    
    void write(std::ostringstream& oss) {
        uint8_t empty = 0x00;
        uint8_t exist = 0x01;
        if (!is_empty) {
            oss.write((char const*)&exist, 1);
            value.write(oss);
        } else {
            oss.write((char const*)&empty, 1);
        }
    }
    
    operator T& () {
        return value;
    }
    
    void fill() {
        is_empty = false;
    }
};

struct WriteContext {
    lx::m::array<lx::m::attr_t> attrs;
    optional<lx::m::uniform_block_t> vs_uniform_block;
    optional<lx::m::uniform_block_t> fs_uniform_block;
    optional<lx::m::array<lx::m::texture_t>> fs_textures;
    lx::m::array<lx::m::program_t> programs;
    
    void write(std::ostringstream &oss) {
        // write header
        // write array<vertex attribute>
        // write vs uniform_block_t
        // write fs uniform_block_t
        // write array<fs texture>
        // write array<programs>
        header_t h;
        h.write(oss);
        attrs.write(oss);
        vs_uniform_block.write(oss);
        fs_uniform_block.write(oss);
        fs_textures.write(oss);
        programs.write(oss);
    }
};

struct Writer {
    static std::string gen(WriteContext& ctx) {
        std::ostringstream oss;
        ctx.write(oss);
        return oss.str();
    }
};

} // end namespace m

} // end namespace lxtap
