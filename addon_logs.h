///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
// addon logs function reference 
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
#include "loader_addon_shared.h"
#include "addon_objects.h"

using namespace reshade::api;

extern void log_addon_init();
extern void log_addon_cleanup_cloned();
extern void log_addon_cleanup_filtered();
extern void log_error_array_uniform(std::string effect_name, std::string uniform_name, uint32_t out_array_length);
extern void log_uniform(std::string effect_name, std::string uniform_name, float uniform_value);
extern void log_effect_reloaded();
extern void log_pipeline_init(PipeLine_Definition pipeline);
extern void log_filtered_added(uint64_t handle);
extern void log_shader_code_readed(const wchar_t filename[], uint32_t hash, size_t code_size);
extern void log_shader_code_error(const wchar_t filename[], uint32_t hash);
extern void log_cloning_pipeline(reshade::api::pipeline pipeline, reshade::api::pipeline_layout layout, Shader_Definition* newShader, uint32_t subobjectCount);
extern void log_pipeline_clone_OK(uint64_t orig_handle, uint64_t cloned_handle);
extern void log_pipeline_clone_error(uint64_t orig_handle);
extern void log_saved_pipelines_value(save_pipeline saved_pipeline);
extern void log_error_code_for_hash();
extern void log_shader(reshade::api::pipeline pipeline, Shader_Definition shader_def, bool status);

// extern void log_shader_by_hash();