/*
    Generate the output header for sokol_gfx.h
*/
#include "lxtap.h"
#include "shdc.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>
#include <cstdint>


namespace shdc {

static lx::m::WriteContext mat_ctx;
static std::string file_content;

#if defined(_MSC_VER)
#define L(str, ...) file_content.append(fmt::format(str, __VA_ARGS__))
#else
#define L(str, ...) file_content.append(fmt::format(str, ##__VA_ARGS__))
#endif

#pragma mark - LX custom

static void lxtap_write_header(const args_t& args, const input_t& inp, const spirvcross_t& spirvcross) {
    for (const auto& item: inp.programs) {
        const program_t& prog = item.second;

        int vs_snippet_index = inp.snippet_map.at(prog.vs_name);
        int fs_snippet_index = inp.snippet_map.at(prog.fs_name);
        int vs_src_index = spirvcross.find_source_by_snippet_index(vs_snippet_index);
        int fs_src_index = spirvcross.find_source_by_snippet_index(fs_snippet_index);
        assert((vs_src_index >= 0) && (fs_src_index >= 0));
        const spirvcross_source_t& vs_src = spirvcross.sources[vs_src_index];
        const spirvcross_source_t& fs_src = spirvcross.sources[fs_src_index];
        
        
            
        fmt::print("[LX] program:{} vs:{} fs:{}\n", prog.name, prog.vs_name, prog.fs_name);
        // attribute name and type
        for (const attr_t& attr: vs_src.refl.inputs) {
            if (attr.slot >= 0) {
                lx::m::attr_t mattr;
                mattr.slot = attr.slot;
                mattr.name = attr.name;
                mattr.type = (lx::m::attr_type)attr.type;
                mat_ctx.attrs.arr.push_back(mattr);
                fmt::print("[LX] Attrib Name {} {}\n", attr.name, attr_t::type_to_str(attr.type));
            }
        }
        // uniform block
        // vs uniforms
        for (const uniform_block_t& ub: vs_src.refl.uniform_blocks) {
            lx::m::uniform_block_t& muniform_block = mat_ctx.vs_uniform_block.value;
            mat_ctx.vs_uniform_block.fill();
            muniform_block.name = ub.name;
            muniform_block.slot = ub.slot;
            muniform_block.size = ub.size;
            fmt::print("[LX] VS Uniform: {} {}\n", ub.name, ub.slot);
            for (const uniform_t& uniform: ub.uniforms) {
                lx::m::uniform_t mu;
                mu.name = uniform.name;
                mu.type = (lx::m::attr_type)uniform.type;
                mu.offset = uniform.offset;
                mu.array_count = uniform.array_count;
                muniform_block.params.arr.push_back(mu);
                fmt::print("[LX] uniform: {}\t\t {}\t\t {}\n", uniform.name, uniform_t::type_to_str(uniform.type), uniform.offset);
            }
        }
        // fs uniforms
        for (const uniform_block_t& ub: fs_src.refl.uniform_blocks) {
            lx::m::uniform_block_t& muniform_block = mat_ctx.fs_uniform_block.value;
            mat_ctx.vs_uniform_block.fill();
            muniform_block.name = ub.name;
            muniform_block.slot = ub.slot;
            muniform_block.size = ub.size;
            fmt::print("[LX] FS Uniform: name_{} slot_{} size_{}:\n", ub.name, ub.slot, ub.size);
            for (const uniform_t& uniform: ub.uniforms) {
                lx::m::uniform_t mu;
                mu.name = uniform.name;
                mu.type = (lx::m::attr_type)uniform.type;
                mu.offset = uniform.offset;
                mu.array_count = uniform.array_count;
                muniform_block.params.arr.push_back(mu);
                fmt::print("[LX] uniform: {}\t\t {}\t\t {}\n", uniform.name, uniform_t::type_to_str(uniform.type), uniform.offset);
            }
        }
        
        fmt::print("[LX] images:\n");
        for (const image_t& img: fs_src.refl.images) {
            mat_ctx.fs_textures.fill();
            lx::m::texture_t mt;
            mt.name = img.name;
            mt.slot = img.slot;
            mt.type = (lx::m::texture_type)img.type;
            mat_ctx.fs_textures.value.arr.push_back(mt);
            fmt::print("[LX] {} {} {}\n", img.name, img.slot, image_t::type_to_str(img.type));
        }
    }
    
    // lx add end
}

static void lxtap_write_shader_sources_and_blobs(const input_t& inp,
                                           const spirvcross_t& spirvcross,
                                           const bytecode_t& bytecode,
                                           slang_t::type_t slang) {
    
    // Algorithm
    // program: lang, vs_shader, fs_shader
    // if not Find progs has this lang:
    //  create one program
    // prog add vs or fs shader
    auto& mprogs = static_cast<std::vector<lx::m::program_t>&>(mat_ctx.programs);
    for (int snippet_index = 0; snippet_index < (int)inp.snippets.size(); snippet_index++) {
        const snippet_t& snippet = inp.snippets[snippet_index];
        if ((snippet.type != snippet_t::VS) && (snippet.type != snippet_t::FS)) {
            continue;
        }
        int src_index = spirvcross.find_source_by_snippet_index(snippet_index);
        assert(src_index >= 0);
        const spirvcross_source_t& src = spirvcross.sources[src_index];
        int blob_index = bytecode.find_blob_by_snippet_index(snippet_index);
        const bytecode_blob_t* blob = 0;
        if (blob_index != -1) {
            blob = &bytecode.blobs[blob_index];
        }
        
        lx::m::program_t *current_p = nullptr;
        for (lx::m::program_t& p: mprogs) {
            if ((int)p.lang == (int)slang) {
                current_p = &p;
                break;
            }
        }
        if (current_p == nullptr) {
            // create new progam
            lx::m::program_t prog;
            prog.lang = (lx::m::lang_type)slang;
            mprogs.push_back(prog);
            current_p = &mprogs[mprogs.size() - 1];
        }
        
        lx::m::shader_t& ms = current_p->vs;
        fmt::print("[LX] lang:{} \n", slang_t::to_str(slang));
        if (snippet.type == snippet_t::VS) {
            ms = current_p->vs;
            fmt::print("[LX] VS Shader\n");
        } else if (snippet.type == snippet_t::FS) {
            ms = current_p->fs;
            fmt::print("[LX] FS Shader\n");
        }
    
        if (blob) {
            ms.src_type = lx::m::source_type::ByteCode;
            ms.source = std::string((const char *)(blob->data.data()), blob->data.size());
        }
        else {
            ms.src_type = lx::m::source_type::SourceCode;
            ms.source = src.source_code;
            
            /* if no bytecode exists, write the source code, but also a a byte array with a trailing 0 */
            std::vector<std::string> code_lines;
            pystring::splitlines(src.source_code, code_lines);
            fmt::print("bytes:{}\n", src.source_code.size());
        }
    }
}

static void lxtap_write_end() {
    file_content = lx::m::Writer::gen(mat_ctx);
}

errmsg_t lxtap_t::gen(const args_t& args, const input_t& inp,
                     const std::array<spirvcross_t,slang_t::NUM>& spirvcross,
                     const std::array<bytecode_t,slang_t::NUM>& bytecode)
{
    // first write everything into a string, and only when no errors occur,
    // dump this into a file (so we don't have half-written files lying around)
    file_content.clear();

    errmsg_t err;
    bool comment_header_written = false;
    for (int i = 0; i < slang_t::NUM; i++) {
        slang_t::type_t slang = (slang_t::type_t) i;
        if (args.slang & slang_t::bit(slang)) {
            errmsg_t err = output_t::check_errors(inp, spirvcross[i], slang);
            if (err.valid) {
                return err;
            }
            
            if (!comment_header_written) {
                if (inp.programs.size() != 1) {
                    fmt::print(stderr, "[lxtap] Material could only contain one program. Now is {}\n", inp.programs.size());
                    exit(1);
                }
                lxtap_write_header(args, inp, spirvcross[i]);
                comment_header_written = true;
            }
            lxtap_write_shader_sources_and_blobs(inp, spirvcross[i], bytecode[i], slang);
        }
    }
    
    lxtap_write_end();

    // write result into output file
    FILE* f = fopen(args.output.c_str(), "w");
    if (!f) {
        return errmsg_t::error(inp.base_path, 0, fmt::format("failed to open output file '{}'", args.output));
    }
    fwrite(file_content.c_str(), file_content.length(), 1, f);
    fclose(f);
    return errmsg_t();
}

} // namespace shdc
