
cbuffer constants : register(b0)
{
    row_major float4x4 transform;
    row_major float4x4 projection;
    float3   lightvector;
}

struct vs_in
{
    float3 position : POS;
    float3 normal   : NOR;
    float2 texcoord : TEX;
    float3 color    : COL;
    uint Instance   : SV_InstanceID;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float2 texcoord : TEX;
    float4 color    : COL;
};

Texture2D    mytexture : register(t0);
SamplerState mysampler : register(s0);


float hash13(float3 p3)
{
    p3 = frac(p3 * .1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return frac((p3.x + p3.y) * p3.z);
}

float3 hash31(float p)
{
    float3 p3 = frac(float3(p,p,p) * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xxy + p3.yzz) * p3.zyx);
}

vs_out vs_main(vs_in input)
{
    float light = clamp(dot(mul(input.normal, transform), normalize(-lightvector)), 0.0f, 1.0f) * 0.8f + 0.2f;

    vs_out output;

    output.position = mul(float4(input.position, 1.0f), mul(transform, projection));
    output.texcoord = input.texcoord;
    output.color = float4(input.color * light, 1.0f);

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    return float4(1.0,0.0,0.0,1.0);
    return mytexture.Sample(mysampler, input.texcoord) * input.color;
}