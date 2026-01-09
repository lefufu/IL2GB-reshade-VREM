
///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
//  on_init_pipeline_layout : called once per game session, initialize pipeline layout
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

	// *******************************************************************************************************
	/// create_CB_layout()
	///  create a CB layout for CB created or modified by VREM 
	

	//*******************************************************************************
	__declspec(dllexport) void vrem_on_init_pipeline_layout(device* dev, uint32_t paramCount, const pipeline_layout_param* params, pipeline_layout layout) {


		log_init_pipeline_layout(paramCount, params, layout);
		// generate data for constant_buffer or shader_resource_view
		// for (uint32_t paramIndex = 0; paramIndex < paramCount; ++paramIndex) {
			// auto param = params[paramIndex];
			// reshade::api::pipeline_layout_param param = params[paramIndex];


			//log infos
			// log_init_pipeline_params(paramCount, params, layout, paramIndex, param);
		//}
		/*
			if (param.push_descriptors.type == descriptor_type::constant_buffer)
			{

				// create pipeline layout for injecting VREM settings/parameters in CB CBINDEX
				create_modified_CB_layout(dev, CBINDEX, "VREM settings Cbuffer", SETTINGS_CB_NB);

				g_shared_state->DX11_layout = layout;

				// create pipeline layout for injecting CperFrame parameters in CB CPERFRAME_INDEX
				// create_modified_CB_layout(dev, CPERFRAME_INDEX, "CperFrame", CPERFRAME_CB_NB);
			}

			
			else if (param.push_descriptors.type == descriptor_type::shader_resource_view)
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

				bool  result = dev->create_pipeline_layout(std::size(params), params, &shared_data.saved_pipeline_layout_RV);

				if (result)  log_create_RVlayout();
				else log_error_creating_RVlayout();
			}
			*/
	
	}
}


