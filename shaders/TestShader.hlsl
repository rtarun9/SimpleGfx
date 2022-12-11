struct VSInput
{
    float3 position : POSITION;
    float2 textureCoord : TEXTURECOORD;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float3 biTangent : BITANGENT;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEX_COORD;
};

cbuffer sceneBuffer : register(b0) { row_major matrix viewProjectionMatrix; }
cbuffer transformBuffer : register(b1)
{
    row_major matrix modelMatrix;
    row_major matrix inverseModelMatrix;
};

VSOutput VsMain(VSInput input)
{
    VSOutput output;
    output.position = mul(mul(float4(input.position, 1.0f), modelMatrix), viewProjectionMatrix);
    output.textureCoord = input.textureCoord;

    return output;
}

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

float4 PsMain(VSOutput input) : SV_Target
{
    float4 output = tex.Sample(smp, input.textureCoord);
    if (output.a < 0.2f)
    {
        discard;
    }

    return output;
}