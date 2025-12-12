///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
//  references of functions of VREM mod, including messages
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

#pragma once

#include <reshade.hpp>
#include "addon_objects.h"
#include <optional>

extern void get_settings_from_uniforms(reshade::api::effect_runtime* runtime);
// extern uint32_t calculateShaderHash(void* shaderData);
extern uint32_t calculateShaderHash(const reshade::api::shader_desc& desc);
extern std::optional<Shader_Definition> is_in_mod_hash(uint32_t hash);
extern bool load_shader_code(std::vector<std::vector<uint8_t>>& shaderCode, wchar_t filename[]);
extern void clone_pipeline(reshade::api::device* device, reshade::api::pipeline_layout layout, uint32_t subobjectCount, const reshade::api::pipeline_subobject* subobjects, reshade::api::pipeline pipeline, std::vector<std::vector<uint8_t>>& ReplaceshaderCode, Shader_Definition* newShader);
extern void save_pipeline_in_list(reshade::api::device* device, reshade::api::pipeline_layout layout, uint32_t subobject_count, const reshade::api::pipeline_subobject* subobjects, reshade::api::pipeline pipeline);
extern save_pipeline* find__pipeline_per_handle(uint64_t handle);
extern std::vector<save_pipeline*> find__pipelines_per_hash(uint32_t hash);
extern size_t number_of_saved_pipeline();
extern void delete_saved_pipeline(save_pipeline& p);
extern void delete_all_saved_pipelines();


