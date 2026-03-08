///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
//  on bind pipeline : do all actions when binding a pipeline
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
#include <unordered_map>


#include "loader_addon_shared.h"
#include "addon_functions.h"
#include "addon_objects.h"
#include "addon_logs.h"

#include "to_string.hpp"


using namespace reshade::api;

static uint32_t last_pipeline_hash_PS = 0;


//*******************************************************************************
// setup variables regarding the action

void process_action_log(std::unordered_map<uint64_t, Shader_Definition>::iterator it)
{
	// PS for global : set eye : 1 = left, 2 = right
	if (it->second.feature == Feature::PS_global)
	{
		a_shared.count_display += 1;
		a_shared.cb_inject_values.count_display = a_shared.count_display;
#if _DEBUG_LOGS  
		// log infos
		log_increase_count_display();
#endif
	}

	//trace VR mode
	if (it->second.feature == Feature::PS_VRMirror)
	{
		a_shared.cb_inject_values.VRMode = 1.0;
#if _DEBUG_LOGS  
		log_mirror_view();
#endif
	}

	// PS for ego plane : stop collecting textures for mask
	if (it->second.feature == Feature::PS_ownPlane)
	{
		// stop tracking resource
		track_for_texture = false;
		a_shared.flag_texture_dump = false;

		// as same VS is called two time, do not get texture for later call (external)
		// a_shared.not_track_mask_anymore = true;

#if _DEBUG_LOGS  
					// log infos
		log_stop_monitor("ego plane mask");
#endif

	}

	if (it->second.feature == Feature::PS_external)
	{
		// for security in case of (2 call for the same VS)
		track_for_texture = false;

		// as same VS is called two time, do not get texture for later call
		a_shared.not_track_mask_anymore = true;


		//dump textures at next push_descriptors
		a_shared.flag_texture_dump = false;
	}


	// PS for GUI : set flag
	if (it->second.feature == Feature::GUI)
	{
		a_shared.cb_inject_values.GUItodraw = 1.0;

		// log infos
#if _DEBUG_LOGS  
		log_start_monitor("GUItodraw");
#endif
	}


	/* DCS stuff
	if (it->second.feature == Feature::VS_global2)
	{
		// handle the case where the Ps is called 2 time consecutivelly because of mirror view
		if (a_shared.last_feature != Feature::Global && !a_shared.cb_inject_values.mapMode)
		{

			// if texure has been copied previously, increase draw count, otherwise do nothing, to avoid counting shader calls for MFD rendering
			// if (a_shared.depthStencil_copy_started)
			{

				a_shared.count_display += 1;
				// log max of count_display to enable or not features for VR / Quad view
				a_shared.count_draw = max(a_shared.count_draw, a_shared.count_display);
				// it's stupid but I'm too lazy to change code now..
				a_shared.cb_inject_values.count_display = a_shared.count_display;

				a_shared.track_for_render_target = false;
#if _DEBUG_LOGS
							// log infos
							log_increase_count_display();
#endif

							if (a_shared.VREM_setting[SET_EFFECTS] && !a_shared.cb_inject_values.mapMode)
							{
								// handle effects : setup flag for draw
								a_shared.render_technique = true;
								//a_shared.track_for_render_target = false;
#if _DEBUG_LOGS
								// log infos
								log_effect_requested();
#endif
							}
						}
						*/

						/* else
						{
							// log infos
							log_not_increase_draw_count();
						} */
						/*
							}


							// set up draw flag to avoid push_constant() doing effect before draw (it will be overwritten by the PS)
							a_shared.draw_passed = false;
						} */

						// set flag for tracking render target if feature enabled and not in 2D
				// if (it->second.feature == Feature::Effects && shared_data.effects_feature && shared_data.count_draw > 1)
				// TODO test to make it work in 2D
	/*
	if (it->second.feature == Feature::mapMode)
	{
		a_shared.cb_inject_values.mapMode = 0.0;
		a_shared.cockpit_rendering_started = true;
	}

	// PS for mirror view : setup VR mode
	if (it->second.feature == Feature::VRMode)
	{
		a_shared.cb_inject_values.VRMode = 1.0;
		// identify which view was used before mirror view
		// defaut
		a_shared.mirror_VR = 0;
		// secure only 1 and 2 view processed
		if (a_shared.count_display == 1) a_shared.mirror_VR = 0;
		if (a_shared.count_display == 2) a_shared.mirror_VR = 1;
#if _DEBUG_LOGS
					log_mirror_view();
#endif
				}
				*/
}

//*******************************************************************************
// inject texture 
void process_action_injectText(command_list* commandList, std::unordered_map<uint64_t, Shader_Definition>::iterator it)
{
	// inject texture using push_descriptor() if things has been initialized => draw index is > -1

//check if the current planeMask is declared
	auto it_mask = a_shared.copied_textures.find(current_PlaneMask_handle);
	if (it_mask != a_shared.copied_textures.end())
	{
		// stencil depth textures in shaders 
		if ((it->second.feature == Feature::PS_global || it->second.feature == Feature::PS_sun) && a_shared.copied_textures[current_PlaneMask_handle].copied)
		{

			//inject mask texture, if slot =0 => t3 as 3 is defined in pipeline_layout
			inject_texture(commandList, 0, current_PlaneMask_handle, "PlaneMask");

			//inject depth texture, if slot = 1 => t4 as 3 is defined in pipeline_layout
			inject_texture(commandList, 1, current_depth_handle, "Depth");
		}

		// photo texture in shader
		if (it->second.feature == Feature::PS_global && a_shared.copied_textures[current_Photo_handle].copied)
		{
			//inject photo texture, if slot = 2 => t5 as 3 is defined in pipeline_layout
			inject_texture(commandList, 2, current_Photo_handle, "Photo");
		}
	}
}

//*******************************************************************************
// replace shader if needed and inject CB for mod parameters
void process_action_replace(command_list* commandList, pipeline_stage stages, pipeline pipelineHandle, std::unordered_map<uint64_t, Shader_Definition>::iterator it)
{
	// TODO : add optimization to avoid pushing CB13 if not needed
	{
		// use push constant() to push the mod parameter in CB13,a sit is assumed a replaced shader will need mod parameters
		// pipeline_layout for CB initialized in init_pipeline() once for all

		commandList->push_constants(
			shader_stage::all,
			a_shared.saved_pipeline_layout_CB[SETTINGS_CB_NB],
			0,
			0,
			CBSIZE,
			&a_shared.cb_inject_values
		);
#if _DEBUG_LOGS  
		log_CB_injected("VREM CB");
#endif
	}
	if (it->second.action & action_replace_bind || ((it->second.action & action_replace) && g_shared_state->debug))
	{
		// shader is to be replaced by the new one created in on_Init_Pipeline
		commandList->bind_pipeline(stages, it->second.substitute_pipeline);
		// log infos
#if _DEBUG_LOGS  
		log_pipeline_replaced(pipelineHandle.handle, it->second.substitute_pipeline.handle);
#endif
	}
}

//*******************************************************************************
// setup flags to get texture
void process_action_action_get_text(std::unordered_map<uint64_t, Shader_Definition>::iterator it)
{
	// track stencil and depth
	if (it->second.feature == Feature::VS_ext_ownPlane && !a_shared.not_track_mask_anymore)
	{
		//set flag to get mask texture at next push_descriptors
		track_for_texture = true;

#if _DEBUG_LOGS  
		// log infos
		log_start_monitor("ego plane mask");
#endif

	}

	// track photo texture
	if (it->second.feature == Feature::VS_ownPlane)
	{
		//set flag to get mask texture at next push_descriptors
		track_for_texture = true;

#if _DEBUG_LOGS  
		// log infos
		log_start_monitor("photo");
#endif

	}

	a_shared.last_feature = it->second.feature;
}

//*******************************************************************************
// setup flags to dump  textures
void process_action_action_dump_text(std::unordered_map<uint64_t, Shader_Definition>::iterator it)
{
	// if (it->second.feature == Feature::VS_ext_ownPlane && !a_shared.not_track_mask_anymore)
	if (!a_shared.not_track_mask_anymore)
	{
		//set flag to get mask texture at next push_descriptors
		track_for_texture = true;

#if _DEBUG_LOGS  
		// log infos
		log_start_monitor("VS plane&ext : request text. dump");
#endif
		//dump textures at next push_descriptors
		a_shared.flag_texture_dump = true;
		a_shared.ps_hash_for_text_dump = 0x0;
	}
}

//*******************************************************************************
// setup flags to render technique
void process_action_action_trackRT(std::unordered_map<uint64_t, Shader_Definition>::iterator it)
{
	if (g_shared_state->technique_enabled)
	{
		a_shared.track_for_render_target = true;
#if _DEBUG_LOGS  
		// log infos
		log_start_monitor("Render target");
#endif
	}
}

//*******************************************************************************
// setup flags to render technique
void process_action_action_renderTechnique(std::unordered_map<uint64_t, Shader_Definition>::iterator it)
{
	if (g_shared_state->technique_enabled)
	{
		
		a_shared.track_for_render_target = false;

		a_shared.render_technique = true;
#if _DEBUG_LOGS  
		// log infos
		log_start_monitor("end of traking Render target");
#endif
	}
}


//*******************************************************************************
// inject modified CB (not used in IL2 yet, keep only for future DCS VREM2 mod)
void process_action_action_action_injectCB(command_list* commandList, std::unordered_map<uint64_t, Shader_Definition>::iterator it)
{
	// inject constant buffer other than the one containing VREM setting

	//CPERFRAME/haze for global illumination :  need to modify value for haze but set orgi. value for reflection
	if (it->second.feature == Feature::GetStencil && a_shared.CB_copied[CPERFRAME_CB_NB] && a_shared.VREM_setting[SET_MISC])
	{

		//modify value for Haze
		a_shared.dest_CB_array[CPERFRAME_CB_NB][FOG_INDEX] = a_shared.orig_values[CPERFRAME_CB_NB][GATMINTENSITY_SAVE] * a_shared.cb_inject_values.hazeReduction;
		// other value
		a_shared.dest_CB_array[CPERFRAME_CB_NB][GCOCKPITIBL_INDEX_X] = a_shared.orig_values[CPERFRAME_CB_NB][GCOCKPITIBL_X_SAVE];
		a_shared.dest_CB_array[CPERFRAME_CB_NB][GCOCKPITIBL_INDEX_Y] = a_shared.orig_values[CPERFRAME_CB_NB][GCOCKPITIBL_Y_SAVE];
		// use push constant() to push CPerFrame 
		// pipeline_layout for CB initialized in init_pipeline() once for all
		commandList->push_constants(
			shader_stage::all,
			a_shared.saved_pipeline_layout_CB[CPERFRAME_CB_NB],
			0,
			0, // can not injecting only the haze value to be updated (so first = FOG_INDEX) because it is making the game crash...
			CPERFRAME_SIZE,
			&a_shared.dest_CB_array[CPERFRAME_CB_NB]
		);
#if _DEBUG_LOGS  
		log_CB_injected("CPerFrame updated for fog, GCOCKPITIBL default");
#endif

		// last_replaced_shader = pipelineHandle.handle;
		a_shared.last_feature = it->second.feature;
	}

	//CPERFRAME/haze for other shaders :  need to keep orig. value
	if (it->second.feature == Feature::Sky && a_shared.CB_copied[CPERFRAME_CB_NB] && a_shared.VREM_setting[SET_MISC])
	{

		//modify value for Haze
		a_shared.dest_CB_array[CPERFRAME_CB_NB][FOG_INDEX] = a_shared.orig_values[CPERFRAME_CB_NB][GATMINTENSITY_SAVE];
		//other value
		a_shared.dest_CB_array[CPERFRAME_CB_NB][GCOCKPITIBL_INDEX_X] = a_shared.orig_values[CPERFRAME_CB_NB][GCOCKPITIBL_X_SAVE];
		a_shared.dest_CB_array[CPERFRAME_CB_NB][GCOCKPITIBL_INDEX_Y] = a_shared.orig_values[CPERFRAME_CB_NB][GCOCKPITIBL_Y_SAVE];

		// use push constant() to push CPerFrame 
		// pipeline_layout for CB initialized in init_pipeline() once for all
		commandList->push_constants(
			shader_stage::all,
			a_shared.saved_pipeline_layout_CB[CPERFRAME_CB_NB],
			0,
			0,
			CPERFRAME_SIZE,
			&a_shared.dest_CB_array[CPERFRAME_CB_NB]
		);
#if _DEBUG_LOGS  
		log_CB_injected("CPerFrame original");
#endif

		// last_replaced_shader = pipelineHandle.handle;
		a_shared.last_feature = it->second.feature;

	}

	//CPERFRAME/reflect for instrument
	if (it->second.feature == Feature::NoReflect && a_shared.CB_copied[CPERFRAME_CB_NB])
	{

		//modify value
		a_shared.dest_CB_array[CPERFRAME_CB_NB][GCOCKPITIBL_INDEX_X] = a_shared.orig_values[CPERFRAME_CB_NB][GCOCKPITIBL_X_SAVE] * a_shared.cb_inject_values.gCockpitIBL;
		a_shared.dest_CB_array[CPERFRAME_CB_NB][GCOCKPITIBL_INDEX_Y] = a_shared.orig_values[CPERFRAME_CB_NB][GCOCKPITIBL_Y_SAVE] * a_shared.cb_inject_values.gCockpitIBL;

		// use push constant() to push CPerFrame 
		// pipeline_layout for CB initialized in init_pipeline() once for all
		commandList->push_constants(
			shader_stage::all,
			a_shared.saved_pipeline_layout_CB[CPERFRAME_CB_NB],
			0,
			0,
			CPERFRAME_SIZE,
			&a_shared.dest_CB_array[CPERFRAME_CB_NB]
		);
#if _DEBUG_LOGS  
		log_CB_injected("CPerFrame for GCOCKPITIBL");
#endif

		// last_replaced_shader = pipelineHandle.handle;
		a_shared.last_feature = it->second.feature;
	}
}

#ifdef _DEBUG
extern "C" {
#endif
	//*******************************************************************************
	// on bind pipeline : do all actions when binding a pipeline
	VREM_EXPORT void vrem_on_bind_pipeline(command_list* commandList, pipeline_stage stages, pipeline pipelineHandle)
	{
		
		
		// optimize performance by reducing processing to ALLOWED_STAGES
		if ((static_cast<uint32_t>(stages) & static_cast<uint32_t>(ALLOWED_STAGES)) == 0) return;
#if _DEBUG_CRASH reshade::log::message(reshade::log::level::info, "***** addon - vrem_on_bind_pipeline started");
#endif

#ifdef _DEBUG		
		//for rt dump engage dump at next draw only if change of PS
		if (stages == pipeline_stage::pixel_shader && g_shared_state->save_rt_flag && flag_capture)
		{

			save_pipeline* pipe = find__pipeline_per_handle(pipelineHandle.handle);

			//if (pipe->hash[0] != a_shared.last_pipeline_hash_PS)
			if (pipe->hash[0] != last_pipeline_hash_PS)
			{
				uint32_t fisrthash = pipe->hash[0];
				
				save_render_target(commandList, last_pipeline_hash_PS);
				//a_shared.last_pipeline_hash_PS = pipe->hash[0];
				last_pipeline_hash_PS = pipe->hash[0];
			}

		}

		// store handle if shader hunting and PS, replace pipleine or skip the draw call
		if (g_shared_state->shader_hunter)
		{
			
			on_bind_pipeline_hunting(commandList, stages, pipelineHandle);
			// skip processing if shader hunting
#if _DEBUG_CRASH  reshade::log::message(reshade::log::level::info, "***** addon - vrem_on_bind_pipeline ended (hunting)");
#endif
			return;
		}
#endif
		// identify if the pipeline is to be processed
		auto it = filtered_pipeline.find(pipelineHandle.handle);

		if (it == filtered_pipeline.end()) [[likely]]
		{
#if _DEBUG_CRASH reshade::log::message(reshade::log::level::info, "***** addon - vrem_on_bind_pipeline ended (not used)");
#endif
			return;
		}
		{
		
			//shader is found in the filtered pipeline list
			Shader_Definition& shader = it->second;
#if _DEBUG_LOGS  
			log_shader_binded(pipelineHandle.handle, shader);
#endif
			// use shader
			uint32_t action = shader.action;
			auto& pipeline = shader.substitute_pipeline;

			// setup variables regarding the action
			if (it->second.action & action_log) process_action_log(it);

			// inject texture
			if (it->second.action & action_injectText)	process_action_injectText(commandList, it);

			// replace pipelines during bind and inject CB for mod parameters
			if (it->second.action & action_replace_bind || it->second.action & action_replace)
				process_action_replace(commandList, stages, pipelineHandle, it);

			// get texture 
			if (it->second.action & action_get_text) process_action_action_get_text(it);

			//dump texture (debug only)
#ifdef _DEBUG
			if ((it->second.action & action_dump) && flag_capture) process_action_action_dump_text(it);
#endif
			// setup flag for tracking render target for technique
			if (it->second.action & action_track_RT) process_action_action_trackRT(it);

			// setup flag for trendering technique
			if (it->second.action & action_renderTechnique) process_action_action_renderTechnique(it);

			// not used in IL2 inject modified CB (modifed or restored version)
			// if (it->second.action & action_injectCB) process_action_action_action_injectCB(commandList, it);

			// trace current feature for next call
			a_shared.last_feature = it->second.feature;
		}

#if _DEBUG_CRASH reshade::log::message(reshade::log::level::info, "***** addon - vrem_on_bind_pipeline ended");
#endif
	}
#ifdef _DEBUG
}
#endif