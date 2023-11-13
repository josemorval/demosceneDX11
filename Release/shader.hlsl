//--------------------------------------------------------------------------------------
// Constant Buffers.
//--------------------------------------------------------------------------------------

cbuffer global_shader_constants : register(b0) 
{
    float4x4 projection_matrix;
    float4x4 view_matrix;
    float time;
};

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
    float2 texcoord :   TEX;
};

VS_OUT vs_main(VS_IN input)
{
    VS_OUT output;

    output.position = float4(input.position, 1.0);
    output.position.y += sin(0.5 * output.position.x + time) * sin(0.7 * output.position.z + 1.1*time);
    output.position = mul(projection_matrix,mul(view_matrix, output.position));
    output.texcoord = input.texcoord;
    output.normal = input.normal;

    return output;
}

float4 ps_main(VS_OUT input) : SV_TARGET
{
    return float4(0.5 * input.normal + 0.5,1.0);
}