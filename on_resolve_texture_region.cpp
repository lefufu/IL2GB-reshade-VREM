///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
// on_resolve_texture_region : track MSAA resolve and save textures if requested
// ----------------------------------------------------------------------------------------
// 
// (c) Lefuneste.
//
// All rights reserved.
// https://github.com/xxx
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
//  * Redistributions of source code must retain the above copyright notice, this
//	  list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and / or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// This software is using part of code or algorithms provided by
// * Crosire https://github.com/crosire/reshade  
// * FransBouma https://github.com/FransBouma/ShaderToggler
// * ShortFuse https://github.com/clshortfuse/renodx
// 
/////////////////////////////////////////////////////////////////////////

#include <reshade.hpp>

#include "loader_addon_shared.h"
#include "addon_functions.h"
#include "addon_objects.h"
#include "addon_logs.h"

#include "to_string.hpp"

using namespace reshade::api;

// *******************************************************************************************************

// cle = handle de la resource (uint64_t), valeur = RTV cree dessus
static std::unordered_map<uint64_t, resource_view> g_rtv_cache;

// a appeler dans on_destroy_resource (event ReShade) pour eviter les handles perimes
static void invalidate_cached_rtv(device* dev, resource res)
{
    auto it = g_rtv_cache.find(res.handle);
    if (it != g_rtv_cache.end())
    {
        dev->destroy_resource_view(it->second);
        g_rtv_cache.erase(it);
    }
}

// a appeler une seule fois, typiquement dans on_destroy_device ou au unload de l'addon
static void clear_rtv_cache(device* dev)
{
    for (auto& [handle, rtv] : g_rtv_cache)
        dev->destroy_resource_view(rtv);
    g_rtv_cache.clear();
}

// renvoie une RTV valide sur "res", en la creant si besoin.
// "fmt" doit etre le format de la resource (recupere via get_resource_desc).
static resource_view get_or_create_cached_rtv(device* dev, resource res, format fmt, bool is_msaa)
{
    if (res.handle == 0)
        return resource_view{ 0 };

    auto it = g_rtv_cache.find(res.handle);
    if (it != g_rtv_cache.end())
        return it->second;

    resource_view_type view_type = is_msaa ? resource_view_type::texture_2d_multisample
        : resource_view_type::texture_2d;

    resource_view_desc rtv_desc(view_type, fmt, 0, 1, 0, 1);

    resource_view new_rtv{ 0 };
    if (!dev->create_resource_view(res, resource_usage::render_target, rtv_desc, &new_rtv))
    {
        reshade::log::message(reshade::log::level::error,
            "VREM: echec creation RTV cachee pour resolve non tracke");
        return resource_view{ 0 };
    }

    g_rtv_cache[res.handle] = new_rtv;
    return new_rtv;
}


#ifdef _DEBUG
extern "C" {
#endif
	// *******************************************************************************************************
	// vrem_on_resolve_texture_region() : to call when effects are reloaded, should not do aything before real effect compilation (another call from on_present)
	// called a lot !n
	VREM_EXPORT bool vrem_on_resolve_texture_region(command_list* cmd_list, resource source, uint32_t source_subresource, const subresource_box* source_box, resource dest, uint32_t dest_subresource, uint32_t dest_x, uint32_t dest_y, uint32_t dest_z, reshade::api::format format)
	{

#if _DEBUG_CRASH 
		reshade::log::message(reshade::log::level::info, "addon - vrem_on_resolve_texture_region started");
#endif
        if (g_shared_state->debug_log && flag_capture)
        {
            device* dev = cmd_list->get_device();

            resource_desc src_desc = dev->get_resource_desc(source);
            resource_desc dst_desc = dev->get_resource_desc(dest);

            bool matches_tracked_rt = false;
            if (last_RTV_saved.RV.handle != 0)
            {
                resource tracked_resource = dev->get_resource_from_view(last_RTV_saved.RV);
                matches_tracked_rt = (tracked_resource.handle == source.handle);
            }

            std::stringstream s;
            s << "[VREM][RESOLVE #"
                << "src=" << std::hex << source.handle << std::dec
                << " (" << src_desc.texture.width << "x" << src_desc.texture.height
                << ", samples=" << (int)src_desc.texture.samples << ") "
                << "-> dst=" << std::hex << dest.handle << std::dec
                << " (" << dst_desc.texture.width << "x" << dst_desc.texture.height << ") "
                << "| src == render_target_VREM ? " << (matches_tracked_rt ? "OUI" : "non")
                << " | technique deja rendue sur cette passe ? " << (a_shared.vrem_technique_rendered_flag ? "OUI" : "non");

            reshade::log::message(reshade::log::level::warning, s.str().c_str());
        }


        if (a_shared.render_technique && a_shared.draw_passed && a_shared.VREM_setting[SET_TECHNIQUE] && a_shared.cb_inject_values.MSAA > 0)
        {
            // display_to_use = 0 => outer left, 1 = outer right, 2 = Inner left, 3 = inner right.
            short int display_to_use = a_shared.count_display - 1;

            resource_desc src_desc = g_shared_state->device->get_resource_desc(dest);

            //if (src_desc.texture.samples > 1)
            {
                resource_view rtv_for_this_resource = get_or_create_cached_rtv(
                    g_shared_state->device, dest, src_desc.texture.format, src_desc.texture.samples > 1);

                last_RTV_saved.RV = rtv_for_this_resource;
                last_RTV_saved.copied = true;

                render_technique(display_to_use, cmd_list);
                a_shared.render_technique = false;
            }
            a_shared.render_technique = false;
        }


        return false;

#if _DEBUG_CRASH 
		reshade::log::message(reshade::log::level::info, "addon - vrem_on_resolve_texture_region ended");
#endif



	}
#ifdef _DEBUG
}
#endif