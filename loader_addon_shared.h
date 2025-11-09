///////////////////////////////////////////////////////////////////////
//
// Reshade DCS VREM addon - Hot Reload Support
// 
// Structures partagées entre le loader et l'implementation
// 
///////////////////////////////////////////////////////////////////////

#pragma once

#include <unordered_map>
#include <chrono>
#include <reshade.hpp>
// #include "shader_definitions.h"

// Structure contenant toutes les variables partagées
struct SharedState {
    // int s_fps_limit = 0;
    float last_fps_limit = 0;
    std::chrono::high_resolution_clock::time_point s_last_time_point;
    bool overlay_is_open = false;
   //  bool was_open_last_frame = false;
   //  bool initialized = false;
};


// Structure pour les données qui survivent au rechargement
struct PersistentPipelineData {
    // Maps des pipelines (conservées entre rechargements)
    // std::unordered_map<uint32_t, Shader_Definition> pipeline_by_hash;
    // std::unordered_map<uint64_t, Shader_Definition> pipeline_by_handle;
};

// Signatures des fonctions exportées par l'addon implementation
typedef void (*VremInitFunc)(
    reshade::api::device*,
    reshade::api::command_queue*,
    reshade::api::swapchain*,
    PersistentPipelineData*,
    SharedState*
    );

typedef void (*VremCleanupFunc)(PersistentPipelineData*); 
