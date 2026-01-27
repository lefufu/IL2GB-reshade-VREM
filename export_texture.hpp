///////////////////////////////////////////////////////////////////////
//
// Reshade IL2 VREM addon. VR Enhancer Mod for IL2 using reshade
// "hot" reload of mod possible using a Reshade addon as launcher (loaded with the game)
// and a dll containing the mod logic itselve. Mod settings are in uniforms of a technique
// 
// ----------------------------------------------------------------------------------------
// export textures functions
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
#include <filesystem>
#include <fstream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <Windows.h>

#include <thread>
#include <chrono>

#include "config.hpp"
#include "to_string.hpp"

namespace fs = std::filesystem;

extern SharedState* g_shared_state;

class TextureExporter {
private:
    uint32_t draw_call_counter = 0;
    uint64_t current_ps_hash = 0;

    // get a timestamp
    uint64_t get_reshade_timestamp() {
        static auto start_time = std::chrono::high_resolution_clock::now();
        auto current_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_time - start_time
        );
        return duration.count();
    }

    // genrate a filename including some meta data
    std::string generate_filename(
        uint64_t ps_hash,
        uint32_t draw_num,
        uint64_t timestamp,
        uint32_t texture_index,
        const std::string& extension = RESHADE_ADDON_TEXTURE_SAVE_FORMAT
    ) {
        std::ostringstream oss;
        // oss << "texture_"
        oss << "PS_0x" << std::hex << std::setfill('0') << std::setw(16) << ps_hash
            << "_draw_" << std::dec << draw_num
            << "_ts_" << timestamp
            << "_idx_" << texture_index
            << extension;
        return oss.str();
    }

    // Sauvegarder une texture en DDS
    bool save_texture(
        reshade::api::device* device,
        reshade::api::command_list* cmd_list,
        reshade::api::resource texture,
        const std::string& filepath
    ) {
        using namespace reshade::api;

        // Récupérer les informations de la texture source
        resource_desc desc = device->get_resource_desc(texture);

        if (desc.type != resource_type::texture_2d) {
            return false; // Supporte uniquement les textures 2D
        }

        // Créer une texture de staging pour la copie CPU
        resource_desc staging_desc = desc;
        staging_desc.usage = resource_usage::copy_dest;
        staging_desc.heap = memory_heap::gpu_to_cpu;
        staging_desc.flags = resource_flags::none;

        resource staging_texture = {};
        if (!device->create_resource(staging_desc, nullptr, resource_usage::copy_dest, &staging_texture)) {
            reshade::log::message(reshade::log::level::error, "Failed to create staging texture");
            return false;
        }

        // Copier la texture GPU vers la texture de staging
        cmd_list->barrier(texture, resource_usage::shader_resource, resource_usage::copy_source);
        cmd_list->copy_resource(texture, staging_texture);
        cmd_list->barrier(texture, resource_usage::copy_source, resource_usage::shader_resource);

        // Attendre que la copie soit terminée
        // Note: Dans un vrai addon, vous devriez utiliser une fence ou attendre la fin de la frame
        //device->wait_idle();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Mapper la texture de staging
        subresource_data mapped_data = {};
        if (!device->map_texture_region(
            staging_texture,
            0, // subresource
            nullptr, // box (toute la texture)
            map_access::read_only,
            &mapped_data
        )) {
            reshade::log::message(reshade::log::level::error, "Failed to map staging texture");
            device->destroy_resource(staging_texture);
            return false;
        }


        // Vérifier que les données mappées sont valides
        if (!mapped_data.data) {
            reshade::log::message(reshade::log::level::error, "Mapped data is null");
            device->unmap_texture_region(staging_texture, 0);
            device->destroy_resource(staging_texture);
            return false;
        }

		// save texture image using reshade function
        bool status = save_texture_image(desc, mapped_data, filepath);

        /*
        // Créer le fichier DDS
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            device->unmap_texture_region(staging_texture, 0);
            device->destroy_resource(staging_texture);
            return false;
        }

        // En-tête DDS simplifié (à adapter selon le format exact)
        struct DDS_HEADER {
            uint32_t magic = 0x20534444; // "DDS "
            uint32_t size = 124;
            uint32_t flags = 0x1 | 0x2 | 0x4 | 0x1000; // CAPS, HEIGHT, WIDTH, PIXELFORMAT
            uint32_t height;
            uint32_t width;
            uint32_t pitch;
            uint32_t depth = 0;
            uint32_t mipMapCount = 1;
            uint32_t reserved1[11] = {};
            // DDSPIXELFORMAT
            struct {
                uint32_t size = 32;
                uint32_t flags = 0x41; // DDPF_RGB | DDPF_ALPHAPIXELS
                uint32_t fourCC = 0;
                uint32_t rgbBitCount = 32;
                uint32_t rBitMask = 0x00FF0000;
                uint32_t gBitMask = 0x0000FF00;
                uint32_t bBitMask = 0x000000FF;
                uint32_t aBitMask = 0xFF000000;
            } ddspf;
            uint32_t caps = 0x1000; // DDSCAPS_TEXTURE
            uint32_t caps2 = 0;
            uint32_t caps3 = 0;
            uint32_t caps4 = 0;
            uint32_t reserved2 = 0;
        } header;

        header.width = desc.texture.width;
        header.height = desc.texture.height;
        header.pitch = mapped_data.row_pitch;

        reshade::log::message(reshade::log::level::info, "start writing");

        //if (desc.texture.width != 0 && desc.texture.height != 0)
        {
            // Écrire l'en-tête
            file.write(reinterpret_cast<char*>(&header), sizeof(header));
            reshade::log::message(reshade::log::level::info, "header written");

            // Écrire les données de la texture
            const uint8_t* src = static_cast<const uint8_t*>(mapped_data.data);
            for (uint32_t y = 0; y < desc.texture.height; ++y) {
                file.write(
                    reinterpret_cast<const char*>(src + y * mapped_data.row_pitch),
                    desc.texture.width * 4 // Supposons RGBA 8 bits
                );
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_BETWEEN_TEXTURE_EXPORT_MS));
        }
        reshade::log::message(reshade::log::level::info, "close file");
        file.close();
        
        */
        // Nettoyer
        device->unmap_texture_region(staging_texture, 0);
        device->destroy_resource(staging_texture);

        return status;
    }

public:
    // Fonction principale à appeler depuis on_push_descriptors
    void export_descriptors(
        reshade::api::command_list* cmd_list,
        reshade::api::shader_stage stages,
        reshade::api::pipeline_layout layout,
        uint32_t param_index,
        const reshade::api::descriptor_table_update& update,
        uint64_t ps_hash, // Hash du pixel shader qui sera bindé
        uint32_t vr_draw_num // Numéro de draw pour quad view VR (0, 1, etc.)
    ) {
        using namespace reshade::api;

        // Obtenir le device depuis la command list
        device* dev = cmd_list->get_device();

        // Timestamp ReShade
        uint64_t timestamp = get_reshade_timestamp();
        
        
        // Créer le dossier de sortie si nécessaire
        /*
        fs::path output_dir = "reshade_texture_dumps";
        if (!fs::exists(output_dir)) {
            fs::create_directories(output_dir);
        }*/
        

        // fs::path output_dir = fs::path(g_shared_state->g_vrem_base_path) / RESHADE_ADDON_TEXTURE_SAVE_DIR;
        fs::path output_dir = RESHADE_ADDON_TEXTURE_SAVE_DIR;
        if (!fs::exists(output_dir)) {
            fs::create_directories(output_dir);
        }

        // Parcourir tous les descripteurs dans la table
        for (uint32_t i = 0; i < update.count; ++i) {
            // Vérifier si c'est une texture (SRV)
            if (update.type == descriptor_type::shader_resource_view) {
                resource_view srv = static_cast<const resource_view*>(update.descriptors)[i];

                if (srv.handle == 0) continue;

                // Obtenir la ressource depuis la vue
                resource tex = dev->get_resource_from_view(srv);

                if (tex.handle == 0) continue;

                // Générer le nom de fichier
                std::string filename = generate_filename(
                    ps_hash,
                    vr_draw_num,
                    timestamp,
                    i // Index de la texture dans la table
                );

                fs::path filepath = output_dir / filename;

                // Exporter la texture
                if (save_texture(dev, cmd_list, tex, filepath.string())) {
                    reshade::log::message(
                        reshade::log::level::info,
                        ("Exported texture: " + filepath.string()).c_str()
                    );
                }
            }
        }
    }
};

