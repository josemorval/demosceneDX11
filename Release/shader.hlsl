#include "common.hlsl"

Texture2D    shadowMap : register(t0);
SamplerState shadowMapSampler
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Wrap;
    AddressV = Wrap;
    ComparisonFunc = Never;
};

struct VS_IN
{
    float3 position :   POSITION;
    float3 normal   :   NORMAL;
    float2 texcoord :   TEXCOORD;
};

struct VS_OUT
{
    float4 position :   SV_POSITION;
    float3 normal   :   NORMAL;
    float2 texcoord :   TEXCOORD0;
    float3 wpos     :   TEXCOORD1;
};

VS_OUT vs_main(VS_IN input)
{
    VS_OUT output;

    float3 wpos = get_world_position(input.position);

    output.position = mul(view_matrix, float4(wpos, 1.0));
    output.position = mul(projection_matrix, output.position) +float4(0.0, 0.0, 0.0, 0.0);

    output.texcoord = input.texcoord;
    output.wpos     = wpos;
    output.normal   = input.normal;

    return output;
}
float4 ps_main(VS_OUT input) : SV_TARGET
{

    float4 position_light_space = mul(lightview_matrix, float4(input.wpos,1.0));
    position_light_space = mul(lightprojection_matrix, position_light_space);

    float shadow_map_depth = shadowMap.Sample(shadowMapSampler, 0.5 - 0.5 * float2(-1.0, 1.0) * position_light_space.xy).r;
    float current_depth = position_light_space.z;

    //Compute light direction from light matrix
    float3 light_forward = -(mul(float4(0.0, 0.0, 1.0, 0.0),lightview_matrix)).xyz;

    float sha = 1.0-smoothstep(-0.01,-0.05,shadow_map_depth - current_depth);
    float3 nor = normalize(input.normal);
    float dif = clamp(dot(nor, normalize(light_forward)),0.0,1.0);

    float3 col = float3(0.5,0.5,0.5);
    col *= dif * sha;
    col += 0.3*float3(0.1 + 0.0005 * input.position.x, 0.0, 0.1 + 0.0005 * input.position.y);

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