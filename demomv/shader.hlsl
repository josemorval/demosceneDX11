//--------------------------------------------------------------------------------------
// Constant Buffers.
//--------------------------------------------------------------------------------------

cbuffer global_shader_constants : register(b0)
{
    float4x4 projection_matrix;
    float4x4 view_matrix;
    float4x4 lightprojection_matrix;
    float4x4 lightview_matrix;
    float time;
};

Texture2D    mytexture : register(t0);
SamplerState mysampler : register(s0);

struct VS_IN
{
    float3 position :   POSITION;
    float3 normal   :   NORMAL;
    float2 texcoord :   TEXCOORD;
    uint   instance :   SV_InstanceID;
};

struct VS_OUT
{
    float4 position :   SV_POSITION;
    float3 normal   :   NORMAL;
    float2 texcoord :   TEXCOORD0;
    float3 wpos     :   TEXCOORD1;
};

float3 get_world_position(float3 local_position)
{
    //local_position.y += sin(0.6 * local_position.x + time) * sin(0.7 * local_position.z + 1.1 * time);
    return local_position;
}

VS_OUT vs_main(VS_IN input)
{
    VS_OUT output;

    float3 wpos = get_world_position(input.position);

    output.position = mul(view_matrix, float4(wpos, 1.0));
    output.position = mul(projection_matrix, output.position);

    output.texcoord = input.texcoord;
    output.wpos     = wpos;
    output.normal   = input.normal;

    return output;
}

float4 ps_main(VS_OUT input) : SV_TARGET
{

    float4 position_light_space = mul(lightview_matrix, float4(input.wpos,1.0));
    position_light_space = mul(lightprojection_matrix, position_light_space);

    float shadow_map_depth = mytexture.Sample(mysampler, 0.5 - 0.5 * float2(-1.0, 1.0) * position_light_space.xy).r;
    float current_depth = position_light_space.z;

    //Compute light direction from light matrix
    float3 light_forward = -(mul(float4(0.0, 0.0, 1.0, 0.0),lightview_matrix)).xyz;

    float sha = 1.0-smoothstep(-0.01,-0.05,shadow_map_depth - current_depth);
    float3 nor = normalize(input.normal);
    float dif = clamp(dot(nor, normalize(light_forward)),0.0,1.0);

    float3 col = float3(0.5,0.5,0.5);
    col *= dif * sha;
    col += 0.3*float3(0.1 + 0.0005 * input.position.x, 0.0, 0.1 + 0.0005 * input.position.y);

    float gamma = 1.0;
    col.x = pow(col.x, gamma);
    col.y = pow(col.y, gamma);
    col.z = pow(col.z, gamma);

    return float4(col,1.0);
}

VS_OUT vs_main_light(VS_IN input)
{
    VS_OUT output;

    float3 wpos = get_world_position(input.position);

    output.position = mul(lightview_matrix, float4(wpos,1.0));
    output.position = mul(lightprojection_matrix, output.position);

    output.texcoord = input.texcoord;
    output.wpos = wpos;
    output.normal = input.normal;

    return output;
}

float4 ps_main_light(VS_OUT input) : SV_TARGET
{
    return float4(0.0,0.0,0.0,0.0);
}
