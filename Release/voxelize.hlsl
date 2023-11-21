#include "common.hlsl"

RWTexture3D<float4> voxelTexture : register(u1);

struct VS_IN
{
    float3 position :   POSITION;
    float3 normal   :   NORMAL;
    float2 texcoord :   TEXCOORD;
};

struct VS_OUT
{
    float4 position :   SV_POSITION;
    float3 normal   :   TEXCOORD0;
    float2 texcoord :   TEXCOORD1;
    float3 wpos     :   TEXCOORD2;
};

struct GS_OUT
{
    float4 position         : SV_POSITION;
    float3 position_view    : TEXCOORD0;
    float3 normal           : TEXCOORD1;
};

VS_OUT vs_main(VS_IN input)
{
    VS_OUT output;

    float3 wpos = get_world_position(input.position);

    output.position = mul(view_matrix, float4(wpos, 1.0));
    output.position = mul(projection_matrix, output.position);

    output.texcoord = input.texcoord;
    output.wpos = wpos;
    output.normal = input.normal;

    return output;
}

[maxvertexcount(3)]
void gs_main(triangle VS_OUT input[3], inout TriangleStream<GS_OUT> tri_stream) 
{
    for (int i = 0; i < 3; i++)
    {
        GS_OUT output;

        float3 wpos = input[i].wpos;
        float2 pos = wpos.xy;
        pos /= 15.0;

        float angle = 2. * 3.1419 * float(i)/3.0;
        pos = 0.3*float2(cos(angle), sin(angle));

        output.position = float4(pos,1.0,1.0);
        output.position_view = input[i].wpos;
        output.normal = input[i].normal;

        tri_stream.Append(output);
    }

    tri_stream.RestartStrip();
}

float4 ps_main(GS_OUT input) : SV_TARGET
{

    float3 wpos = input.position_view - float3(0.0,3.0,0.0);
    float vdim = float(voxel_dim);

    wpos = floor((wpos/12.0 + 0.5) * vdim);

    if (wpos.x < vdim && wpos.y < vdim && wpos.z < vdim)
    {
        voxelTexture[uint3(wpos.x,wpos.y,wpos.z)] = float4(input.normal, 1.0);
    }
    

    return float4(1.0,0.0,0.0,0.0);
}

