#version 460
#extension GL_GOOGLE_include_directive  : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_ARB_shader_clock : enable

#include "shader_utils.glsl"
#include "random.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT top_level_AS;
layout(binding = 2, set = 0) uniform CameraMatrices {
    mat4 view;
    mat4 proj;
    mat4 view_inverse;
    mat4 proj_inverse;
    int frame;
} cam;
layout(binding = 3, set = 0) buffer Scene {Instance i[];} scene;
layout(binding = 4, set = 0) buffer Vertices {Vertex v[];} vertices[];
layout(binding = 5, set = 0) buffer Indices {uint i[];} indices[];
layout(binding = 6, set = 0) uniform sampler2D textures[];
layout(binding = 7, set = 0) buffer Materials {Material m[];} materials;

layout(push_constant) uniform Constants {
    vec4 clear_color;
    vec3 light_position;
    float light_instensity;
    int light_type;
} constants;

layout(location = 0) rayPayloadInEXT HitPayloadSimple payload;
layout(location = 1) rayPayloadInEXT bool is_shadow;
hitAttributeEXT vec3 attribs;

void main() {

    uint mesh_id = scene.i[gl_InstanceCustomIndexEXT].mesh_id;
    uint mat_id = scene.i[gl_InstanceCustomIndexEXT].mat_id;
    Material mat = materials.m[mat_id];

    ivec3 idx = ivec3(indices[nonuniformEXT(mesh_id)].i[3 * gl_PrimitiveID + 0], 
                    indices[nonuniformEXT(mesh_id)].i[3 * gl_PrimitiveID + 1], 
                    indices[nonuniformEXT(mesh_id)].i[3 * gl_PrimitiveID + 2]);

    Vertex v0 = vertices[nonuniformEXT(mesh_id)].v[idx.x];
    Vertex v1 = vertices[nonuniformEXT(mesh_id)].v[idx.y];
    Vertex v2 = vertices[nonuniformEXT(mesh_id)].v[idx.z];

    const vec3 barycentre = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 normal = normalize(v0.normal * barycentre.x + v1.normal * barycentre.y + v2.normal * barycentre.z);
    normal = normalize(vec3(scene.i[gl_InstanceCustomIndexEXT].inverted * vec4(normal, 0.0)));

    vec3 world_pos = v0.pos * barycentre.x + v1.pos * barycentre.y + v2.pos * barycentre.z;
    world_pos = vec3(scene.i[gl_InstanceCustomIndexEXT].transform * vec4(world_pos, 1.0));

    vec2 tex_coord = v0.tex_coord * barycentre.x + v1.tex_coord * barycentre.y + v2.tex_coord * barycentre.z;
    
    vec3 albedo = mat.diffuse_color;
    if(mat.texture_id > -1)
        albedo = texture(textures[mat.texture_id], tex_coord).xyz;
    payload.direct_color = albedo;
    
    uint ray_flags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;

    vec3 light_dir = normalize(constants.light_position - world_pos);
    float t_max = length(constants.light_position - world_pos);
    is_shadow = true;
    traceRayEXT(top_level_AS, ray_flags, 0xFF, 0, 0, 1, world_pos, 0.1, light_dir, t_max, 1);

    if(is_shadow) {
       payload.direct_color *= 0.1;
    } else {
        payload.direct_color *= max(0.1, dot(normal, light_dir));
    }

    /*
    vec3 tangent, bitangent;
    create_coordinate_system(normal, tangent, bitangent);
    vec3 dir = sampling_hemi(payload.seed, tangent, bitangent, normal);

    vec3 emittance = vec3(0.2);
    const float p = 1 / M_PI;
    float cos_theta = dot(dir, normal);
    vec3 BRDF = albedo / M_PI;

    payload.direct_color = emittance;
    payload.ray_origin = world_pos;
    payload.ray_direction = dir;
    payload.weight = BRDF * cos_theta / p;
    */
}
