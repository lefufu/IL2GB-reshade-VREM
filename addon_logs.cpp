///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
// VREM logs function defintion 
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

#include "VREM_settings.h"
#include "loader_addon_shared.h"
#include "to_string.hpp"
#include "addon_objects.h"

//extern SharedState g_shared_state;

void log_addon_init() {
	if (g_shared_state->debug)
	{
		reshade::log::message(reshade::log::level::info, "VREM addon : initialization");
	}
}

void log_addon_cleanup() {
	if (g_shared_state->debug)
	{
		reshade::log::message(reshade::log::level::info, "VREM addon : cleaning up");
	}
}

void log_error_array_uniform(std::string effect_name, std::string uniform_name, uint32_t out_array_length) {
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "effect name=" << effect_name << ", uniform name = " << uniform_name;
		s << "array_length > 1 :" << out_array_length << ";";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_uniform(std::string effect_name, std::string uniform_name, float uniform_value){
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "effect name=" << effect_name << ", uniform name = " << uniform_name << ", uniform value =" << uniform_value << "; ";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_effect_reloaded() {
	if (g_shared_state->debug)
	{
		reshade::log::message(reshade::log::level::info, "VREM addon :effect reloaded => update settings from uniforms");
	}
}

void log_pipeline_init(PipeLine_Definition pipeline)
{
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "on_init_pipeline : added pipeline, hash = " << std::hex << pipeline.hash << ", handle = " << std::hex << pipeline.handle << ", type = " << to_string(pipeline.type) << "; ";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_filtered_added(uint64_t handle) {
	if (g_shared_state->debug)
	{
		std::stringstream s;
		//filtered_pipeline

		s << "on_init_pipeline : added filtered pipeline, handle = " << std::hex << handle << ", hash = " << std::hex << filtered_pipeline[handle].hash;
		s << ", action = " << to_string(filtered_pipeline[handle].action) << ", feature = " << to_string(filtered_pipeline[handle].feature);
		s << ", replace_filename = " << to_string(filtered_pipeline[handle].replace_filename) << "; ";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}

}

void log_shader_code_readed(wchar_t filename[], std::vector<std::vector<uint8_t>>& shaderCode) {
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "load_shader_code() - Shader " << to_string(filename)  << " readed, size = " << (void*)shaderCode.size() << ")";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_shader_code_error(pipeline pipelineHandle, uint32_t hash, wchar_t filename[])
{

	std::stringstream s;

	s << "onInitPipeline, pipelineHandle: " << (void*)pipelineHandle.handle << "), ";
	s << "hash to handle = " << std::hex << hash << " ;";
	s << "!!! Error in loading code for :" << to_string(filename) << "; !!!";

	reshade::log::message(reshade::log::level::error, s.str().c_str());
	s.str("");
	s.clear();
}

void log_cloning_pipeline(reshade::api::pipeline pipeline, reshade::api::pipeline_layout layout, Shader_Definition* newShader, uint32_t subobjectCount) {

	if (g_shared_state->debug)
	{
		
		std::stringstream s;

		//log beginning of copy
		s << "CLONING PIPELINE("
			<< reinterpret_cast<void*>(pipeline.handle)
			<< ") Layout : " << reinterpret_cast<void*>(layout.handle)
			<< ", Hash = " << std::hex << newShader->hash << ";"
			<< ", subobjects counts: " << (subobjectCount)
			<< " )";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
		s.str("");
		s.clear();
	}
}

void  log_pipeline_clone_OK(uint64_t orig_handle, uint64_t cloned_handle) {

	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "pipeline  cloned  ("
			<< ", orig pipeline handle: " << std::hex << orig_handle
			<< ", cloned pipeline handle: " << std::hex << cloned_handle
			<< ")";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
		s.str("");
		s.clear();
	}
}

void  log_pipeline_clone_error(uint64_t orig_handle) {

		std::stringstream s;
		s << "!!! Error in cloning pipeline !!! ("
			<< ", orig pipeline: " << reinterpret_cast<void*>(orig_handle)
			<< ")";
		reshade::log::message(reshade::log::level::error, s.str().c_str());

}


void log_saved_pipelines_value(save_pipeline saved_pipeline) {
	if (g_shared_state->debug)
	{
		uint32_t hash;
		std::stringstream s;
		s << "addon - saved pipeline : handle = " << std::hex << saved_pipeline.pipeline.handle << ", subobject_count = " << saved_pipeline.subobject_count << "; ";
		// reshade::log::message(reshade::log::level::info, s.str().c_str());
		// s.str("");
		// s.clear();

		// display sub_objects
		for (uint32_t i = 0; i < saved_pipeline.subobject_count; i++) {
			hash = 0;
			const auto& sub = saved_pipeline.subobjects[i];

			switch (sub.type) {
				case reshade::api::pipeline_subobject_type::vertex_shader: {
					hash = saved_pipeline.vs_hash;
					break;
				}

				case reshade::api::pipeline_subobject_type::pixel_shader: {
					hash = saved_pipeline.ps_hash;
					break;
				}
			}
			s << "subobject[" << i << "] : hash = " << std::hex << hash << ", type = " << to_string(sub.type) << "; ";
			reshade::log::message(reshade::log::level::info, s.str().c_str());
			s.str("");
			s.clear();
		}
	}
}

void log_error_code_for_hash() {
	reshade::log::message(reshade::log::level::error, "VREM addon : can not get code for computing shader hash");
}


/*
void shader_by_hash() {
	if (g_shared_state->debug)
	{

		//filtered_pipeline
		for (const auto& [hash, shader] : shader_by_hash) {
			std::stringstream s;

			s << "on_init_pipeline : added filtered pipeline, handle = " << std::hex << handle << ", hash = " << std::hex << filtered_pipeline[handle].hash;
			s << ", action = " << to_string(filtered_pipeline[handle].action) << ", feature = " << to_string(filtered_pipeline[handle].feature);
			s << ", replace_filename = " << to_string(filtered_pipeline[handle].replace_filename) << "; ";
			reshade::log::message(reshade::log::level::info, s.str().c_str());
		}
	}
}
*/
