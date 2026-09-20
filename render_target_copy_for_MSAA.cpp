///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
// all fucntions to copy render target for MSAA 
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

ColorResolveState g_color_resolve;

static void destroy_color_resolve(device* dev)
{
    if (g_color_resolve.resolved_srv.handle != 0)
        dev->destroy_resource_view(g_color_resolve.resolved_srv);
    if (g_color_resolve.resolved_srv_srgb.handle != 0 &&
        g_color_resolve.resolved_srv_srgb.handle != g_color_resolve.resolved_srv.handle)
        dev->destroy_resource_view(g_color_resolve.resolved_srv_srgb);
    if (g_color_resolve.resolved_tex.handle != 0)
        dev->destroy_resource(g_color_resolve.resolved_tex);

    g_color_resolve = {};
}

// base format is used to create the non-sRGB SRV
static format make_non_srgb_format(format fmt)
{
    switch (fmt)
    {
    case format::r8g8b8a8_unorm_srgb: return format::r8g8b8a8_unorm;
    case format::b8g8r8a8_unorm_srgb: return format::b8g8r8a8_unorm;
    default: return fmt;
    }
}
static format make_srgb_format(format fmt)
{
    switch (fmt)
    {
    case format::r8g8b8a8_unorm: return format::r8g8b8a8_unorm_srgb;
    case format::b8g8r8a8_unorm: return format::b8g8r8a8_unorm_srgb;
	default: return fmt; // no variant sRGB for this format -> we will reuse the same view
    }
}

static bool ensure_color_resolve_texture(device* dev, uint32_t width, uint32_t height, format src_format, bool is_msaa)
{
    // rien à refaire si déjà bon
    if (g_color_resolve.resolved_tex.handle != 0 &&
        g_color_resolve.width == width &&
        g_color_resolve.height == height &&
        g_color_resolve.format == src_format &&
        g_color_resolve.is_msaa_source == is_msaa)
    {
        return true;
    }

    destroy_color_resolve(dev);

	//typeless-friendy base format use to create both views (linear + sRGB)
    format base_format = make_non_srgb_format(src_format);

    resource_desc desc = {};
    desc.type = resource_type::texture_2d;
    desc.texture.width = width;
    desc.texture.height = height;
    desc.texture.depth_or_layers = 1;
    desc.texture.levels = 1;
	desc.texture.samples = 1; // destination is a resolve target, so it must be non-MSAA
    desc.texture.format = base_format;
    desc.heap = memory_heap::default_;

	//resolve_dest is necessary to be the target of a resolve; shader_resource to be read by shaders
    desc.usage = resource_usage::resolve_dest | resource_usage::shader_resource;

    if (!dev->create_resource(desc, nullptr, resource_usage::resolve_dest, &g_color_resolve.resolved_tex))
    {
        reshade::log::message(reshade::log::level::error, "VREM: echec creation texture resolve COLOR");
        return false;
    }

    resource_view_desc srv_desc(resource_view_type::texture_2d, base_format, 0, 1, 0, 1);
    if (!dev->create_resource_view(g_color_resolve.resolved_tex, resource_usage::shader_resource, srv_desc, &g_color_resolve.resolved_srv))
    {
        reshade::log::message(reshade::log::level::error, "VREM: echec creation SRV (non-sRGB) resolve COLOR");
        destroy_color_resolve(dev);
        return false;
    }

    format srgb_format = make_srgb_format(base_format);
    if (srgb_format != base_format)
    {
        resource_view_desc srv_desc_srgb(resource_view_type::texture_2d, srgb_format, 0, 1, 0, 1);
        if (!dev->create_resource_view(g_color_resolve.resolved_tex, resource_usage::shader_resource, srv_desc_srgb, &g_color_resolve.resolved_srv_srgb))
        {
			// not blocking: we fall back to the same view as the non-sRGB
            g_color_resolve.resolved_srv_srgb = g_color_resolve.resolved_srv;
        }
    }
    else
    {
        g_color_resolve.resolved_srv_srgb = g_color_resolve.resolved_srv;
    }

    g_color_resolve.width = width;
    g_color_resolve.height = height;
    g_color_resolve.format = src_format;
    g_color_resolve.is_msaa_source = is_msaa;

    return true;
}


//*******************************************************************************************************
// backbuffer copy for MSAA

 void update_color_binding_from_backbuffer(effect_runtime* runtime, command_list* cmd_list, resource_view source_rtv)
{
    device* dev = cmd_list->get_device();

    resource src_resource = dev->get_resource_from_view(source_rtv);
    if (src_resource.handle == 0)
        return;

    resource_desc src_desc = dev->get_resource_desc(src_resource);
    const bool is_msaa = src_desc.texture.samples > 1;

    if (g_shared_state->debug_log && flag_capture)
    {
        std::stringstream s;
        s << "**** update_color_binding_from_backbuffer ***";
        reshade::log::message(reshade::log::level::error, s.str().c_str());
    }

    if (!ensure_color_resolve_texture(dev, src_desc.texture.width, src_desc.texture.height, src_desc.texture.format, is_msaa))
        return;

    // state transitiion
	// source : we suppose it is currently usable as render_target (this is the state it is in just after a bind_render_targets_and_depth_stencil).
	// Adapt this "before" state if it is not the case for you (ex: resource_usage::present).
    cmd_list->barrier(src_resource, resource_usage::render_target, is_msaa ? resource_usage::resolve_source : resource_usage::copy_source);
    cmd_list->barrier(g_color_resolve.resolved_tex, resource_usage::shader_resource, is_msaa ? resource_usage::resolve_dest : resource_usage::copy_dest);

    // fucntion only called if MSAA
    if (is_msaa)
    {
        
        if (g_shared_state->debug_log && flag_capture)
        {
            std::stringstream s;
            s << "**** resolve_texture_region to g_color_resolve.resolved_tex.handle = " << g_color_resolve.resolved_tex.handle << " ***";
            reshade::log::message(reshade::log::level::error, s.str().c_str());
        }
        
        cmd_list->resolve_texture_region(
            src_resource, 0, nullptr,
            g_color_resolve.resolved_tex, 0, 0, 0, 0,
            make_non_srgb_format(src_desc.texture.format));
    }

    // Retour aux etats normaux
    cmd_list->barrier(g_color_resolve.resolved_tex, is_msaa ? resource_usage::resolve_dest : resource_usage::copy_dest, resource_usage::shader_resource);
    cmd_list->barrier(src_resource, is_msaa ? resource_usage::resolve_source : resource_usage::copy_source, resource_usage::render_target);

    // --- Injection dans le semantic COLOR ---
    //runtime->update_texture_bindings("COLOR", g_color_resolve.resolved_srv, g_color_resolve.resolved_srv_srgb);
}
