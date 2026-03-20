///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
//  on push_descriptors : used 
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
#include <thread>

#include "loader_addon_shared.h"
#include "addon_functions.h"
#include "addon_logs.h"

#include "export_texture.hpp"
#include "export_CB.hpp"

using namespace reshade::api;

//*******************************************************************************************************
// for hunting
void dump_text_cb(command_list* cmd_list, shader_stage stages, pipeline_layout layout, uint32_t param_index, const descriptor_table_update& update)
{
#ifdef _DEBUG
	// log for shader hunting
	if (g_shared_state->shader_hunter)
	{
#if _DEBUG_LOGS
		log_hunting_push_descriptor(cmd_list, stages, layout, param_index, update);
#endif

	}
	
	//export textures for hunting if requested
	if (flag_capture && a_shared.flag_texture_dump && g_shared_state->save_texture_flag && update.type == descriptor_type::shader_resource_view)
	{
		TextureExporter g_exporter;
		g_exporter.export_descriptors(
			cmd_list,
			stages,
			layout,
			param_index,
			update,
			a_shared.ps_hash_for_text_dump,
			a_shared.count_display
		);


	}

	if (flag_capture && a_shared.flag_cb_dump && g_shared_state->save_cb_flag && update.type == descriptor_type::constant_buffer)
	{
		ConstantBufferExporter cb_exporter;
		cb_exporter.export_constant_buffers(
			cmd_list,
			stages,
			layout,
			param_index,
			update,
			a_shared.ps_hash_for_cb_dump,
			a_shared.count_display,
			true // true = export en .txt (lisible), false = .bin (binaire)
		);
		
	}

#endif
}

//*******************************************************************************************************
// injection of texture 
void get_texture(command_list* cmd_list, shader_stage stages, pipeline_layout layout, uint32_t param_index, const descriptor_table_update& update)
{
#if _DEBUG_LOGS
	//log infos
	log_push_descriptor(stages, layout, param_index, update);
#endif


	device* dev = cmd_list->get_device();

	// get mask from ext plane  PS, filter by the number of resource
	if (a_shared.last_feature == Feature::VS_ext_ownPlane && (update.count == 15 || update.count == 16))
	{
		//default for update.count == 16
		uint32_t text_num = 10;
		uint32_t depth_num = 12;
		if (update.count == 15)
		{
			text_num = 9;
			depth_num = 11;
		}

		// in some case the resource view handle is null, skip these cases
		if (reinterpret_cast<const reshade::api::resource_view*>(update.descriptors)[text_num].handle != 0)
		{

			// to retrieve infos for pushing texture in bind_pipeline
			current_PlaneMask_handle = copy_texture_from_desc(cmd_list, stages, layout, param_index, update, text_num, "PlaneMask");
		}


		if (reinterpret_cast<const reshade::api::resource_view*>(update.descriptors)[depth_num].handle != 0)
		{

			// to retrieve infos for pushing texture in bind_pipeline
			current_depth_handle = copy_texture_from_desc(cmd_list, stages, layout, param_index, update, depth_num, "Depth");
		}


	}

	// get photo texture, it should be T4
	if (a_shared.last_feature == Feature::VS_ownPlane && a_shared.cb_inject_values.photo_on)
	{
		uint32_t text_num = 4;
		// get only texture when needed (widht = 1024, format = bc2_unorm)
		// get resource info 
		reshade::api::resource_view src_resource_view_texture;
		src_resource_view_texture = static_cast<const reshade::api::resource_view*>(update.descriptors)[text_num];
		resource scr_resource = dev->get_resource_from_view(src_resource_view_texture);
		resource_desc src_resource_desc = dev->get_resource_desc(scr_resource);

		//copy texure having size and mips level, tha last one is the good one!
		if (src_resource_desc.texture.width == 1024 && src_resource_desc.texture.levels == 11 )
		{

			// in some case the resource view handle is null, skip these cases
			if (reinterpret_cast<const reshade::api::resource_view*>(update.descriptors)[text_num].handle != 0 && (a_shared.current_photo_number == a_shared.target_photo_number || a_shared.default_photo_number))
			{

				// to retrieve infos for pushing texture in bind_pipeline
				current_Photo_handle = copy_texture_from_desc(cmd_list, stages, layout, param_index, update, text_num, "Photo");
			}

			a_shared.current_photo_number = a_shared.current_photo_number + 1;
			if (a_shared.current_photo_number > a_shared.max_photo_number)
				a_shared.max_photo_number = a_shared.current_photo_number;

		}
	}

	// stop tracking
	track_for_texture = false;
}


#ifdef _DEBUG
extern "C" {
#endif
	// *******************************************************************************************************
	// on_push_descriptors() : to be monitored in order to copy texture and engage effect
	// called a lot !
	VREM_EXPORT  void vrem_on_push_descriptors(command_list* cmd_list, shader_stage stages, pipeline_layout layout, uint32_t param_index, const descriptor_table_update& update)
	{

#if _DEBUG_CRASH 
		reshade::log::message(reshade::log::level::info, "***** addon - vrem_on_push_descriptors started");
#endif
#ifdef _DEBUG
		// for hunting and texture / CB dump
		dump_text_cb(cmd_list, stages, layout, param_index, update);
#endif
		// ********** to be updated for later effects
		// to limit processing only when a tracking is setup 
		// if (!a_shared.render_technique && !track_for_texture && ( ((a_shared.cb_inject_values.hazeReduction == 1.0 && a_shared.cb_inject_values.gCockpitIBL == 1.0) && a_shared.VREM_setting[SET_MISC]) || !a_shared.VREM_setting[SET_MISC])  ) return;
		if (!track_for_texture && !a_shared.render_technique)
		{
#if _DEBUG_CRASH 
			reshade::log::message(reshade::log::level::info, "***** addon - vrem_on_push_descriptors ended (no track)");
#endif
			return;
		}

		// display_to_use = 0 => outer left, 1 = outer right, 2 = Inner left, 3 = inner right.
		short int display_to_use = a_shared.count_display - 1;

		// render effect part
		// do not engage effect if option not selected 
		if (a_shared.render_technique && a_shared.draw_passed && a_shared.VREM_setting[SET_TECHNIQUE])
		{
		
			render_technique(display_to_use, cmd_list);
			a_shared.render_technique = false;
		}

		// inject texture part
	
		if (track_for_texture && update.type == descriptor_type::shader_resource_view && stages == shader_stage::pixel)
		{
			get_texture(cmd_list, stages, layout, param_index, update);
		}

#if _DEBUG_CRASH 
		reshade::log::message(reshade::log::level::info, "***** addon - vrem_on_push_descriptors ended");
#endif

	}
#ifdef _DEBUG
}
#endif

