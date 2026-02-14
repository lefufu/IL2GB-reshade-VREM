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

#ifdef _DEBUG
extern "C" {
#endif
	// *******************************************************************************************************
	// on_push_descriptors() : to be monitored in order to copy texture and engage effect
	// called a lot !
	VREM_EXPORT  void vrem_on_push_descriptors(command_list* cmd_list, shader_stage stages, pipeline_layout layout, uint32_t param_index, const descriptor_table_update& update)
	{

#ifdef _DEBUG
		// log for shader hunting
		if (g_shared_state->shader_hunter)
		{
#if _DEBUG_LOGS
			log_hunting_push_descriptor(cmd_list, stages, layout, param_index, update);
#endif

		}
		/*
		if (g_shared_state->debug_log && flag_capture && a_shared.flag_texture_dump)
		{
			std::stringstream s;
			s << "addon - vrem_on_push_descriptors : a_shared.flag_texture_dump = " << a_shared.flag_texture_dump << ", update.type = " << to_string(update.type) << "; ";
			reshade::log::message(reshade::log::level::info, s.str().c_str());
		}*/

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
			/*
			if (g_shared_state->debug_log && flag_capture)
			{
				std::stringstream s;
				s << "addon - vrem_on_push_descriptors : should dump textures, param_index = " << param_index << "; ";
				reshade::log::message(reshade::log::level::info, s.str().c_str());
			}*/

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
		// ********** to be updated for later effects
		// to limit processing only when a tracking is setup 
		// if (!a_shared.render_effect && !track_for_planeMask && ( ((a_shared.cb_inject_values.hazeReduction == 1.0 && a_shared.cb_inject_values.gCockpitIBL == 1.0) && a_shared.VREM_setting[SET_MISC]) || !a_shared.VREM_setting[SET_MISC])  ) return;
		if (!track_for_planeMask) return;


		// display_to_use = 0 => outer left, 1 = outer right, 2 = Inner left, 3 = inner right.
		short int display_to_use = a_shared.count_display - 1;

		// render effect part
		// do not engage effect if option not selected and not in cockpit
		if (a_shared.render_effect && a_shared.VREM_setting[SET_EFFECTS] && !a_shared.cb_inject_values.mapMode && a_shared.draw_passed)
		{
			render_effect(display_to_use, cmd_list);
		}

		//handle only shader_resource_view when needed
		// handle mask => 18 textures for the shader !
		// if (track_for_planeMask && update.type == descriptor_type::shader_resource_view && stages == shader_stage::pixel && update.count == 18) 
		if (track_for_planeMask && update.type == descriptor_type::shader_resource_view && stages == shader_stage::pixel)
		{
#if _DEBUG_LOGS
			//log infos
			log_push_descriptor(stages, layout, param_index, update);
#endif
			/*
			// get mask from onw plane  PS, filter by the number of resource
			if (a_shared.last_feature == Feature::VS_ext_ownPlane && (update.count == 18 || update.count == 17))
			{

				// in some case the resource view handle is null, skip these cases
				if (reinterpret_cast<const reshade::api::resource_view*>(update.descriptors)[8].handle != 0)
				{
					// to retrieve infos for pushing texture in bind_pipeline
					current_PlaneMask_handle = copy_texture_from_desc(cmd_list, stages, layout, param_index, update, 8, "PlaneMask");
				}
			}
			*/

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

			// stop tracking
			track_for_planeMask = false;
		}


		/*if (g_shared_state->debug && flag_capture && a_shared.render_effect)
		{
			std::stringstream s;
			s << "addon - vrem_on_push_descriptors : a_shared.render_effect : " << a_shared.render_effect << ", a_shared.VREM_setting[SET_EFFECTS] : " << a_shared.VREM_setting[SET_EFFECTS] << ", a_shared.cb_inject_values.mapMode : " << a_shared.cb_inject_values.mapMode << ", a_shared.draw_passed : " << a_shared.draw_passed << "; ";
			reshade::log::message(reshade::log::level::info, s.str().c_str());
		}*/
		
		//handle CB modification
		// CB cPerFrame is generated once at the beginning of the frame, it is not needed to use a dedicated shader to track the push_descriptor command
		if ((a_shared.cb_inject_values.hazeReduction != 1.0 || a_shared.cb_inject_values.gCockpitIBL != 1.0) && a_shared.VREM_setting[SET_MISC])
		{


			if (update.type == descriptor_type::constant_buffer && update.binding == CPERFRAME_INDEX && update.count == 1 && stages == shader_stage::pixel)
			{

				bool error = read_constant_buffer(cmd_list, update, "CPerFrame", 0, a_shared.dest_CB_array[CPERFRAME_CB_NB], CPERFRAME_SIZE);
				if (!error)
				{

					// copy original value for gAtmIntensity
					a_shared.orig_values[CPERFRAME_CB_NB][GATMINTENSITY_SAVE] = a_shared.dest_CB_array[CPERFRAME_CB_NB][FOG_INDEX];

					// copy original value for gCockpitIBL.xy
					a_shared.orig_values[CPERFRAME_CB_NB][GCOCKPITIBL_X_SAVE] = a_shared.dest_CB_array[CPERFRAME_CB_NB][GCOCKPITIBL_INDEX_X];
					a_shared.orig_values[CPERFRAME_CB_NB][GCOCKPITIBL_Y_SAVE] = a_shared.dest_CB_array[CPERFRAME_CB_NB][GCOCKPITIBL_INDEX_Y];


					a_shared.CB_copied[CPERFRAME_CB_NB] = true;
				}

			}
		}
	}
#ifdef _DEBUG
}
#endif

