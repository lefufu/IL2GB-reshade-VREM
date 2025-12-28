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
#include "VREM_settings.h"
#include "addon_logs.h"

#include "to_string.hpp"


using namespace reshade::api;


extern "C" {
	//*******************************************************************************
	__declspec(dllexport) void vrem_on_bind_pipeline(command_list* commandList, pipeline_stage stages, pipeline pipelineHandle) 
	{
		
		// for frame capture, to ensure there is at least one bind_pipeline in the frame => need to be moved to another event sooner than bind_pipeline !
		if (request_capture)
		{
			request_capture = false;
			flag_capture = true;
			frame_started = true;
			log_start_capture_frame();
		}
		
		
		
		// identify if the pipeline is to be processed
		auto it = filtered_pipeline.find(pipelineHandle.handle);

		if (it != filtered_pipeline.end())
		{

			//shader is found in the filtered pipeline list
			Shader_Definition& shader = it->second;
			
			log_shader_binded(pipelineHandle.handle, shader);

			// use shader
			uint32_t action = shader.action;
			auto& pipeline = shader.substitute_pipeline;

			// if (it->second.action & action_replace_bind || it->second.action & action_replace)
			if (it->second.action & action_replace_bind )
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
					log_CB_injected("VREM CB");	
				}
				// shader is to be replaced by the new one created in on_Init_Pipeline
				commandList->bind_pipeline(stages, it->second.substitute_pipeline);
				// log infos
				log_pipeline_replaced(pipelineHandle.handle, it->second.substitute_pipeline.handle);
			}
		}

	}
}