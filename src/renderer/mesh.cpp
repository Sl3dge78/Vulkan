#include <SDL/SDL.h>

#include <sl3dge/sl3dge.h>

#include "mesh.h"

void RendererLoadMaterialsAndTextures(Renderer *context, cgltf_data *data, const char *directory) {
    // Copy material data
    void *mapped_mat_buffer;
    const u32 offset = context->materials_count * sizeof(Material);
    MapBuffer(context->device, &context->mat_buffer, &mapped_mat_buffer);
    GLTFLoadMaterialBuffer(data, (Material *)mapped_mat_buffer);
    UnmapBuffer(context->device, &context->mat_buffer);

    context->materials_count += data->materials_count;

    SDL_Log("Loading textures...");
    context->textures_count += data->textures_count;
    if(data->textures_count > 0) {
        SDL_Surface **surfaces =
            (SDL_Surface **)calloc(data->textures_count, sizeof(SDL_Surface *));
        Buffer *image_buffers = (Buffer *)calloc(data->textures_count, sizeof(Buffer));
        VkCommandBuffer *cmds =
            (VkCommandBuffer *)calloc(data->textures_count, sizeof(VkCommandBuffer));

        AllocateCommandBuffers(
            context->device, context->graphics_command_pool, data->textures_count, cmds);

        for(u32 i = 0; i < data->textures_count; ++i) {
            char *image_path = data->textures[i].image->uri;
            ASSERT_MSG(image_path,
                       "Attempting to load an embedded texture. "
                       "This isn't supported yet");
            u32 file_path_length = strlen(directory) + strlen(image_path) + 1;
            char *full_image_path = (char *)calloc(file_path_length, sizeof(char *));
            strcat_s(full_image_path, file_path_length, directory);
            strcat_s(full_image_path, file_path_length, image_path);

            SDL_Surface *temp_surf = IMG_Load(full_image_path);
            if(!temp_surf) {
                SDL_LogError(0, IMG_GetError());
            }

            if(temp_surf->format->format != SDL_PIXELFORMAT_ABGR8888) {
                surfaces[i] = SDL_ConvertSurfaceFormat(temp_surf, SDL_PIXELFORMAT_ABGR8888, 0);
                if(!surfaces[i]) {
                    SDL_LogError(0, SDL_GetError());
                }
                SDL_FreeSurface(temp_surf);
            } else {
                surfaces[i] = temp_surf;
            }
            free(full_image_path);
        }

        for(u32 i = 0; i < data->textures_count; ++i) {
            VkDeviceSize image_size = surfaces[i]->h * surfaces[i]->pitch;
            VkExtent2D extent = {(u32)surfaces[i]->h, (u32)surfaces[i]->w};

            VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
            if(data->textures[i].type == cgltf_texture_type_base_color)
                format = VK_FORMAT_R8G8B8A8_SRGB;

            CreateImage(context->device,
                        &context->memory_properties,
                        format,
                        extent,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        &context->textures[i]);
            DEBUGNameImage(context->device, &context->textures[i], data->textures[i].image->uri);
            CreateBuffer(context->device,
                         &context->memory_properties,
                         image_size,
                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &image_buffers[i]);
            SDL_LockSurface(surfaces[i]);
            UploadToBuffer(context->device, &image_buffers[i], surfaces[i]->pixels, image_size);
            SDL_UnlockSurface(surfaces[i]);
            BeginCommandBuffer(
                context->device, cmds[i], VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            CopyBufferToImage(
                cmds[i], extent, surfaces[i]->pitch, &image_buffers[i], &context->textures[i]);
            vkEndCommandBuffer(cmds[i]);
            VkSubmitInfo si = {
                VK_STRUCTURE_TYPE_SUBMIT_INFO, NULL, 0, NULL, 0, 1, &cmds[i], 0, NULL};
            vkQueueSubmit(context->graphics_queue, 1, &si, VK_NULL_HANDLE);
        }
        vkQueueWaitIdle(context->graphics_queue);
        VulkanUpdateTextureDescriptorSet(context->device,
                                         context->main_render_group.descriptor_sets[0],
                                         context->texture_sampler,
                                         context->textures_count,
                                         context->textures);

        vkFreeCommandBuffers(
            context->device, context->graphics_command_pool, data->textures_count, cmds);
        free(cmds);
        for(u32 i = 0; i < data->textures_count; ++i) {
            SDL_FreeSurface(surfaces[i]);
            DestroyBuffer(context->device, &image_buffers[i]);
        }
        free(surfaces);
        free(image_buffers);
    }
}

Mesh *LoadMesh(const char *file, Renderer *renderer) {
    Mesh *mesh = (Mesh *)malloc(sizeof(Mesh));
    *mesh = {};

    char *directory;
    const char *last_sep = strrchr(file, '/');
    u32 size = last_sep - file;
    directory = (char *)calloc(size + 2, sizeof(char));
    strncpy_s(directory, size + 2, file, size);
    directory[size] = '/';
    directory[size + 1] = '\0';

    cgltf_data *data;
    cgltf_options options = {};
    cgltf_result result = cgltf_parse_file(&options, file, &data);
    if(result != cgltf_result_success) {
        SDL_LogError(0, "Error reading mesh");
        ASSERT(0);
    }

    cgltf_load_buffers(&options, data, file);

    // Transforms
    mesh->primitive_nodes_count = data->nodes_count;
    mesh->primitive_transforms = (Mat4 *)calloc(mesh->primitive_nodes_count, sizeof(Mat4));
    GLTFLoadTransforms(data, mesh->primitive_transforms);

    // Primitives
    mesh->total_primitives_count = 0;
    for(u32 m = 0; m < data->meshes_count; ++m) {
        mesh->total_primitives_count += data->meshes[m].primitives_count;
    };
    mesh->primitives = (Primitive *)calloc(mesh->total_primitives_count, sizeof(Primitive));

    u32 i = 0;
    for(u32 m = 0; m < data->meshes_count; ++m) {
        for(u32 p = 0; p < data->meshes[m].primitives_count; p++) {
            cgltf_primitive *prim = &data->meshes[m].primitives[p];

            Primitive *primitive = &mesh->primitives[i];

            primitive->vertex_count = (u32)prim->attributes[0].data->count;
            primitive->vertex_offset = mesh->total_vertex_count;
            mesh->total_vertex_count += (u32)prim->attributes[0].data->count;

            primitive->index_count += prim->indices->count;
            primitive->index_offset = mesh->total_index_count;
            mesh->total_index_count += (u32)prim->indices->count;

            primitive->material_id = GLTFGetMaterialID(prim->material) + renderer->materials_count;

            ++i;
        }
    }
    mesh->all_index_offset = mesh->total_vertex_count * sizeof(Vertex);
    const u32 buffer_size =
        mesh->total_vertex_count * sizeof(Vertex) + mesh->total_index_count * sizeof(u32);

    // RendererLoadGLTF(data);
    // TODO : Move that v into here ^
    // Vertex & Index Buffer
    CreateBuffer(renderer->device,
                 &renderer->memory_properties,
                 buffer_size,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &mesh->buffer);
    DEBUGNameBuffer(renderer->device, &mesh->buffer, "GLTF VTX/IDX");

    void *mapped_buffer;
    MapBuffer(renderer->device, &mesh->buffer, &mapped_buffer);
    i = 0;
    for(u32 m = 0; m < data->meshes_count; ++m) {
        for(u32 p = 0; p < data->meshes[m].primitives_count; ++p) {
            GLTFLoadVertexAndIndexBuffer(&data->meshes[m].primitives[p],
                                         &mesh->primitives[i],
                                         mesh->total_vertex_count * sizeof(Vertex),
                                         mapped_buffer);
            ++i;
        }
    }
    UnmapBuffer(renderer->device, &mesh->buffer);

    // Materials
    RendererLoadMaterialsAndTextures(renderer, data, directory);

    mesh->instance_transforms = (Mat4 *)calloc(1, sizeof(Mat4));

    cgltf_free(data);
    free(directory);

    return mesh;
}

void DestroyMesh(Renderer *renderer, Mesh *mesh) {
    free(mesh->primitives);

    free(mesh->instance_transforms);

    DestroyBuffer(renderer->device, &mesh->buffer);

    free(mesh->primitive_transforms);
    free(mesh);
}

inline void DrawMeshInstance(Frame *frame,
                             const u32 primitives_count,
                             Primitive *primitives,
                             const Mat4 *primitives_transforms,
                             const Mat4 *mesh_transform) {
    for(u32 p = 0; p < primitives_count; p++) {
        const Primitive *prim = &primitives[p];

        Mat4 mat = mat4_mul(mesh_transform, &primitives_transforms[prim->node_id]);
        PushConstant push = {mat, prim->material_id};
        vkCmdPushConstants(
            frame->cmd, frame->layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &push);
        vkCmdDrawIndexed(
            frame->cmd, prim->index_count, 1, prim->index_offset, prim->vertex_offset, 0);
    }
}

void DrawMesh(Frame *frame, Mesh *mesh) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(frame->cmd, 0, 1, &mesh->buffer.buffer, &offset);
    vkCmdBindIndexBuffer(
        frame->cmd, mesh->buffer.buffer, mesh->all_index_offset, VK_INDEX_TYPE_UINT32);

    for(u32 i = 0; i < mesh->instance_count; i++) {
        DrawMeshInstance(frame,
                         mesh->total_primitives_count,
                         mesh->primitives,
                         mesh->primitive_transforms,
                         &mesh->instance_transforms[i]);
    }
}