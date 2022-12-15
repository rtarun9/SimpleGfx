struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : Texture_Coord;
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


VSOutput VsMain(uint vertexID : SV_VertexID)
{
    static const float3 VERTEX_POSITIONS[3] = {float3(-1.0f, 1.0f, 0.0f), float3(3.0f, 1.0f, 0.0f), float3(-1.0f, -3.0f, 0.0f)};

    VSOutput output;
    output.position = float4(VERTEX_POSITIONS[vertexID], 1.0f);
    output.textureCoord = output.position * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    return output;
}

Texture2D<float4> albedoTexture : register(t0);
Texture2D<float4> positionTexture : register(t1);
Texture2D<float4> normalTexture : register(t2);

SamplerState wrapSampler : register(s0);

float4 PsMain(VSOutput input) : SV_Target
{
    float4 albedoColor = albedoTexture.Sample(wrapSampler, input.textureCoord);
    if (albedoColor.a < 0.2f)
    {
        discard;
    }

    float3 normal = normalTexture.Sample(wrapSampler, input.textureCoord).xyz;
    float3 viewSpacePixelPosition = positionTexture.Sample(wrapSampler, input.textureCoord).xyz;

    // Ambient lighting.
    const float ambientStrength = 0.05f;
    const float3 ambientColor = albedoColor.xyz * ambientStrength;

    float3 result = ambientColor;

    [unroll(5)]
    for (int i = 0; i < 5; ++i)
    {
        // Diffuse lighting.
        float3 pixelToLightDirection = normalize(viewSpaceLightPosition[i].xyz - viewSpacePixelPosition);
        float attenuation = 1.0f / (length(viewSpaceLightPosition[i].xyz - viewSpacePixelPosition));

        // If Directional light.
        if (i == 0)
        {
            pixelToLightDirection = normalize(viewSpaceLightPosition[i].xyz);
            attenuation = 1.0f;
        }

        const float diffuseStrength = max(dot(pixelToLightDirection, normal), 0.0f);
        const float3 diffuseColor = diffuseStrength * albedoColor.xyz;

        // Specular lighting.
        const float3 reflectionDirection = normalize(reflect(-pixelToLightDirection, normal));
        const float specularStrength = 0.2;

        const float3 viewDirection = normalize(-viewSpacePixelPosition);

        const float3 halfWayVector = normalize(pixelToLightDirection + viewDirection);

        const float specularIntensity = specularStrength * pow(max(dot(halfWayVector, normal), 0.0f), 64.0f);
        const float3 specularColor = specularIntensity * albedoColor.xyz;

        result +=  (diffuseColor + specularColor) * attenuation * lightColorIntensity[i].xyz * lightColorIntensity[i].w;
    }


    return float4(result, 1.0f);
}