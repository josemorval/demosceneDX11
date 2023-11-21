#include "common.hlsl"

static const float3 boxcorners[24] =
{
    1.0, -1.0, 1.0,
    1.0, 1.0, 1.0,
    -1.0, -1.0, 1.0,
    -1.0, 1.0, 1.0,

    -1.0, 1.0, 1.0,
    -1.0, 1.0, -1.0,
    -1.0, -1.0, 1.0,
    -1.0, -1.0, -1.0,

    1.0, 1.0, -1.0,
    1.0, 1.0, 1.0,
    1.0, -1.0, -1.0,
    1.0, -1.0, 1.0,

    -1.0, 1.0, -1.0,
    1.0, 1.0, -1.0,
    -1.0, -1.0, -1.0,
    1.0, -1.0, -1.0,

    1.0, 1.0, 1.0,
    1.0, 1.0, -1.0,
    -1.0, 1.0, 1.0,
    -1.0, 1.0, -1.0,

    -1.0, -1.0, -1.0,
    1.0, -1.0, -1.0,
    -1.0, -1.0, 1.0,
    1.0, -1.0, 1.0
};

Texture2D    shadowMap          : register(t0);
Texture3D    voxelTexture       : register(t1);
SamplerState shadowMapSampler
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Wrap;
    AddressV = Wrap;
};

struct VS_IN
{
    uint vid        :   SV_VertexID;
};

struct VS_OUT
{
    float4 position :   SV_POSITION;
    float3 normal : TEXCOORD0;
};

struct GS_OUT
{
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float3 wpos : TEXCOORD1;
};

VS_OUT vs_main(VS_IN input)
{
    VS_OUT output;
    uint vdim = uint(voxel_dim);

    uint3 id = uint3(input.vid % vdim, (input.vid/vdim) % vdim,(input.vid/(vdim*vdim)));
    output.position = float4(12.0*(float3(id.x,id.y,id.z)/voxel_dim-0.5),voxelTexture[id].w);
    //output.normal = voxelTexture[id].xyz;

    output.normal = voxelTexture.SampleLevel(shadowMapSampler, float3(id)/(voxel_dim-1.0), 0);
    return output;
}

[maxvertexcount(24)]
void gs_main(point VS_OUT input[1], inout TriangleStream<GS_OUT> tri_stream)
{
    if (input[0].position.w < 0.5)
    {
        return;
    }

    input[0].position.xyz += float3(0.0, 3.0, 0.0);

    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            GS_OUT output;
            float voxelsize = 6.0 / float(voxel_dim);
            output.position = mul(mul(projection_matrix, view_matrix), float4(voxelsize*boxcorners[4*i+j] + input[0].position.xyz, 1.0));
            output.normal = input[0].normal;
            output.wpos = input[0].position.xyz;
            tri_stream.Append(output);
        }
        tri_stream.RestartStrip();
    }
}

float4 ps_main(GS_OUT input) : SV_TARGET
{
    float4 position_light_space = mul(lightview_matrix, float4(input.wpos,1.0));
    position_light_space = mul(lightprojection_matrix, position_light_space);

    float shadow_map_depth = shadowMap.Sample(shadowMapSampler, 0.5 - 0.5 * float2(-1.0, 1.0) * position_light_space.xy).r;
    float current_depth = position_light_space.z;

    //Compute light direction from light matrix
    float3 light_forward = -(mul(float4(0.0, 0.0, 1.0, 0.0),lightview_matrix)).xyz;

    float sha = 1.0 - smoothstep(-0.01,-0.05,shadow_map_depth - current_depth);
    float3 nor = normalize(input.normal);
    float dif = clamp(dot(nor, normalize(light_forward)),0.0,1.0);

    float3 col = float3(0.5,0.5,0.5);
    col *= dif * sha;
    col += 0.3 * float3(0.1 + 0.0005 * input.position.x, 0.0, 0.1 + 0.0005 * input.position.y);

    return float4(col,1.0);
}