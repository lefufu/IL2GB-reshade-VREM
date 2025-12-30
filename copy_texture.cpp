///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
//  clone pipeline and add reference of the cloned pipeline in the original 
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
/// copy_depthStencil()
/// <summary>
///  create needed resource, then copy existing resource into the new one, then create the new associated resource views
/// </summary>
bool copy_depthStencil2(command_list* cmd_list, shader_stage stages, pipeline_layout layout, uint32_t param_index, const descriptor_table_update& update)
{
	
	device* dev = cmd_list->get_device();

	// get resource info (depth = t3)
	a_shared.src_resource_view_depth = static_cast<const reshade::api::resource_view*>(update.descriptors)[3];
	a_shared.src_resource_view_stencil = static_cast<const reshade::api::resource_view*>(update.descriptors)[4];
	resource scr_resource = dev->get_resource_from_view(a_shared.src_resource_view_depth);
	resource_desc src_resource_desc = dev->get_resource_desc(scr_resource);

	log_creation_start("depthStencil");

	if (a_shared.depthStencil_resource.handle != 0)
	{
		// destroy existing resource
		dev->destroy_resource(a_shared.depthStencil_resource);
		a_shared.depthStencil_resource = {};
	}

	bool status = dev->create_resource(src_resource_desc, nullptr, resource_usage::shader_resource, &a_shared.depthStencil_resource, nullptr);
	log_resource_view_created("depthStencil", dev, src_resource_desc, a_shared.depthStencil_resource.handle);

	if (!status)
	{
		//error when creating resource or resource view
		log_error_creating_view();
	}

	// if (!a_shared.depthStencil_res[a_shared.count_display].copied)
	{
		//put resources in good usage for copying
		cmd_list->barrier(scr_resource, resource_usage::shader_resource, resource_usage::copy_source);
		cmd_list->barrier(a_shared.depthStencil_res[a_shared.count_display].texresource, resource_usage::shader_resource, resource_usage::copy_dest);
		// do copy
		cmd_list->copy_resource(scr_resource, a_shared.depthStencil_res[a_shared.count_display].texresource);
		//restore usage
		cmd_list->barrier(scr_resource, resource_usage::copy_source, resource_usage::shader_resource);
		cmd_list->barrier(a_shared.depthStencil_res[a_shared.count_display].texresource, resource_usage::copy_dest, resource_usage::shader_resource);

		// flag the first copy of texture to avoid usage of PS before texture copy (eg MFD)
		a_shared.depthStencil_copy_started = true;

		//flag texture copied to avoid double copy for MSAA because shader is called multiple time for a same "eye rendering" and only first call has good texture
		a_shared.depthStencil_res[a_shared.count_display].copied = true;

		//log copy done
		log_copy_texture("depthStencil");

	}



	return true;
}


// *******************************************************************************************************
/// copy_depthStencil()
/// <summary>
///  create needed resource, then copy existing resource into the new one, then create the new associated resource views
/// </summary>
bool copy_depthStencil(command_list* cmd_list, shader_stage stages, pipeline_layout layout, uint32_t param_index, const descriptor_table_update& update)
{
	// get resource info (depth = t3)
	resource_view src_resource_view_depth = static_cast<const reshade::api::resource_view*>(update.descriptors)[3];
	device* dev = cmd_list->get_device();
	resource scr_resource = dev->get_resource_from_view(src_resource_view_depth);


	// to do once per draw number and per game session: if resource and view not created, create them
	if (!a_shared.depthStencil_res[a_shared.count_display].created)
	{

		//get additional infos (stencil = t4)
		resource_view src_resource_view_stencil = static_cast<const reshade::api::resource_view*>(update.descriptors)[4];
		resource_desc src_resource_desc = dev->get_resource_desc(scr_resource);

		//log start of creation process
		log_creation_start("depthStencil");

		// not working : try to optimize things by avoiding texture copy => only &shared_data.stencil_view[shared_data.count_display].texresource_view are updated
		// shared_data.stencil_view[shared_data.count_display].texresource_view = static_cast<const reshade::api::resource_view*>(update.descriptors)[4];		

		// create the target resource for texture
		bool status1 = dev->create_resource(src_resource_desc, nullptr, resource_usage::shader_resource, &a_shared.depthStencil_res[a_shared.count_display].texresource, nullptr);

		// flag creation to avoid to create it again 
		a_shared.depthStencil_res[a_shared.count_display].created = true;

		// create the resources view
		resource_view_desc resview_desc_depth = dev->get_resource_view_desc(src_resource_view_depth);
		bool status2 = dev->create_resource_view(a_shared.depthStencil_res[a_shared.count_display].texresource, resource_usage::shader_resource, resview_desc_depth, &a_shared.depth_view[a_shared.count_display].texresource_view);
		a_shared.depth_view[a_shared.count_display].created = true;
		resource_view_desc resview_desc_stencil = dev->get_resource_view_desc(src_resource_view_stencil);
		bool status3 = dev->create_resource_view(a_shared.depthStencil_res[a_shared.count_display].texresource, resource_usage::shader_resource, resview_desc_stencil, &a_shared.stencil_view[a_shared.count_display].texresource_view);

		if (!status1 || !status2 || !status3)
		{
			//error when creating resource or resource view
			log_error_creating_view();
		}
		else
		{
			// continue
			a_shared.stencil_view[a_shared.count_display].created = true;

			// setup the descriptor table
			a_shared.update.binding = 0; // t3 as 3 is defined in pipeline_layout
			a_shared.update.count = 1;
			a_shared.update.type = reshade::api::descriptor_type::shader_resource_view;

			//log infos and check resource view created
			resource_desc check_new_res = dev->get_resource_desc(a_shared.depthStencil_res[a_shared.count_display].texresource);
			log_resource_view_created("depthStencil", dev, check_new_res, a_shared.stencil_view[a_shared.count_display].texresource_view.handle);

			// store resolution of resource of first draw to compute protential undersampling/supersampling factor (not for MSAA)
			if (a_shared.count_display == 0)
			{
				a_shared.renderTargetX = check_new_res.texture.width;
			}
		}
	}
	


	// to do before binding of global illum shader in associated push_descriptor: copy current resource to new resource for later usage in another shader
	// do it once per frame
	if (!a_shared.depthStencil_res[a_shared.count_display].copied)
	{
		//put resources in good usage for copying
		cmd_list->barrier(scr_resource, resource_usage::shader_resource, resource_usage::copy_source);
		cmd_list->barrier(a_shared.depthStencil_res[a_shared.count_display].texresource, resource_usage::shader_resource, resource_usage::copy_dest);
		// do copy
		cmd_list->copy_resource(scr_resource, a_shared.depthStencil_res[a_shared.count_display].texresource);
		//restore usage
		cmd_list->barrier(scr_resource, resource_usage::copy_source, resource_usage::shader_resource);
		cmd_list->barrier(a_shared.depthStencil_res[a_shared.count_display].texresource, resource_usage::copy_dest, resource_usage::shader_resource);

		// flag the first copy of texture to avoid usage of PS before texture copy (eg MFD)
		a_shared.depthStencil_copy_started = true;

		//flag texture copied to avoid double copy for MSAA because shader is called multiple time for a same "eye rendering" and only first call has good texture
		a_shared.depthStencil_res[a_shared.count_display].copied = true;

		//log copy done
		log_copy_texture("depthStencil");

	}


	return true;
}

// *******************************************************************************************************
/// copy_NS430_text()
/// 
/// TODO : make a generic copy process to avoid duplicate function for each resources
/// <summary>
///  create needed resource, then copy existing resource into the new one, then create the new associated resource views
/// </summary>
/// 

/*
bool copy_NS430_text(command_list* cmd_list, shader_stage stages, pipeline_layout layout, uint32_t param_index, const descriptor_table_update& update)
{
	// get resource info (t0)
	resource_view src_resource_view_NS430 = static_cast<const reshade::api::resource_view*>(update.descriptors)[0];
	device* dev = cmd_list->get_device();
	resource scr_resource = dev->get_resource_from_view(src_resource_view_NS430);

	// to do once per draw number : if resource and view not created, create them
	if (!shared_data.NS430_res[shared_data.count_display].created)
	{

		//get additional infos
		resource_desc src_resource_desc = dev->get_resource_desc(scr_resource);

		//log start of creation process
		log_creation_start("NS430");

		// create the target resource for texture
		dev->create_resource(src_resource_desc, nullptr, resource_usage::shader_resource, &shared_data.NS430_res[shared_data.count_display].texresource, nullptr);
		// flag creation to avoid to create it again 
		shared_data.NS430_res[shared_data.count_display].created = true;

		// create the resources view
		resource_view_desc resview_NS430 = dev->get_resource_view_desc(src_resource_view_NS430);
		dev->create_resource_view(shared_data.NS430_res[shared_data.count_display].texresource, resource_usage::shader_resource, resview_NS430, &shared_data.NS430_view[shared_data.count_display].texresource_view);
		shared_data.NS430_view[shared_data.count_display].created = true;

		// setup the descriptor table (same for all texture)
		shared_data.update.binding = 0; // t3 as 3 is defined in pipeline_layout
		shared_data.update.count = 1;
		shared_data.update.type = reshade::api::descriptor_type::shader_resource_view;

		//log infos and check resource view created
		resource_desc check_new_res = dev->get_resource_desc(shared_data.NS430_res[shared_data.count_display].texresource);
		log_resource_created("NS430", dev, check_new_res);
	}

	// to do before binding of global illum shader in associated push_descriptor: copy current resource to new resource for later usage in another shader
	// do it once per frame pre draw number
	if (!shared_data.NS430_res[shared_data.count_display].copied)
	{
		//put resources in good usage for copying
		cmd_list->barrier(scr_resource, resource_usage::shader_resource, resource_usage::copy_source);
		cmd_list->barrier(shared_data.NS430_res[shared_data.count_display].texresource, resource_usage::shader_resource, resource_usage::copy_dest);
		// do copy
		cmd_list->copy_resource(scr_resource, shared_data.NS430_res[shared_data.count_display].texresource);
		//restore usage
		cmd_list->barrier(scr_resource, resource_usage::copy_source, resource_usage::shader_resource);
		cmd_list->barrier(shared_data.NS430_res[shared_data.count_display].texresource, resource_usage::copy_dest, resource_usage::shader_resource);

		// flag the first copy of texture to avoid usage of PS before texture copy (eg MFD)
		shared_data.NS430_copy_started = true;

		// flag texture copied to avoid double copy for MSAA because shader is called multiple time for a same "eye rendering" and only first call has good texture
		shared_data.NS430_res[shared_data.count_display].copied = true;

		//log copy done
		log_copy_texture("NS430");

	}


	return true;
}
*/
// *******************************************************************************************************
void delete_texture_resources(device* device)
{ 
	//delete resource and resource view if created (done here to get a reshade entry point for destroy*)
	if (a_shared.depthStencil_res[0].created)
	{
		for (int i = 0; i < MAXVIEWSPERDRAW; i++)
		{
			device->destroy_resource(a_shared.depthStencil_res[i].texresource);
			a_shared.depthStencil_res[i].created = false;
			device->destroy_resource_view(a_shared.depth_view[i].texresource_view);
			a_shared.depth_view[i].created = false;
			device->destroy_resource_view(a_shared.stencil_view[i].texresource_view);
			a_shared.stencil_view[i].created = false;
		}
	}
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