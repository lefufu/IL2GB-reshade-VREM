///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
//  all functions regarding constant buffer injection and handling
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
#include <string>

#include "addon_logs.h"

using namespace reshade::api;


// *******************************************************************************************************
///  create a CB layout for CB created or modified by VREM 

void create_modified_CB_layout(reshade::api::device* device, int cbindex, std::string CB_name, int layout_number)
{
	// create a new pipeline_layout for VREM constant buffer to be updated by push_constant(), cb number defined in CBINDEX (in mod_injection.h)
	// pipeline_layout_param
	// uint32_t 	binding 			OpenGL uniform buffer binding index. 
	// uint32_t 	dx_register_index	D3D10/D3D11/D3D12 constant buffer register index. 
	// uint32_t 	dx_register_space	D3D12 constant buffer register space. 
	// uint32_t 	count				Number of constants in this range (in 32-bit values). 
	// shader_stage visibility			Shader pipeline stages that can make use of the constants in this range. 
	/*
	reshade::api::pipeline_layout_param newParams;
	newParams.type = reshade::api::pipeline_layout_param_type::push_constants;
	newParams.push_constants.binding = 0;
	newParams.push_constants.count = 1;
	newParams.push_constants.dx_register_index = cbindex;
	newParams.push_constants.dx_register_space = 0;
	newParams.push_constants.visibility = reshade::api::shader_stage::all;

	//put the VREM parameters in CB
	bool  result = device->create_pipeline_layout(1, &newParams, &a_shared.saved_pipeline_layout_CB[layout_number]);
	//logs
	if (result) log_create_CBlayout(CB_name, layout_number);
	else log_error_creating_CBlayout(CB_name, layout_number);
	*/

	if (a_shared.saved_pipeline_layout_CB[layout_number].handle == 0)
	{

		reshade::api::pipeline_layout_param newParams;
		newParams.type = reshade::api::pipeline_layout_param_type::push_constants;
		newParams.push_constants.binding = 0;
		newParams.push_constants.count = CBSIZE;
		newParams.push_constants.dx_register_index = cbindex;
		newParams.push_constants.dx_register_space = 0;
		newParams.push_constants.visibility = reshade::api::shader_stage::all;

		bool result = device->create_pipeline_layout(1, &newParams, &a_shared.saved_pipeline_layout_CB[layout_number]);

		if (result)
			log_create_CBlayout(CB_name, layout_number);
		else
			log_error_creating_CBlayout(CB_name, layout_number);
	}

}

// *******************************************************************************************************
///  create all CB layout for CB created or modified by VREM 
void create_all_modified_CB_layout(reshade::api::device* device)
{
	create_modified_CB_layout(device, CBINDEX, "VREM settings Cbuffer", SETTINGS_CB_NB);
}

// *******************************************************************************************************
/// delete_CB_layout()
/// delete the previoulsy created CB layout, called at each unload of the VREM dll

void delete_modified_CB_layout(reshade::api::device* device, int cbindex, std::string CB_name, int layout_number)
{
	
	for (int i = 0; i < NUMBER_OF_MODIFIED_CB; i++)
	{
		if (a_shared.saved_pipeline_layout_CB[i].handle != 0)
		{
			log_destroy_CBlayout(a_shared.saved_pipeline_layout_CB[i].handle);
			device->destroy_pipeline_layout(a_shared.saved_pipeline_layout_CB[i]);
			a_shared.saved_pipeline_layout_CB[i] = { 0 }; 

		}
	}
}