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

void log_end_capture_frame() {
	if (g_shared_state->debug)
	{
		reshade::log::message(reshade::log::level::info, "addon - onpresent :  -- End Frame -- ;");
	}
}

void log_start_capture_frame() {
	if (g_shared_state->debug)
	{
		reshade::log::message(reshade::log::level::info, "addon - onpresent :  -- Start Frame -- ;");
	}
}

void log_shader_binded(uint64_t handle, Shader_Definition shader_def) {
	if (g_shared_state->debug && flag_capture)
	{
		std::stringstream s;
		s << "addon - shader binded : " << std::hex << handle << ", shader :" << to_string(shader_def) << "; ";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_pipeline_replaced(uint64_t pipelineHandle, uint64_t cloned) {

	if (g_shared_state->debug && flag_capture)
	{
		std::stringstream s;
		s << "addon - bind pipeline, replaced pipeline : " << std::hex << pipelineHandle << ", by cloned pipeline :" << std::hex << cloned << "; ";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_init_pipeline_layout(const uint32_t paramCount, const reshade::api::pipeline_layout_param* params, reshade::api::pipeline_layout layout)
{
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << " addon - init_pipeline_layout  = " << std::hex << layout.handle << ";";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_init_pipeline_params(const uint32_t paramCount, const reshade::api::pipeline_layout_param* params, reshade::api::pipeline_layout layout, uint32_t paramIndex, reshade::api::pipeline_layout_param param)
{
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "    looping on  paramCount : param = " << to_string(paramIndex) << ", param.type = " << to_string(param.type) << ", param.push_descriptors.type = " << to_string(param.push_descriptors.type);
		reshade::log::message(reshade::log::level::info, s.str().c_str());
		s.str("");
		s.clear();
	}
}

void log_create_CBlayout(std::string CBName, int CB_number)
{
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "addon - new pipeline layout created, hash =" << reinterpret_cast<void*>(&a_shared.saved_pipeline_layout_CB[CB_number].handle) << " ).  for CB " << CBName << " injection, ";
		s << "  dx_register_index=" << CBINDEX << "; ";
		reshade::log::message(reshade::log::level::error, s.str().c_str());
	}
}


void log_error_creating_CBlayout(std::string CBName, int CB_number)
{
	std::stringstream s;

	s << "addon - pipeline_layout(" << reinterpret_cast<void*>(&a_shared.saved_pipeline_layout_CB[CB_number].handle) << " ). !!! Error in creating DX11 layout for CB " << CBName << "!!!;";
	reshade::log::message(reshade::log::level::error, s.str().c_str());
	s.str("");
	s.clear();
}

void log_CB_injected(std::string CBName)
{
	if (g_shared_state->debug && flag_capture)
	{
		std::stringstream s;
		s << " -> on_bind_pipeline: CB injected :" << CBName << " ;";
		s << "  a_shared.cb_inject_values.testFlag = " << a_shared.cb_inject_values.testFlag << ";";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}


void log_destroy_CBlayout(uint64_t layout_handle)

{
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "addon - destroy pipeline layout for CB injection,  hash =" << std::hex << layout_handle  <<  ";";
		reshade::log::message(reshade::log::level::error, s.str().c_str());
	}
}

void log_increase_count_display()
{
	if (g_shared_state->debug && flag_capture)
	{
		std::stringstream s;
		s << "addon - on_bind_pipeline : update draw count : " << a_shared.count_display << ", mapMode = " << a_shared.cb_inject_values.mapMode << ";";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_not_increase_draw_count()
{
	if (g_shared_state->debug && flag_capture)
	{
		std::stringstream s;
		s << "addon - on_bind_pipeline : depthstencil not copied => do not update draw count, texture not copied;";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_start_monitor(std::string texture_name)
{
	if (g_shared_state->debug && flag_capture)
	{
		std::stringstream s;
		s << "addon - on_bind_pipeline : start monitor " << texture_name << ", draw : " << a_shared.count_display << "; ";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_mirror_view()
{

	if (g_shared_state->debug && flag_capture)
	{
		std::stringstream s;
		s << "addon - on_bind_pipeline() : count_display used for mirror view = " << a_shared.count_display << ", mirror VR = " << a_shared.mirror_VR << "; ";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}

}

void log_push_descriptor(shader_stage stages, pipeline_layout layout, uint32_t param_index, const descriptor_table_update& update)
{
	if (g_shared_state->debug && flag_capture)
	{
		std::stringstream s;
		s << "on_push_descriptors(" << to_string(stages) << ", " << (void*)layout.handle << ", " << param_index << ", { " << to_string(update.type) << ", " << update.binding << ", " << update.count << " })";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
		s.str("");
		s.clear();

		if (update.type == descriptor_type::shader_resource_view)
		{
			// add info on textures hash
			for (uint32_t i = 0; i < update.count; ++i)
			{
				auto item = static_cast<const reshade::api::resource_view*>(update.descriptors)[i];
				s << "=> on_push_descriptors(), resource_view[" << i << "],  handle = " << reinterpret_cast<void*>(item.handle) << " })";
				reshade::log::message(reshade::log::level::info, s.str().c_str());
				s.str("");
				s.clear();
			}
		}
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_creation_start(std::string texture_name)
{

	// if (g_shared_state->debug)
	if (g_shared_state->debug && flag_capture)
	{
		std::stringstream s;
		s << " create resources and resource views to copy " << texture_name << ", count_display = " << a_shared.count_display << ";";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}


void log_resource_view_created(std::string texture_name, device* dev, resource_desc check_new_res, uint64_t handle)
{

	// if ((debug_flag && flag_capture) || FORCE_LOG)
	// if (g_shared_state->debug)
	if (g_shared_state->debug && flag_capture)
	{

		// display resource info

		std::stringstream s;
		// s << " => copy_depthStencil: for draw " << shared_data.count_display << ": create stencil/depth resource, type: " << to_string(check_new_res.type);
		s << " => copy_" << texture_name << ": for draw " << a_shared.count_display << ": create stencil/depth resource view, type: " << to_string(check_new_res.type) << ", handle =" << std::hex << handle;

		switch (check_new_res.type) {
		default:
		case reshade::api::resource_type::unknown:
			s << "!!! error: resource_type not texture* !!!";
			break;

		case reshade::api::resource_type::texture_1d:
		case reshade::api::resource_type::texture_2d:
		case reshade::api::resource_type::texture_3d:
			s << ", texture format: " << to_string(check_new_res.texture.format);
			s << ", texture width: " << check_new_res.texture.width;
			s << ", texture height: " << check_new_res.texture.height;
			s << ", texture depth: " << check_new_res.texture.depth_or_layers;
			s << ", texture samples: " << check_new_res.texture.samples;
			s << ", texture levels: " << check_new_res.texture.levels;
			s << ", usage: " << to_string(check_new_res.usage);
			break;
		}
		s << ";";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
		s.str("");
		s.clear();

		//display resource view infos
		resource_view_desc check_new_res2 = dev->get_resource_view_desc(a_shared.depth_view[a_shared.count_display].texresource_view);
		// resource_view_desc check_new_res2 = dev->get_resource_view_desc(a_shared.src_resource_view_depth);
		s << " => copy_" << texture_name << ": create depth resource view , type: " << to_string(check_new_res2.type);

		switch (check_new_res2.type) {
		default:
		case reshade::api::resource_view_type::unknown:
			s << "!!! error: resource_type not texture* !!!";
			break;

		case reshade::api::resource_view_type::texture_1d:
		case reshade::api::resource_view_type::texture_2d:
		case reshade::api::resource_view_type::texture_3d:
		case reshade::api::resource_view_type::texture_2d_multisample:
			s << ", view format: " << to_string(check_new_res2.format);
			break;
		}
		s << ";";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
		s.str("");
		s.clear();

		// check_new_res2 = dev->get_resource_view_desc(a_shared.stencil_view[a_shared.count_display].texresource_view);
		//check_new_res2 = dev->get_resource_view_desc(a_shared.src_resource_view_stencil);
		s << " => copy_depthStencil: create depth resource view , type: " << to_string(check_new_res2.type);
		s << " => copy_" << texture_name << ": create stencil resource view , type: " << to_string(check_new_res2.type);

		switch (check_new_res2.type) {
		default:
		case reshade::api::resource_view_type::unknown:
			s << "!!! error: resource_type not texture* !!!";
			break;

		case reshade::api::resource_view_type::texture_1d:
		case reshade::api::resource_view_type::texture_2d:
		case reshade::api::resource_view_type::texture_3d:
		case reshade::api::resource_view_type::texture_2d_multisample:
			s << ", view format: " << to_string(check_new_res2.format);
			break;
		}
		s << ";";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
		s.str("");
		s.clear();

	}
}

void log_copy_texture(std::string texture_name)
{
	// if (debug_flag && shared_data.s_do_capture)
	if (g_shared_state->debug && flag_capture)
	{
		std::stringstream s;
		s << " = > copy_" << texture_name << ": for draw (" << a_shared.count_display << ") : resource copied";
		reshade::log::message(reshade::log::level::info, s.str().c_str());

	}
}

void log_texture_injected(std::string texture_name, int count_displayVS)
{
	if (g_shared_state->debug && flag_capture)
	{
		std::stringstream s;
		s << " => on_bind_pipeline : " << texture_name << " textures injected for draw index : ";
		s << count_displayVS << ";";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
	}
}

void log_error_creating_view()
{

	reshade::log::message(reshade::log::level::error, "Error when creating resources or resources view");
}

void log_create_RVlayout()
{
	if (g_shared_state->debug)
	{
		std::stringstream s;
		s << "on_init_pipeline_layout: new pipeline created, hash =" << reinterpret_cast<void*>(&a_shared.saved_pipeline_layout_RV.handle) << " ).  DX11 layout created for RV;";
		s << "dx_register_index=" << RVINDEX << "; ";
		reshade::log::message(reshade::log::level::warning, s.str().c_str());
	}
}

void log_error_creating_RVlayout()
{
	std::stringstream s;
	s << "on_init_pipeline_layout(" << reinterpret_cast<void*>(&a_shared.saved_pipeline_layout_RV.handle) << " ). !!! Error in creating DX11 layout for RV !!!;";
	reshade::log::message(reshade::log::level::error, s.str().c_str());
}

void log_reset_tracking()
{
	if (g_shared_state->debug && flag_capture)
	{
		reshade::log::message(reshade::log::level::info, " -> on_draw*: All tracking resetted");
	}
}

void log_ondraw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
{
	if ((g_shared_state->debug && flag_capture && (a_shared.track_for_depthStencil || a_shared.track_for_NS430 || a_shared.render_effect)) )
	{
		std::stringstream s;
		s << "draw(" << vertex_count << ", " << instance_count << ", " << first_vertex << ", " << first_instance << ")";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
		s.str("");
		s.clear();
		reshade::log::message(reshade::log::level::info, "Clear tracking flags");
		s.str("");
		s.clear();

		if (a_shared.render_effect)
		{

		}
	}
}

void log_on_draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
{
	if ((g_shared_state->debug && flag_capture && (a_shared.track_for_depthStencil || a_shared.track_for_NS430 || a_shared.render_effect)))
	{
		std::stringstream s;
		s << "draw_indexed(" << index_count << ", " << instance_count << ", " << first_index << ", " << vertex_offset << ", " << first_instance << ")";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
		s.str("");
		s.clear();
		reshade::log::message(reshade::log::level::info, "Clear tracking flags");
	}
}

void log_on_drawOrDispatch_indirect(indirect_command type, resource buffer, uint64_t offset, uint32_t draw_count, uint32_t stride)
{
	if ((g_shared_state->debug && flag_capture && (a_shared.track_for_depthStencil || a_shared.track_for_NS430 || a_shared.render_effect)))
	{
		std::stringstream s;
		s << "draw_indexed_indirect(" << (void*)buffer.handle << ", " << offset << ", " << draw_count << ", " << stride << ")";
		reshade::log::message(reshade::log::level::info, s.str().c_str());
		s.str("");
		s.clear();
		reshade::log::message(reshade::log::level::info, "Clear tracking flags");
	}
}

void log_display_settings()
{
	if (g_shared_state->debug) 
	{
		std::stringstream s;
		reshade::log::message(reshade::log::level::info, "addon - options values :");
		s.str("");
		s.clear();
		for (size_t i = 0; i < SETTINGS_SIZE; i++)
		{
			//if (a_shared.VREM_setting[option])
			{
				s << " a_shared.VREM_setting[" << i << "] = " << a_shared.VREM_setting[i];
				reshade::log::message(reshade::log::level::info, s.str().c_str());
				s.str("");
				s.clear();
			}
		}
	}
}

void log_waiting_setting()
{
	if (g_shared_state->debug)
	{
		reshade::log::message(reshade::log::level::info, "addon - waiting uniforms available");
	}	
}