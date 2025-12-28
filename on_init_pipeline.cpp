// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
// on_init_pipeline() :called once per pipeline, create a persistant list on pipeline, 
// that will be used at each reload of the addon.
// add the pipeline hash in the list of pipeline to be processed if relevant
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

static thread_local std::vector<std::vector<uint8_t>> shader_code;

extern "C" {

	/* void create_vrem_cb(reshade::api::device* device)
	{
		if (a_shared.res_CB13.handle != 0)
			return;  // Déjà créé

		reshade::api::resource_desc desc = {};
		desc.type = reshade::api::resource_type::buffer;
		desc.buffer.size = CBSIZE;
		desc.usage = reshade::api::resource_usage::copy_dest;
		desc.heap = reshade::api::memory_heap::gpu_to_cpu;

		bool result = device->create_resource(
			desc,
			nullptr,
			reshade::api::resource_usage::constant_buffer,
			&a_shared.res_CB13
		);

		if (result) {
			if (g_shared_state->debug)
			{
				std::stringstream s;
				s << "*** creation of cb13 resource " << std::hex << a_shared.res_CB13.handle;
				reshade::log::message(reshade::log::level::info, s.str().c_str());
			}
		}
		else {
			reshade::log::message(reshade::log::level::error, "Error in CB13 resource creation ");
		}
	}
	*/

	//*******************************************************************************
	__declspec(dllexport) void vrem_on_init_pipeline(device* device, pipeline_layout layout, uint32_t subobjectCount, const pipeline_subobject* subobjects, pipeline pipelineHandle)
	{

		// save needed pipelines as init_pipeline is called once per game launch
		// only shader types defined in ALLOWED_SHADERS are saved
		save_pipeline_in_list(device, layout, subobjectCount, subobjects, pipelineHandle);


		//
		// create_vrem_cb(device);

		//if (param.push_descriptors.type == descriptor_type::constant_buffer)
		{

			// create pipeline layout for injecting VREM settings/parameters in CB CBINDEX
			// create_modified_CB_layout(dev, CBINDEX, "VREM settings Cbuffer", SETTINGS_CB_NB);

			g_shared_state->DX11_layout = layout;

			// create pipeline layout for injecting CperFrame parameters in CB CPERFRAME_INDEX
			// create_modified_CB_layout(dev, CPERFRAME_INDEX, "CperFrame", CPERFRAME_CB_NB);
		}

	}
}
/*
		// browse pipeline objets
		for (uint32_t i = 0; i < subobjectCount; i++)
		{		
			{
				const auto type = subobjects[i].type;
				if (std::find(ALLOWED_SHADERS.begin(), ALLOWED_SHADERS.end(), type) != ALLOWED_SHADERS.end())
				{
									
					

					// *****************************************************************************************************************
					//may not be needed anymore
					/*
					//compute has and see if it is declared in shader mod list
					auto* shader_desc = static_cast<const reshade::api::shader_desc*>(subobjects[i].data);

					// Create a non-const copy because calculateShaderHash is also used for cloned shaders
					reshade::api::shader_desc desc_copy = *shader_desc;

					// uint32_t hash = calculateShaderHash(desc_copy);
					uint32_t hash = calculateShaderHash(*shader_desc);

					// if (g_shared_state->VREM_pipelines.pipeline_by_hash.find(hash) == g_shared_state->VREM_pipelines.pipeline_by_hash.end()) 
					
					if (i < MAX_OBJ_PER_PIPELINE)
					{
						// new pipeline => add the pipeline in the persistant list of game pipeline

						PipeLine_Definition pip;
						pip.type = subobjects[i].type;
						pip.hash = hash;
						pip.handle = pipelineHandle.handle;

						g_shared_state->VREM_pipelines.pipeline_by_hash.emplace(pip.hash, pip);
						g_shared_state->VREM_pipelines.pipeline_by_handle.emplace(pip.handle, pip);
						//
						
						
						// log_pipeline_init(pip);
						*/
						/*
						// if the hash is identified in mod shaders, add the pipeline in the list of pipeline to be processed
						if (auto shader = is_in_mod_hash(pip.hash)) {
							
							Shader_Definition pipt = Shader_Definition(shader->action, shader->feature, shader->replace_filename, 0);
							// pipt.hash = shader->hash;
							pipt.hash = hash;
							filtered_pipeline.emplace(pip.handle, pipt);

							log_filtered_added(pip.handle);

							// if action is replace, create a cloned pipeline
							if (shader->action & action_replace_bind || shader->action & action_replace)
							{
								// either replace the shader code or clone the existing pipeline and load the modded shader in it
								//load shader code
								bool status = load_shader_code(shader_code, shader->replace_filename);
								if (!status) {
									// log error
									log_shader_code_error(pipelineHandle, pip.hash, shader->replace_filename);
								}
								else
								{
									// no error, clone pipeline
									// not needed ? keep hash for debug messages
									//newShader.hash = hash;
									// if not done, clone the pipeline 
									clone_pipeline(device, layout, subobjectCount, subobjects, pipelineHandle, shader_code, &pipt);
								}
							}
						}
						*/
						
				/* } */
				
			
					


					// add the pipeline in the list of shader to be processed, with the associated info
					// 
					// auto it = shaders_by_hash.find(hash);
					/*
					std::unordered_map<uint32_t, Shader_Definition>::iterator it = pipeline_by_hash.find(hash);

					if (it != pipeline_by_hash.end()) {
						// shader is to be handled
						// add the shader entry in the map by pipeline handle

						//log
						log_init_pipeline(pipelineHandle, layout, subobjectCount, subobjects, i, hash, it);

						//create the entry for handling shader by pipeline instead of Hash
						Shader_Definition newShader(it->second.action, it->second.feature, it->second.replace_filename, it->second.draw_count);
						newShader.hash = hash;

						if (it->second.action & action_replace_bind)
						{
							// either replace the shader code or clone the existing pipeline and load the modded shader in it
							//load shader code
							bool status = load_shader_code(shader_code, it->second.replace_filename);
							if (!status) {
								// log error
								log_shader_code_error(pipelineHandle, hash, it);
							}
							else
							{
								// no error, clone pipeline
								//keep hash for debug messages
								newShader.hash = hash;
								// if not done, clone the pipeline to have a new version with fixed color for PS
								clone_pipeline(device, layout, subobjectCount, subobjects, pipelineHandle, shader_code, &newShader);
							}
						}
						// setup some global variables according to the feature
						if (it->second.action & action_identify)
						{

						}

						// store new shader to re use it later
						pipeline_by_handle.emplace(pipelineHandle.handle, newShader);
						*/
					//}

/*
					}
					break;
				}
			}
		}
		// else log_invalid_subobjectCount(pipelineHandle);
	}
}
*/