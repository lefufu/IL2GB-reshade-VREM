///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
// Defitniton of all functions exported by VREM dll and registered by the launcher addon
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
#include <thread>
#include <reshade.hpp>

#include "loader_addon_shared.h"
#include "loader_logs.h"


namespace fs = std::filesystem;
using namespace reshade::api;

extern SharedState g_shared_state;

extern void delete_all_saved_pipelines();

// Signatures of function exported by the addon !!! is including shared structure  !!!
typedef void (*InitFunc)(
    reshade::api::device*,
    reshade::api::command_queue*,
    reshade::api::swapchain*,
    PersistentPipelineData*,
    SharedState*
    );

typedef void (*CleanupFunc)(PersistentPipelineData*);

//*********************************************************************************************
// function mapping defintion (!!! they should also be handled in VREMHotReloader for a working mapping !!!)

struct AddonFunctions {
    InitFunc init = nullptr;
    CleanupFunc cleanup = nullptr;
    void* on_init_pipeline = nullptr;
    void* on_bind_pipeline = nullptr;
    void* on_init_pipeline_layout = nullptr;
    void* on_draw = nullptr;
    void* on_draw_indexed = nullptr;
    void* on_draw_indirect = nullptr;
    void* on_push_descriptors = nullptr;
    void* on_create_pipeline = nullptr;
    void* on_after_create_pipeline = nullptr;
    void* on_bind_render_targets = nullptr;
    void* on_reshade_present = nullptr;
    void* on_reshade_overlay = nullptr;
    void* on_reshade_reloaded_effects = nullptr;
    void* on_reshade_set_technique_state = nullptr;
    void* on_destroy_pipeline = nullptr;
};


//*******************************************************************************
// loader structure

class VREMHotReloader {
private:
    HMODULE addon_module = nullptr;
    AddonFunctions funcs;
    fs::file_time_type last_write_time;
    std::string addon_path;
    std::string temp_path;
    std::chrono::steady_clock::time_point last_check;
    const std::chrono::milliseconds check_interval{ 500 };

    device* cached_device = nullptr;
    command_queue* cached_queue = nullptr;
    swapchain* cached_swapchain = nullptr;

    PersistentPipelineData persistent_data;

public:
    VREMHotReloader(const std::string& path) : addon_path(path) {
        temp_path = addon_path + ".loaded";
        if (fs::exists(addon_path)) {
            last_write_time = fs::last_write_time(addon_path);
            load_addon();
        }
    }

    ~VREMHotReloader() {
        unload_addon();
    }

    /*
    void check_and_reload() {
        auto now = std::chrono::steady_clock::now();
        if (now - last_check < check_interval) return;
        last_check = now;

        if (!fs::exists(addon_path)) return;

        try {
            auto current_time = fs::last_write_time(addon_path);
            if (current_time != last_write_time) {
                last_write_time = current_time;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                //log_reload();
                reshade::log::message(reshade::log::level::info,
                    "DCS VREM: Modification détectée, rechargement...");
                reload_addon();
            }
        }
        catch (...) {}
    }
    */

    void manual_reload() {
        // reshade::log::message(reshade::log::level::info,"DCS VREM: Rechargement manuel...");
        log_manual_reload();
        reload_addon();
    }

    /*
    void set_cache(device* dev, command_queue* queue, swapchain* swap) {
        cached_device = dev;
        // cached_queue = queue; // peut être nullptr
        cached_queue = nullptr; // peut être nullptr
        cached_swapchain = swap;
    }
    */

    AddonFunctions& get_functions() { return funcs; }
    PersistentPipelineData& get_persistent_data() { return persistent_data; }
    bool is_loaded() const { return addon_module != nullptr; }

private:
    bool load_addon() {
        try {
            if (!fs::copy_file(addon_path, temp_path, fs::copy_options::overwrite_existing)) {
                // reshade::log::message(reshade::log::level::error,"DCS VREM: Impossible de copier la DLL");
                log_dll_copy_error();
                return false;
            }
        }
        catch (const std::exception& e) {
            reshade::log::message(reshade::log::level::error,
                (std::string("DCS VREM: Erreur copie: ") + e.what()).c_str());
            return false;
        }

        addon_module = LoadLibraryA(temp_path.c_str());
        if (!addon_module) {
            DWORD error = GetLastError();
            // reshade::log::message(reshade::log::level::error,("DCS VREM: LoadLibrary échoué, code: " + std::to_string(error)).c_str());
            log_dll_copy_error_code(error);
            return false;
        }

        // function mapping defintion (!!! they should also be handled in AddonFunctions for a working mapping !!!)
        funcs.init = (InitFunc)GetProcAddress(addon_module, "vrem_init");
        funcs.cleanup = (CleanupFunc)GetProcAddress(addon_module, "vrem_cleanup");
        funcs.on_reshade_present = GetProcAddress(addon_module, "vrem_on_reshade_present");
        funcs.on_reshade_reloaded_effects = GetProcAddress(addon_module, "vrem_on_reshade_reloaded_effects");

        funcs.on_init_pipeline = GetProcAddress(addon_module, "vrem_on_init_pipeline");
        funcs.on_bind_pipeline = GetProcAddress(addon_module, "vrem_on_bind_pipeline");
        funcs.on_init_pipeline_layout = GetProcAddress(addon_module, "vrem_on_init_pipeline_layout");
        funcs.on_draw = GetProcAddress(addon_module, "vrem_on_draw");
        funcs.on_draw_indexed = GetProcAddress(addon_module, "vrem_on_draw_indexed");
        funcs.on_draw_indirect = GetProcAddress(addon_module, "vrem_on_draw_indirect");
        funcs.on_push_descriptors = GetProcAddress(addon_module, "vrem_on_push_descriptors");
        funcs.on_create_pipeline = GetProcAddress(addon_module, "vrem_on_create_pipeline");
        funcs.on_after_create_pipeline = GetProcAddress(addon_module, "vrem_on_after_create_pipeline");
        funcs.on_bind_render_targets = GetProcAddress(addon_module, "vrem_on_bind_render_targets");
        funcs.on_reshade_overlay = GetProcAddress(addon_module, "vrem_on_reshade_overlay");
        funcs.on_reshade_set_technique_state = GetProcAddress(addon_module, "vrem_on_reshade_set_technique_state");
        funcs.on_destroy_pipeline = GetProcAddress(addon_module, "vrem_on_destroy_pipeline");


        // reshade::log::message(reshade::log::level::info,"DCS VREM: Addon chargé avec succès");
        log_success_load();

        // if (funcs.init && cached_device && cached_queue && cached_swapchain) {
        if (funcs.init) {
            // funcs.init(cached_device, cached_queue, cached_swapchain, &persistent_data);
            funcs.init(cached_device, cached_queue, cached_swapchain, &persistent_data, &g_shared_state);
        }

        return true;
    }

    void unload_addon() {
        if (!addon_module) return;

        if (funcs.cleanup) {
            funcs.cleanup(&persistent_data);
        }


        FreeLibrary(addon_module);
        addon_module = nullptr;
        memset(&funcs, 0, sizeof(funcs));

        try {
            for (int i = 0; i < 10; i++) {
                if (DeleteFileA(temp_path.c_str())) break;
                Sleep(10);
            }
        }
        catch (...) {}

        // free memory of addon persistant unordered_map => may be no more needed
        std::unordered_map<uint32_t, PipeLine_Definition>().swap(g_shared_state.VREM_pipelines.pipeline_by_hash);
        std::unordered_map<uint64_t, PipeLine_Definition>().swap(g_shared_state.VREM_pipelines.pipeline_by_handle);

        // delete saved pipelines
        delete_all_saved_pipelines();

        // reshade::log::message(reshade::log::level::info,"DCS VREM: Addon decharge");
        log_unloaded();
    }

    void reload_addon() {
        unload_addon();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        load_addon();
    }
};

//*******************************************************************************
// declaration of addon 

extern VREMHotReloader* g_reloader;

static void on_init_pipeline(device* dev, pipeline_layout layout, uint32_t count,
    const pipeline_subobject* objs, pipeline pipe) {

    if (g_reloader && g_reloader->get_functions().on_init_pipeline) {
        typedef void (*Func)(device*, pipeline_layout, uint32_t , const pipeline_subobject* , pipeline );
        ((Func)g_reloader->get_functions().on_init_pipeline)(dev, layout, count, objs, pipe);
    }
}

static void on_bind_pipeline(command_list* cmd, pipeline_stage stages, pipeline pipe) {
    if (g_reloader && g_reloader->get_functions().on_bind_pipeline) {
        typedef void (*Func)(command_list*, pipeline_stage, pipeline);
        ((Func)g_reloader->get_functions().on_bind_pipeline)(cmd, stages, pipe);
    }
}

static void on_init_pipeline_layout(device* dev, uint32_t count,
    const pipeline_layout_param* params, pipeline_layout layout) {
    if (g_reloader && g_reloader->get_functions().on_init_pipeline_layout) {
        typedef void (*Func)(device*, uint32_t, const pipeline_layout_param*, pipeline_layout);
        ((Func)g_reloader->get_functions().on_init_pipeline_layout)(dev, count, params, layout);
    }
}

static bool on_draw(command_list* cmd, uint32_t v, uint32_t i, uint32_t fv, uint32_t fi) {
    if (g_reloader && g_reloader->get_functions().on_draw) {
        typedef bool (*Func)(command_list*, uint32_t, uint32_t, uint32_t, uint32_t);
        return ((Func)g_reloader->get_functions().on_draw)(cmd, v, i, fv, fi);
    }
    return false;
}

static bool on_draw_indexed(command_list* cmd, uint32_t ic, uint32_t ins,
    uint32_t fi, int32_t vo, uint32_t fii) {
    if (g_reloader && g_reloader->get_functions().on_draw_indexed) {
        typedef bool (*Func)(command_list*, uint32_t, uint32_t, uint32_t, int32_t, uint32_t);
        return ((Func)g_reloader->get_functions().on_draw_indexed)(cmd, ic, ins, fi, vo, fii);
    }
    return false;
}

static bool on_draw_indirect(command_list* cmd, indirect_command type, resource buf,
    uint64_t off, uint32_t cnt, uint32_t stride) {
    if (g_reloader && g_reloader->get_functions().on_draw_indirect) {
        typedef bool (*Func)(command_list*, indirect_command, resource, uint64_t, uint32_t, uint32_t);
        return ((Func)g_reloader->get_functions().on_draw_indirect)(cmd, type, buf, off, cnt, stride);
    }
    return false;
}

static void on_push_descriptors(command_list* cmd, shader_stage stages, pipeline_layout layout,
    uint32_t idx, const descriptor_table_update& upd) {
    if (g_reloader && g_reloader->get_functions().on_push_descriptors) {
        typedef void (*Func)(command_list*, shader_stage, pipeline_layout, uint32_t, const descriptor_table_update&);
        ((Func)g_reloader->get_functions().on_push_descriptors)(cmd, stages, layout, idx, upd);
    }
}

static bool on_create_pipeline(device* dev, pipeline_layout layout, uint32_t count,
    const pipeline_subobject* objs) {
    if (g_reloader && g_reloader->get_functions().on_create_pipeline) {
        typedef bool (*Func)(device*, pipeline_layout, uint32_t, const pipeline_subobject*);
        return ((Func)g_reloader->get_functions().on_create_pipeline)(dev, layout, count, objs);
    }
    return false;
}

static void on_after_create_pipeline(device* dev, pipeline_layout layout, uint32_t count,
    const pipeline_subobject* objs, pipeline pipe) {
    if (g_reloader && g_reloader->get_functions().on_after_create_pipeline) {
        typedef void (*Func)(device*, pipeline_layout, uint32_t, const pipeline_subobject*, pipeline);
        ((Func)g_reloader->get_functions().on_after_create_pipeline)(dev, layout, count, objs, pipe);
    }
}

static void on_bind_render_targets(command_list* cmd, uint32_t count,
    const resource_view* rtvs, resource_view dsv) {
    if (g_reloader && g_reloader->get_functions().on_bind_render_targets) {
        typedef void (*Func)(command_list*, uint32_t, const resource_view*, resource_view);
        ((Func)g_reloader->get_functions().on_bind_render_targets)(cmd, count, rtvs, dsv);
    }
}

static void on_reshade_reloaded_effects(effect_runtime* runtime) {
    if (g_reloader && g_reloader->get_functions().on_reshade_reloaded_effects) {
        typedef void (*Func)(effect_runtime*);
        ((Func)g_reloader->get_functions().on_reshade_reloaded_effects)(runtime);
    }
}

static void on_reshade_set_technique_state(effect_runtime* runtime,
    effect_technique technique, bool enabled) {
    if (g_reloader && g_reloader->get_functions().on_reshade_set_technique_state) {
        typedef void (*Func)(effect_runtime*, effect_technique, bool);
        ((Func)g_reloader->get_functions().on_reshade_set_technique_state)(runtime, technique, enabled);
    }
}

static void on_destroy_pipeline(device* dev, pipeline pipe) {
    if (g_reloader && g_reloader->get_functions().on_destroy_pipeline) {
        typedef void (*Func)(device*, pipeline);
        ((Func)g_reloader->get_functions().on_destroy_pipeline)(dev, pipe);
    }
}