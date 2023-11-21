cbuffer global_shader_constants : register(b0)
{
    float4x4 projection_matrix;
    float4x4 view_matrix;
    float4x4 lightprojection_matrix;
    float4x4 lightview_matrix;
    int voxel_dim;
    bool anim_scene;
    float time;
};

float3 get_world_position(float3 local_position)
{
    if (anim_scene)
    {
        local_position.y += sin(0.6 * local_position.x + time) * sin(0.7 * local_position.z + 1.1 * time);
    }
    return local_position;
}
