// ============================================================
//  Chrono1940_Texture.hpp
//  Chargement d'un fichier .dds -> reshade::api::resource + resource_view
//  Utilise exclusivement l'API abstraite ReShade (reshade::api::*)
//  Aucun appel D3D11/D3D12/Vulkan direct.
// ============================================================

#pragma once

#include <reshade.hpp>   // reshade::api::device, resource, resource_view...

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>

#include "addon_logs.h"

namespace LoadDDS
{

// ----------------------------------------------------------
//  Structures DDS (header only, pas de dépendance externe)
// ----------------------------------------------------------
namespace DDS
{
    static constexpr uint32_t MAGIC        = 0x20534444; // "DDS "
    static constexpr uint32_t DDPF_FOURCC  = 0x00000004;
    static constexpr uint32_t DDPF_RGB     = 0x00000040;
    static constexpr uint32_t DDPF_ALPHA   = 0x00000002;
    static constexpr uint32_t DDPF_ALPHAPX = 0x00000001;

#pragma pack(push, 1)
    struct PixelFormat {
        uint32_t dwSize, dwFlags, dwFourCC;
        uint32_t dwRGBBitCount;
        uint32_t dwRBitMask, dwGBitMask, dwBBitMask, dwABitMask;
    };
    struct Header {
        uint32_t    dwSize, dwFlags, dwHeight, dwWidth;
        uint32_t    dwPitchOrLinearSize, dwDepth, dwMipMapCount;
        uint32_t    dwReserved1[11];
        PixelFormat ddspf;
        uint32_t    dwCaps, dwCaps2, dwCaps3, dwCaps4, dwReserved2;
    };
    struct HeaderDXT10 {
        uint32_t dxgiFormat;        // DXGI_FORMAT (uint32)
        uint32_t resourceDimension;
        uint32_t miscFlag;
        uint32_t arraySize;
        uint32_t miscFlags2;
    };
#pragma pack(pop)

    inline uint32_t MakeCC(char a, char b, char c, char d) {
        return uint32_t(a)|(uint32_t(b)<<8)|(uint32_t(c)<<16)|(uint32_t(d)<<24);
    }

    // Mappe DDS PixelFormat -> reshade::api::format
    // reshade::api::format reprend les valeurs DXGI_FORMAT
    inline reshade::api::format GetFormat(const PixelFormat& pf)
    {
        using F = reshade::api::format;

        if (pf.dwFlags & DDPF_FOURCC)
        {
            const uint32_t fc = pf.dwFourCC;
            if (fc == MakeCC('D','X','T','1')) return F::bc1_unorm;
            if (fc == MakeCC('D','X','T','2')) return F::bc2_unorm;
            if (fc == MakeCC('D','X','T','3')) return F::bc2_unorm;
            if (fc == MakeCC('D','X','T','4')) return F::bc3_unorm;
            if (fc == MakeCC('D','X','T','5')) return F::bc3_unorm;
            if (fc == MakeCC('B','C','4','U')) return F::bc4_unorm;
            if (fc == MakeCC('B','C','4','S')) return F::bc4_snorm;
            if (fc == MakeCC('A','T','I','2')) return F::bc5_unorm;
            if (fc == MakeCC('B','C','5','S')) return F::bc5_snorm;
            if (fc == 113)                     return F::r16g16b16a16_float;
            if (fc == 116)                     return F::r32g32b32a32_float;
            if (fc == 111)                     return F::r16_float;
            if (fc == 114)                     return F::r32_float;
            // DXT10 header étendu -> signalé via format::unknown
            if (fc == MakeCC('D','X','1','0')) return F::unknown;
        }

        if (pf.dwFlags & DDPF_RGB)
        {
            if (pf.dwRGBBitCount == 32)
            {
                bool hasAlpha = (pf.dwFlags & DDPF_ALPHAPX) != 0;
                if (pf.dwRBitMask == 0x000000FFu)
                    return hasAlpha ? F::r8g8b8a8_unorm : F::r8g8b8x8_unorm;
                if (pf.dwRBitMask == 0x00FF0000u)
                    return hasAlpha ? F::b8g8r8a8_unorm : F::b8g8r8x8_unorm;
            }
            if (pf.dwRGBBitCount == 16 && pf.dwRBitMask == 0xF800u)
                return F::b5g6r5_unorm;
        }

        if (pf.dwFlags & DDPF_ALPHA)
            return F::a8_unorm;

        return F::unknown;
    }

    inline bool IsBlockCompressed(reshade::api::format fmt)
    {
        using F = reshade::api::format;
        return fmt == F::bc1_unorm || fmt == F::bc1_unorm_srgb
            || fmt == F::bc2_unorm || fmt == F::bc2_unorm_srgb
            || fmt == F::bc3_unorm || fmt == F::bc3_unorm_srgb
            || fmt == F::bc4_unorm || fmt == F::bc4_snorm
            || fmt == F::bc5_unorm || fmt == F::bc5_snorm
            || fmt == F::bc6h_ufloat || fmt == F::bc6h_sfloat
            || fmt == F::bc7_unorm  || fmt == F::bc7_unorm_srgb;
    }

    inline size_t BlockBytes(reshade::api::format fmt)
    {
        using F = reshade::api::format;
        return (fmt == F::bc1_unorm || fmt == F::bc1_unorm_srgb
             || fmt == F::bc4_unorm || fmt == F::bc4_snorm) ? 8u : 16u;
    }

    inline void ComputePitch(reshade::api::format fmt,
                              uint32_t w, uint32_t h,
                              uint32_t& rowPitch, uint32_t& slicePitch)
    {
        if (IsBlockCompressed(fmt))
        {
            uint32_t nbx = (std::max)(1u, (w + 3) / 4);
            uint32_t nby = (std::max)(1u, (h + 3) / 4);
            rowPitch   = uint32_t(nbx * BlockBytes(fmt));
            slicePitch = rowPitch * nby;
        }
        else
        {
            using F = reshade::api::format;
            uint32_t bpp = 32;
            if (fmt == F::r16_float || fmt == F::a8_unorm
             || fmt == F::b5g6r5_unorm)             bpp = 16;
            if (fmt == F::r16g16b16a16_float)        bpp = 64;
            if (fmt == F::r32_float)                 bpp = 32;
            if (fmt == F::r32g32b32a32_float)        bpp = 128;

            rowPitch   = (w * bpp + 7) / 8;
            slicePitch = rowPitch * h;
        }
    }

} // namespace DDS


// ----------------------------------------------------------
//  LoadDDSTexture
//
//  Charge un .dds et crée via reshade::api::device :
//    - reshade::api::resource       (texture GPU)
//    - reshade::api::resource_view  (SRV, à binder en PS)
//
//  Retourne true si succès.
//
//  Destruction :
//    device->destroy_resource_view(outView);
//    device->destroy_resource(outResource);
// ----------------------------------------------------------
inline bool LoadDDSTexture(
    reshade::api::device*           device,
    const wchar_t*                  path,
    reshade::api::resource&         outResource,
    reshade::api::resource_view&    outView)
{
    outResource = { 0 };
    outView     = { 0 };

    // ---- 1. Lire le fichier ----
    FILE* f = nullptr;

    //file name
    wchar_t file_prefix[MAX_PATH] = L"";
    GetModuleFileNameW(nullptr, file_prefix, ARRAYSIZE(file_prefix));
    std::filesystem::path replace_path = file_prefix;
    replace_path = replace_path.parent_path();
    replace_path /= RESHADE_ADDON_SHADER_LOAD_DIR;
    replace_path /= path;

    /*
    if (_wfopen_s(&f, path, L"rb") != 0 || !f)
    {
        log_error_png_not_found(replace_path);
        return false;
    }*/

#ifdef _WIN32
    f = _wfsopen(replace_path.c_str(), L"rb", _SH_DENYNO);
#endif
    if (!f)
    {
        log_error_png_not_found(replace_path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t fileSize = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> buf(fileSize);
    fread(buf.data(), 1, fileSize, f);
    fclose(f);

    const uint8_t* ptr = buf.data();
    const uint8_t* end = ptr + fileSize;

    reshade::log::message(reshade::log::level::error, " *** read_dds - step 2");

    // ---- 2. Vérifier magic + taille minimale ----
    if (fileSize < 4 + sizeof(DDS::Header)
     || *reinterpret_cast<const uint32_t*>(ptr) != DDS::MAGIC)
    {
        log_error_dds_format(replace_path, "magic DDS invalid or file troncated.");
        return false;
    }
    ptr += 4;

    reshade::log::message(reshade::log::level::error, " *** read_dds - step 3");
    // ---- 3. Lire le header ----
    const auto& hdr = *reinterpret_cast<const DDS::Header*>(ptr);
    ptr += sizeof(DDS::Header);

    reshade::api::format fmt = reshade::api::format::unknown;

    if ((hdr.ddspf.dwFlags & DDS::DDPF_FOURCC) &&
         hdr.ddspf.dwFourCC == DDS::MakeCC('D','X','1','0'))
    {
        // Header DXT10 étendu
        if (ptr + sizeof(DDS::HeaderDXT10) > end)
        {
            log_error_dds_format(replace_path, "DXT10 header troncated");
            return false;
        }
        const auto& dxt10 = *reinterpret_cast<const DDS::HeaderDXT10*>(ptr);
        ptr += sizeof(DDS::HeaderDXT10);
        // reshade::api::format est compatible DXGI_FORMAT
        fmt = static_cast<reshade::api::format>(dxt10.dxgiFormat);
    }
    else
    {
        fmt = DDS::GetFormat(hdr.ddspf);
    }

    if (fmt == reshade::api::format::unknown)
    {
        log_error_dds_format(replace_path, "dds format not supported");
        return false;
    }

    const uint32_t width    = hdr.dwWidth;
    const uint32_t height   = hdr.dwHeight;
    //const uint32_t mipCount = (hdr.dwMipMapCount > 0) ? hdr.dwMipMapCount : 1;
    const uint32_t mipCount = 1;

    reshade::log::message(reshade::log::level::error, " *** read_dds - step 4");
    // ---- 4. Collecter les pointeurs de mip ----
    struct MipSlice { const void* data; uint32_t rowPitch; uint32_t slicePitch; };
    std::vector<MipSlice> mips(mipCount);

    for (uint32_t m = 0; m < mipCount; ++m)
    {
        uint32_t w = (std::max)(1u, width  >> m);
        uint32_t h = (std::max)(1u, height >> m);
        uint32_t rp, sp;
        DDS::ComputePitch(fmt, w, h, rp, sp);

        if (ptr + sp > end)
        {
            log_error_dds_format(replace_path, "wip data truncated");
            return false;
        }
        mips[m] = { ptr, rp, sp };
        ptr += sp;
    }
    reshade::log::message(reshade::log::level::error, " *** read_dds - step 5");
    // ---- 5. Créer la resource via reshade::api::device ----
    //
    //  resource_desc décrit la ressource indépendamment du backend.
    //  ReShade se charge de la traduction vers D3D11/D3D12/Vulkan.
    
    reshade::api::resource_desc resDesc = {};
    resDesc.type                    = reshade::api::resource_type::texture_2d;
    resDesc.texture.width           = width;
    resDesc.texture.height          = height;
    resDesc.texture.depth_or_layers = 1;
    resDesc.texture.levels          = static_cast<uint16_t>(mipCount);
    resDesc.texture.format          = fmt;
    resDesc.texture.samples         = 1;
    resDesc.heap                    = reshade::api::memory_heap::cpu_to_gpu;
    resDesc.usage                   = reshade::api::resource_usage::copy_dest;
    resDesc.flags                   = resource_flags::none;
 
  

    // subresource_data : un par mip, pointe dans buf[] encore valide
    std::vector<reshade::api::subresource_data> initData(mipCount);
    for (uint32_t m = 0; m < mipCount; ++m)
    {
        initData[m].data        = const_cast<void*>(mips[m].data);
        initData[m].row_pitch   = mips[m].rowPitch;
        initData[m].slice_pitch = mips[m].slicePitch;
    }

    std::stringstream dbg;
    dbg << "create_resource params:"
        << " w=" << resDesc.texture.width
        << " h=" << resDesc.texture.height
        << " levels=" << resDesc.texture.levels
        << " fmt=" << to_string(resDesc.texture.format)
        << " heap=" << (uint32_t)resDesc.heap
        << " usage=" << (uint32_t)resDesc.usage
        << " initData.size()=" << initData.size()
        << " initData[0].data=" << initData[0].data
        << " initData[0].row_pitch=" << initData[0].row_pitch
        << " SR=" << (uint32_t)reshade::api::resource_usage::shader_resource
        << " CD=" << (uint32_t)reshade::api::resource_usage::copy_dest
        << " general=" << (uint32_t)reshade::api::resource_usage::general
        << " undefined=" << (uint32_t)reshade::api::resource_usage::undefined;
    reshade::log::message(reshade::log::level::error, dbg.str().c_str());


    reshade::log::message(reshade::log::level::error, " *** read_dds - create resource");
    if (!device->create_resource(
            resDesc,
            initData.data(),                            // données initiales
            reshade::api::resource_usage::copy_dest,
            &outResource))
    {
        log_error_png_resource(replace_path, "resource");
        return false;
    }

    reshade::log::message(reshade::log::level::error, " *** read_dds - step 6");
    // ---- 6. Créer la resource_view (SRV) ----
    //
    //  resource_view_desc : texture_2d, tous les mips, layer 0.

    reshade::api::resource_view_desc viewDesc = {};
    viewDesc.type                    = reshade::api::resource_view_type::texture_2d;
    viewDesc.format                  = fmt;
    viewDesc.texture.first_level     = 0;
    viewDesc.texture.level_count     = static_cast<uint32_t>(mipCount);
    viewDesc.texture.first_layer     = 0;
    viewDesc.texture.layer_count     = 1;

    if (!device->create_resource_view(
            outResource,
            //reshade::api::resource_usage::shader_resource,
            reshade::api::resource_usage::copy_dest,
            viewDesc,
            &outView))
    {
        log_error_png_resource(replace_path, "resource_view");
        device->destroy_resource(outResource);
        outResource = { 0 };
        return false;
    }

    log_png_loaded(replace_path);
    return true;
}


// ----------------------------------------------------------
//  DestroyDDSTexture  —  à appeler dans on_destroy_swapchain
// ----------------------------------------------------------
inline void DestroyDDSTexture(
    reshade::api::device*        device,
    reshade::api::resource&      resource,
    reshade::api::resource_view& view)
{
    if (view.handle != 0)     { device->destroy_resource_view(view);  view     = { 0 }; }
    if (resource.handle != 0) { device->destroy_resource(resource);   resource = { 0 }; }
}

} // namespace LoadDDS


// ==============================================================
//  INTÉGRATION DANS L'ADDON  (extrait à coller dans LoadDDS.cpp)
// ==============================================================
//
//  // --- AddonState ---
//  struct AddonState {
//      reshade::api::resource      dialResource = { 0 };
//      reshade::api::resource_view dialView     = { 0 };
//  } g_State;
//
//
//  // --- Chargement dans on_init_swapchain ---
//  static void on_init_swapchain(reshade::api::swapchain* sc)
//  {
//      LoadDDS::LoadDDSTexture(
//          sc->get_device(),
//          L"Chrono1940_Dial.dds",   // dossier du .exe
//          g_State.dialResource,
//          g_State.dialView);
//  }
//
//
//  // --- Binding dans on_reshade_present, avant le draw ---
//  //  Via push_descriptors sur votre pipeline_layout custom :
//
//  reshade::api::descriptor_table_update update = {};
//  update.binding    = 1;           // register(t1) dans le PS
//  update.count      = 1;
//  update.type       = reshade::api::descriptor_type::shader_resource_view;
//  update.descriptors = &g_State.dialView;
//
//  cmd->push_descriptors(
//      reshade::api::shader_stage::pixel,
//      g_PipelineLayout,   // votre pipeline_layout
//      0,                  // index du param layout (set 0)
//      update);
//
//
//  // --- Destruction dans on_destroy_swapchain ---
//  static void on_destroy_swapchain(reshade::api::swapchain* sc)
//  {
//      LoadDDS::DestroyDDSTexture(
//          sc->get_device(),
//          g_State.dialResource,
//          g_State.dialView);
//  }
//
//
//  // --- Enregistrement dans AddonInit ---
//  reshade::register_event<reshade::addon_event::init_swapchain>(on_init_swapchain);
//  reshade::register_event<reshade::addon_event::destroy_swapchain>(on_destroy_swapchain);
