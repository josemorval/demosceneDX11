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
    float3 position :   POS;
    float3 normal   :   NOR;
    float2 texcoord :   TEX;
    uint   instance :   SV_InstanceID;
};

struct VS_OUT
{
    float4 position :   SV_POSITION;
    float3 normal   :   NOR;
    float2 texcoord :   TEXCOORD0;
    float3 texcoord1 :   TEXCOORD1;
};

VS_OUT vs_main(VS_IN input)
{
    VS_OUT output;

    float4 pos = float4(input.position, 1.0);
    pos.y += sin(0.6 *pos.x + time) * sin(0.7 * pos.z + 1.1*time);

    output.position = mul(view_matrix, pos);
    output.position = mul(projection_matrix, output.position);

    output.texcoord = input.texcoord;
    output.texcoord1 = pos;
    output.normal = input.normal;

    return output;
}

float4 ps_main(VS_OUT input) : SV_TARGET
{


    float4 position_light_space = mul(lightview_matrix, input.texcoord1);
    position_light_space = mul(lightprojection_matrix, position_light_space);

    float dx = 0.0002;
    const float2 offsets[9] =
    {
        float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
    };

    float f = 0.0;

    //Shadow map filter
    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        f += mytexture.Sample(mysampler, 0.5 - 0.5 * float2(-1.0, 1.0) * position_light_space.xy + offsets[i]).r;
    }

    float shadow_map_depth = f/9.0;
    float current_depth = position_light_space.z;


    float sha = smoothstep(0.65,0.66,shadow_map_depth-current_depth);

    float3 nor = normalize(input.normal);
    float dif = clamp(dot(nor, normalize(float3(0.5, 0.5, 0.5))),0.0,1.0);
    
    float3 col = float3(0.7 + 0.1*nor.y,0.4+0.3*nor.x,0.5-0.1*nor.z);
    col *= dif * sha;
    col += float3(0.0, 0.0, 0.1+0.0002*input.position.y);
    return float4(col,1.0);
}

VS_OUT vs_main_light(VS_IN input)
{
    VS_OUT output;

    float4 pos = float4(input.position, 1.0);
    pos.y += sin(0.6 * pos.x + time) * sin(0.7 * pos.z + 1.1 * time);

    output.position = mul(lightview_matrix, pos);
    output.position = mul(lightprojection_matrix, output.position);

    output.texcoord = input.texcoord;
    output.texcoord1 = pos;
    output.normal = input.normal;

    return output;
}

float4 ps_main_light(VS_OUT input) : SV_TARGET
{
    return float4(0.0,0.0,0.0,0.0);
}
