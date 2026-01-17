///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
//  on_create_pipeline : save shader code
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

#include "loader_addon_shared.h"
#include "addon_functions.h"
#include "addon_objects.h"
#include "VREM_settings.h"
#include "addon_logs.h"


using namespace reshade::api;

extern "C" {

	// *******************************************************************************************************
	/// create_CB_layout()
	///  create a CB layout for CB created or modified by VREM 


	//*******************************************************************************
	__declspec(dllexport) bool vrem_on_create_pipeline(device* device, pipeline_layout, uint32_t subobject_count, const pipeline_subobject* subobjects) {

		
		if (!g_shared_state->debug) return false;
			
		const device_api device_type = device->get_api();

		for (uint32_t i = 0; i < subobject_count; ++i)
		{
			
			const auto& sub = subobjects[i];
			if (ALLOWED_SHADERS.count(sub.type) > 0) save_shader_code(device_type, *static_cast<const shader_desc*>(subobjects[i].data));
			
			/*
			switch (subobjects[i].type)
			{
			case pipeline_subobject_type::vertex_shader:
			//case pipeline_subobject_type::hull_shader:
			//case pipeline_subobject_type::domain_shader:
			//case pipeline_subobject_type::geometry_shader:
			case pipeline_subobject_type::pixel_shader:
			case pipeline_subobject_type::compute_shader:
			case pipeline_subobject_type::amplification_shader:
			case pipeline_subobject_type::mesh_shader:
			case pipeline_subobject_type::raygen_shader:
			case pipeline_subobject_type::any_hit_shader:
			case pipeline_subobject_type::closest_hit_shader:
			case pipeline_subobject_type::miss_shader:
			case pipeline_subobject_type::intersection_shader:
			case pipeline_subobject_type::callable_shader:
				save_shader_code(device_type, *static_cast<const shader_desc*>(subobjects[i].data));
				break;
			} */
		}

		return false;

	}
}
