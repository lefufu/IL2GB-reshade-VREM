///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
// launcher and addon shared structures definition
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

#include <unordered_map>
#include <chrono>
#include <reshade.hpp>
#include <set>

// name of addon .dll
#define VREM_ADDON_NAME "IL2GB_VREM.dll"

#define MAX_OBJ_PER_PIPELINE 5

// handled pipeline types
static const std::set<reshade::api::pipeline_subobject_type> ALLOWED_SHADERS = {
    reshade::api::pipeline_subobject_type::vertex_shader,
    reshade::api::pipeline_subobject_type::pixel_shader
};


// Structure to store all pipeline infos
struct save_pipeline {
    reshade::api::device* device;
    reshade::api::pipeline_layout layout;
    reshade::api::pipeline pipeline;

    // subobjects copy
    std::vector<reshade::api::pipeline_subobject> subobjects;
    uint32_t subobject_count;

    // store data pointed by subobjects
    std::vector<uint8_t> vs_bytecode;
    std::vector<uint8_t> ps_bytecode;
    std::vector<uint8_t> gs_bytecode;
    std::vector<uint8_t> hs_bytecode;
    std::vector<uint8_t> ds_bytecode;

    reshade::api::shader_desc vs_desc;
    reshade::api::shader_desc ps_desc;
    reshade::api::shader_desc gs_desc;
    reshade::api::shader_desc hs_desc;
    reshade::api::shader_desc ds_desc;

    std::vector<reshade::api::input_element> input_elements;
    reshade::api::primitive_topology topology;
    reshade::api::rasterizer_desc rasterizer;
    reshade::api::blend_desc blend;
    reshade::api::depth_stencil_desc depth_stencil;
    std::vector<reshade::api::format> render_target_formats;
    reshade::api::format depth_stencil_format;

    // metadata for identification
    uint32_t vs_hash;
    uint32_t ps_hash;
    uint32_t hash[MAX_OBJ_PER_PIPELINE];
};


struct PipeLine_Definition {
    reshade::api::pipeline_subobject_type type;
    uint32_t hash;
    uint64_t handle;
};

// Structure for data that should be kept between mod reload
struct PersistentPipelineData {
    std::unordered_map<uint32_t, PipeLine_Definition> pipeline_by_hash;
    std::unordered_map<uint64_t, PipeLine_Definition> pipeline_by_handle;
    std::vector<save_pipeline> saved_pipelines;
};


// Structure to hold shared variables
struct SharedState {
    // for fps used to test the logic
    float last_fps_limit = 0;
    std::chrono::high_resolution_clock::time_point s_last_time_point;
    // to know when overlay is active and so update uniforms
    bool overlay_is_open = false;
    // debug flag
    bool debug = true;

    PersistentPipelineData VREM_pipelines;
};


