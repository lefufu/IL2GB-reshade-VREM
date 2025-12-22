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
		reshade::log::message(reshade::log::level::info, "addon - initialization");
	}
}

void log_addon_cleanup_cloned() {
	if (g_shared_state->debug)
	{
		reshade::log::message(reshade::log::level::info, "addon - cleaning up cloned pipelines");
	}
}

void log_addon_cleanup_filtered() {
	if (g_shared_state->debug)
	{
		reshade::log::message(reshade::log::level::info, "addon - cleaning up filtered pipelines");
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
		reshade::log::message(reshade::log::level::info, "addon - effect reloaded => update settings from uniforms");
	}
}

void log_pipeline_init(PipeLine_Definition pipeline)
{
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "addon - on_init_pipeline : added pipeline, hash = " << std::hex << pipeline.hash << ", handle = " << std::hex << pipeline.handle << ", type = " << to_string(pipeline.type) << "; ";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_filtered_added(uint64_t handle) {
	if (g_shared_state->debug)
	{
		std::stringstream s;
		//filtered_pipeline

		s << "addon - added filtered pipeline, handle = " << std::hex << handle  << "; ";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}

}

void log_shader_code_readed(const wchar_t filename[], uint32_t hash, size_t code_size) {
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "addon - load_shader_code() - Shader " << to_string(filename)  << ", hash =" << std::hex << hash  << ", size = " << code_size << ")";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_shader_code_error(const wchar_t filename[], uint32_t hash)
{

	std::stringstream s;
	s << "ERROR : addon - load_shader_code() - Shader " << to_string(filename) << ", hash =" << std::hex << hash << " file not found";
	reshade::log::message(reshade::log::level::error, s.str().c_str());
}

void log_cloning_pipeline(reshade::api::pipeline pipeline, reshade::api::pipeline_layout layout, Shader_Definition* newShader, uint32_t subobjectCount) {

	if (g_shared_state->debug)
	{
		
		std::stringstream s;

		//log beginning of copy
		s << "addon - cloning pipeline ("
			<< std::hex << pipeline.handle
			<< ") Layout : " << std::hex << layout.handle
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
		s << "addon - pipeline  cloned"
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
		s << "ERROR : addon - error in cloning pipeline ("
			<< ", orig pipeline: " << std::hex << orig_handle
			<< ")";
		reshade::log::message(reshade::log::level::error, s.str().c_str());

}


void log_saved_pipelines_value(save_pipeline saved_pipeline) {
	if (g_shared_state->debug)
	{
		//uint32_t hash;
		std::stringstream s;
		s << "addon - saved pipeline : handle = " << std::hex << saved_pipeline.pipeline.handle << ", subobject_count = " << saved_pipeline.subobject_count << "; ";
		// reshade::log::message(reshade::log::level::info, s.str().c_str());
		// s.str("");
		// s.clear();

		// display sub_objects
		for (uint32_t i = 0; i < saved_pipeline.subobject_count; i++) {
			//hash = 0;
			const auto& sub = saved_pipeline.subobjects[i];

			s << "subobject[" << i << "] : hash = " << std::hex << saved_pipeline.hash[i] << ", type = " << to_string(sub.type) << "; ";
			//reshade::log::message(reshade::log::level::info, s.str().c_str());
			//s.str("");
			//s.clear();
		}
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_error_code_for_hash() {
	reshade::log::message(reshade::log::level::error, "ERROR : addon - can not get code for computing shader hash");
}

void log_shader(reshade::api::pipeline pipeline, Shader_Definition shader_def, bool status) {
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "addon - shader identified in pipeline: " << std::hex << pipeline.handle << ", " << to_string(shader_def) << ", active : " << status << "; ";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_device_null() {
	reshade::log::message(reshade::log::level::error, "ERROR : addon - device is null");
}


void log_delete_cloned_pipeline(uint64_t handle) {
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "addon - delete cloned pipeline : " << std::hex << handle << "; ";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_cleanup_shader_code(){

	if (g_shared_state->debug)
	{
		reshade::log::message(reshade::log::level::info, "addon - cleanup shader code cache");
	}
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
