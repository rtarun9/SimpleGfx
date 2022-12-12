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
};

cbuffer sceneBuffer : register(b0)
{
    row_major matrix viewMatrix;
    row_major matrix viewProjectionMatrix;
    float3 pointLightColor;
    float padding;
    float3 pointLightPosition;
    float padding2;
    float3 cameraPosition;
    float padding3;
};

cbuffer transformBuffer : register(b1)
{
    row_major matrix modelMatrix;
    row_major matrix inverseModelMatrix;
};

VSOutput VsMain(VSInput input)
{
    VSOutput output;
    output.position = mul(mul(float4(input.position, 1.0f), modelMatrix), viewProjectionMatrix);

    return output;
}

float4 PsMain(VSOutput input) : SV_Target { return float4(pointLightColor, 1.0f); }