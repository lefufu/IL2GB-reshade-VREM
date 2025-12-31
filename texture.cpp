///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
//  function for texture management
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
#include <filesystem>
#include <reshade.hpp>

#include "to_string.hpp"
#include "addon_logs.h"
#include "addon_objects.h"

using namespace reshade::api;


// *******************************************************************************************************
/// create a new resource_view by copying the original view_desc but using the 
/// <summary>
/// 
resource_view copy_resource_view(device* dev,  resource_view src_resource_view, resource dst_resource)
{
	resource_view dst_resource_view = {};

	resource_view_desc src_view_desc = {};
	// get original view desc
	src_view_desc = dev->get_resource_view_desc(src_resource_view);

	//create the new resource view
	dev->create_resource_view(dst_resource, reshade::api::resource_usage::shader_resource, src_view_desc, &dst_resource_view);

	return dst_resource_view;
}


// *******************************************************************************************************
/// copy_depthStencil()
/// <summary>
///  create needed resource, then copy existing resource into the new one, then create the new associated resource views
/// </summary>
bool copy_depthStencil(command_list* cmd_list, shader_stage stages, pipeline_layout layout, uint32_t param_index, const descriptor_table_update& update)
{

	device* dev = cmd_list->get_device();

	// get resource info (depth = t3)
	reshade::api::resource_view src_resource_view_depth;
	src_resource_view_depth = static_cast<const reshade::api::resource_view*>(update.descriptors)[3];
	reshade::api::resource_view src_resource_view_stencil;
	src_resource_view_stencil = static_cast<const reshade::api::resource_view*>(update.descriptors)[4];

	resource scr_resource = dev->get_resource_from_view(src_resource_view_depth);
	resource_desc src_resource_desc = dev->get_resource_desc(scr_resource);

	// create target resource once per game session, for each source resource 
	bool resource_found = false;

	auto it = a_shared.saved_DS.find(scr_resource.handle);
	if (it == a_shared.saved_DS.end()) {

		//create the entry to host resource and resource views and store it in a_shared.saved_DS
		resource_DS_copy DS_copy = {};

		// create a new single ressource containing stencil and depth
		log_creation_start("depthStencil");

		bool status = dev->create_resource(src_resource_desc, nullptr, resource_usage::shader_resource, &DS_copy.texresource, nullptr);
		if (!status)
		{
			//error when creating resource or resource view
			log_error_creating_view();
		}
		else
		{
			// create resources view on the copied resource
			DS_copy.texresource_view_depth = copy_resource_view(dev, src_resource_view_depth, DS_copy.texresource);
			DS_copy.texresource_view_stencil = copy_resource_view(dev, src_resource_view_stencil, DS_copy.texresource);

			if (DS_copy.texresource_view_depth.handle != 0 && DS_copy.texresource_view_stencil.handle != 0)
			{
				// store new elements for copied resource
				a_shared.saved_DS.emplace(scr_resource.handle, DS_copy);
				resource_found = true;
				log_resource_created("depthStencil", dev, src_resource_desc, scr_resource.handle);
				log_resource_view_created("Depth",  dev, DS_copy.texresource_view_depth, scr_resource.handle);
				log_resource_view_created("Stencil", dev, DS_copy.texresource_view_stencil, scr_resource.handle);
			}
		}

	}
	else
	{
		resource_found = true;
	}

	if (resource_found && !a_shared.saved_DS[scr_resource.handle].copied)
	{
		
		//flag texture copied to avoid double copy for MSAA because shader is called multiple time for a same "eye rendering" and only first call has good texture
		a_shared.saved_DS[scr_resource.handle].copied = true;

		// copy resource 
		// put resources in good usage for copying
		cmd_list->barrier(scr_resource, resource_usage::shader_resource, resource_usage::copy_source);
		cmd_list->barrier(a_shared.saved_DS[scr_resource.handle].texresource, resource_usage::shader_resource, resource_usage::copy_dest);
		// do copy
		cmd_list->copy_resource(scr_resource, a_shared.saved_DS[scr_resource.handle].texresource);
		//restore usage
		cmd_list->barrier(scr_resource, resource_usage::copy_source, resource_usage::shader_resource);
		cmd_list->barrier(a_shared.saved_DS[scr_resource.handle].texresource, resource_usage::copy_dest, resource_usage::shader_resource);

		// to retrieve infos for pushing texture in bind_pipeline
		a_shared.current_DS_handle = scr_resource.handle;

		//log copy done
		log_copy_texture("depthStencil", a_shared.current_DS_handle);
	}
	return true;
}

// *******************************************************************************************************
void delete_texture_resources(device* device)
{ 
	//delete resource and resource view if created 

	for (auto& [handle, ds_copy] : a_shared.saved_DS) {
		device->destroy_resource_view(ds_copy.texresource_view_depth);
		device->destroy_resource_view(ds_copy.texresource_view_stencil);
		device->destroy_resource(ds_copy.texresource);
	}
	a_shared.saved_DS.clear();
}

// *******************************************************************************************************
// create_pipeline layout tu push texture resource view in shaders

void create_RV_pipeline_layout(device* device)
{

	if (a_shared.saved_pipeline_layout_RV.handle == 0)
	{
		// create a new pipeline_layout for just 1 rsource view to be updated by push_constant(), RV number defined in RVINDEX
		reshade::api::descriptor_range srv_range;
		srv_range.dx_register_index = RVINDEX;
		srv_range.count = UINT32_MAX;
		srv_range.visibility = reshade::api::shader_stage::pixel;
		srv_range.type = reshade::api::descriptor_type::shader_resource_view;

		const reshade::api::pipeline_layout_param params[] = {
			srv_range,
		};

		bool  result = device->create_pipeline_layout(std::size(params), params, &a_shared.saved_pipeline_layout_RV);

		if (result)  log_create_RVlayout();
		else log_error_creating_RVlayout();
	}
}