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
#include "VREM_settings.h"
#include "addon_functions.h"
#include "addon_logs.h"

using namespace reshade::api;

#ifdef _DEBUG
extern "C" {
#endif
	// *******************************************************************************************************
	// on_push_descriptors() : to be monitored in order to copy texture and engage effect
	// called a lot !
	VREM_EXPORT  void vrem_on_push_descriptors(command_list* cmd_list, shader_stage stages, pipeline_layout layout, uint32_t param_index, const descriptor_table_update& update)
	{

		// log for shader hunting
		if (g_shared_state->shader_hunter)
		{
			log_hunting_push_descriptor(cmd_list, stages, layout, param_index, update);
			return;
		}

		// to limit processing only when a tracing is setup
		if (!a_shared.render_effect && !track_for_depthStencil && ( ((a_shared.cb_inject_values.hazeReduction == 1.0 && a_shared.cb_inject_values.gCockpitIBL == 1.0) && a_shared.VREM_setting[SET_MISC]) || !a_shared.VREM_setting[SET_MISC])  ) return;
		
		// display_to_use = 0 => outer left, 1 = outer right, 2 = Inner left, 3 = inner right.
		short int display_to_use = a_shared.count_display - 1;

		// render effect part
		// do not engage effect if option not selected and not in cockpit
		if (a_shared.render_effect && a_shared.VREM_setting[SET_EFFECTS] && !a_shared.cb_inject_values.mapMode && a_shared.draw_passed)
		{
			render_effect(display_to_use, cmd_list);
		}

		//handle only shader_resource_view when needed
		// handle depthStencil
		if (track_for_depthStencil && update.type == descriptor_type::shader_resource_view && stages == shader_stage::pixel && update.count == 6)
		{

			// in some case the resource view handle is null, skip these cases
			if (reinterpret_cast<const reshade::api::resource_view *>(update.descriptors)[5].handle != 0)
			{
				//log infos
				log_push_descriptor(stages, layout, param_index, update);

				// copy depthStencil texture into shared_data
				bool status = copy_depthStencil(cmd_list, stages, layout, param_index, update);
			}

			// stop tracking
			// shared_data.track_for_depthStencil = false;
		}

		// handle NS430
		/* if (shared_data.track_for_NS430 && update.type == descriptor_type::shader_resource_view && stages == shader_stage::pixel && update.count == 1)
		{
			//log infos
			log_push_descriptor(stages, layout, param_index, update);

			// Copy only if texture has good resolution
			device* dev = cmd_list->get_device();
			resource_view NS430_rv = static_cast<const reshade::api::resource_view*>(update.descriptors)[0];
			resource NS430_resource = dev->get_resource_from_view(NS430_rv);
			resource_desc NS430_res_desc = dev->get_resource_desc(NS430_resource);


			if (NS430_res_desc.texture.width == NS430_textSizeX && NS430_res_desc.texture.height == NS430_textSizeY)
			{
				// copy NS430 texture into shared_data (only done once per frame)
				bool status = copy_NS430_text(cmd_list, stages, layout, param_index, update);

				// stop tracking
				shared_data.track_for_NS430 = false;
			}

		}
		*/


		if (g_shared_state->debug && flag_capture && a_shared.render_effect)
		{
			std::stringstream s;
			s << "addon - vrem_on_push_descriptors : a_shared.render_effect : " << a_shared.render_effect << ", a_shared.VREM_setting[SET_EFFECTS] : " << a_shared.VREM_setting[SET_EFFECTS] << ", a_shared.cb_inject_values.mapMode : " << a_shared.cb_inject_values.mapMode << ", a_shared.draw_passed : " << a_shared.draw_passed << "; ";
			reshade::log::message(reshade::log::level::info, s.str().c_str());
		}
		
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

