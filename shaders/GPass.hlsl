struct VSInput
{
    float3 position : POSITION;
    float2 textureCoord : TEXTURECOORD;
    float3 normal : NORMAL;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : TEX_COORD;
    float3 viewSpaceNormal : NORMAL;
    float3 viewSpacePixelPosition : VIEW_SPACE_PIXEL_COORD;
    float3x3 tbnMatrix : TBN_MATRIX;
};

struct PSOutput
{
    float4 albedo : SV_Target0;
    float4 position : SV_Target1;
    float4 normal : SV_Target2;
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

VSOutput VsMain(VSInput input)
{
    VSOutput output;
    output.position = mul(mul(float4(input.position, 1.0f), modelMatrix), viewProjectionMatrix);
    output.textureCoord = input.textureCoord;

    const float3x3 transposedInverseModelViewMatrix = (float3x3)transpose(inverseModelViewMatrix);
    output.viewSpaceNormal = normalize(mul(input.normal, transposedInverseModelViewMatrix));

    output.viewSpacePixelPosition = mul(float4(input.position, 1.0f), mul(modelMatrix, viewMatrix)).xyz;

    // Calculation of tbn matrix.
    const float3 normal = normalize(input.normal);

    // Generate tangent from normal.
    static const float MIN_FLOAT_VALUE = 0.00001f;

    float3 tangent = cross(normal, float3(0.0f, 1.0f, 0.0f));
    tangent = normalize(lerp(cross(normal, float3(1.0f, 0.0f, 0.0f)), tangent, step(MIN_FLOAT_VALUE, dot(tangent, tangent))));

    const float3 biTangent = normalize(cross(normal, tangent));

    const float3 t = normalize(mul(tangent, transposedInverseModelViewMatrix));
    const float3 b = normalize(mul(biTangent, transposedInverseModelViewMatrix));
    const float3 n = normalize(mul(normal, transposedInverseModelViewMatrix));

    output.tbnMatrix = float3x3(t, b, n);

    return output;
}

Texture2D<float4> albedoTexture : register(t0);
SamplerState albedoTextureSampler : register(s0);

Texture2D<float4> normalTexture : register(t1);
SamplerState normalTextureSampler : register(s1);

PSOutput PsMain(VSOutput input) 
{
    float4 albedoColor = albedoTexture.Sample(albedoTextureSampler, input.textureCoord);
    if (albedoColor.a < 0.2f)
    {
        discard;
    }

    uint normalTextureWidth;
    uint normalTextureHeight;

    normalTexture.GetDimensions(normalTextureWidth, normalTextureHeight);

    float3 normal = normalize(input.viewSpaceNormal);
    if (normalTextureWidth != 0)
    {
        normal = 2.0f * normalTexture.Sample(normalTextureSampler, input.textureCoord).xyz - float3(1.0f, 1.0f, 1.0f);
        normal = normalize(mul(normal, input.tbnMatrix));
    }

    // Ambient lighting.
    PSOutput output;
    output.position = float4(input.viewSpacePixelPosition, 1.0f);
    output.normal = float4(normal, 1.0f);
    output.albedo = albedoColor;


    return output;
}