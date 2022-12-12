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
    float3 viewSpaceNormal : NORMAL;
    float3 viewSpacePixelPosition : VIEW_SPACE_PIXEL_COORD;
};

cbuffer sceneBuffer : register(b0)
{
    row_major matrix viewMatrix;
    row_major matrix viewProjectionMatrix;
    float3 pointLightColor;
    float padding;
    float3 viewSpacePointLightPosition;
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
    output.textureCoord = input.textureCoord;

    const float3x3 transposedInverseModelMatrix = (float3x3)transpose(inverseModelMatrix);
    output.viewSpaceNormal = mul(input.normal, mul(transposedInverseModelMatrix, viewMatrix));

    output.viewSpacePixelPosition = mul(float4(input.position, 1.0f), mul(modelMatrix, viewMatrix)).xyz;

    return output;
}

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

float4 PsMain(VSOutput input) : SV_Target
{
    float4 albedoColor = tex.Sample(smp, input.textureCoord);
    if (albedoColor.a < 0.2f)
    {
        discard;
    }

    // Ambient lighting.
    const float ambientStrength = 0.2f;
    const float3 ambientColor = albedoColor.xyz * ambientStrength;

    // Diffuse lighting.
    const float3 pixelToLightDirection = normalize(viewSpacePointLightPosition - input.viewSpacePixelPosition);
    const float3 normal = normalize(input.viewSpaceNormal);

    const float diffuseStrength = max(dot(pixelToLightDirection, normal), 0.0f);
    const float3 diffuseColor = diffuseStrength * albedoColor.xyz;

    // Specular lighting.
    const float3 reflectionDirection = normalize(reflect(-pixelToLightDirection, normal));
    const float specularStrength = 0.5;

    const float3 viewDirection = normalize(-input.viewSpacePixelPosition);

    const float specularIntensity = specularStrength * pow(max(dot(reflectionDirection, viewDirection), 0.0f), 32.0f);
    const float3 specularColor = specularIntensity * albedoColor.xyz;

    return float4((diffuseColor + ambientColor + specularColor) * pointLightColor, 1.0f);
}