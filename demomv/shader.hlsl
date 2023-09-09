cbuffer constants : register(b0) 
{
    row_major float4x4 projection;
    float time;
    float3 camerapos;
    float3 cameradir;
};

struct vs_in
{
    float3 position :   POS;
    float3 normal   :   NOR;
    float2 texcoord :   TEX;
    uint   instance :   SV_InstanceID;
};

struct vs_out
{
    float4 position :   SV_POSITION;
    float3 normal   :   NOR;
    float2 texcoord :   TEX;
};

#define BLOCK_SIZE 8
StructuredBuffer<float3> Input : register(t0);
RWStructuredBuffer<float3> Data : register(u0);

float3 view_pos(float3 t, float3 pos)
{
    float3 worldpos = pos;

    float3 ro = 5.0 * float3(cos(time), 0.6*sin(2.*time), sin(time));
    float3 up = float3(0.0, 1.0, 0.0);
    float3 forward = -normalize(ro);
    float3 right = normalize(cross(forward, up));
    up = normalize(cross(right,forward));

    worldpos -= ro;
    float3 p;
    p.x = dot(worldpos, right);
    p.y = dot(worldpos, up);
    p.z = dot(worldpos, forward);
    worldpos = p;
    
    return worldpos;
}

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
    vs_out output;

    output.position = mul(float4(view_pos(time,input.position), 1.0f), projection);
    output.texcoord = input.texcoord;
    output.normal = input.normal;

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    return float4(0.5 * input.normal + 0.5,1.0);
}

[numthreads(BLOCK_SIZE, 1, 1)]
void cs_main(uint3 Gid : SV_GroupID,
    uint3 DTid : SV_DispatchThreadID,
    uint3 GTid : SV_GroupThreadID,
    uint GI : SV_GroupIndex)
{
    Data[DTid.x] = float3(DTid.x, 0.0, 0.0);
}