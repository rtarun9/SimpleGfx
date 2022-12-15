struct VSInput
{
    float3 position : POSITION;
    float2 textureCoord : TEXTURECOORD;
    float3 normal : NORMAL;
};

struct VSOutput
{
    float4 position : SV_Position;
    float4 colorIntensity : COLOR_INTENSITY;
};

cbuffer sceneBuffer : register(b0)
{
    row_major matrix viewMatrix;
    row_major matrix viewProjectionMatrix;

    float4 lightColorIntensity[5];
    float4 viewSpaceLightPosition[5];
};

cbuffer transformBuffer : register(b1)
{
    row_major matrix modelMatrix;
    row_major matrix inverseModelMatrix;
    row_major matrix inverseModelViewMatrix;
};

cbuffer instanceModelMatrixBuffer : register(b2) { row_major matrix modelMatrices[4]; };

VSOutput VsMain(VSInput input, uint instanceID : SV_InstanceID)
{
    VSOutput output;
    output.position = mul(mul(float4(input.position, 1.0f), modelMatrices[instanceID]), viewProjectionMatrix);
    output.colorIntensity = lightColorIntensity[instanceID];

    return output;
}

float4 PsMain(VSOutput input) : SV_Target { return float4(input.colorIntensity.xyz * input.colorIntensity.z, 1.0f); }