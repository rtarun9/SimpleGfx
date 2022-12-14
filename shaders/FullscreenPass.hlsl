struct VSOutput
{
    float4 position : SV_Position;
    float2 textureCoord : Texture_Coord;
};

VSOutput VsMain(uint vertexID : SV_VertexID)
{
    static const float3 VERTEX_POSITIONS[3] = {float3(-1.0f, 1.0f, 0.0f), float3(3.0f, 1.0f, 0.0f), float3(-1.0f, -3.0f, 0.0f)};

    VSOutput output;
    output.position = float4(VERTEX_POSITIONS[vertexID], 1.0f);
    output.textureCoord = output.position * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    return output;
}

Texture2D<float4> tex : register(t0);
SamplerState wrapSampler : register(s0);

float3 AcesNarkowicz(const float3 color)
{ // Aces tone mapping approximation : Narkowicz ACES model.
  // Reference : https://64.github.io/tonemapping/#aces

    float3 toneMappedColor = 0.6f * color;
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    
    return float3(clamp((toneMappedColor.r * (a * toneMappedColor.r + b)) / (toneMappedColor.r * (c * toneMappedColor.r + d) + e), 0.0f, 1.0f),
                  clamp((toneMappedColor.g * (a * toneMappedColor.g + b)) / (toneMappedColor.g * (c * toneMappedColor.g + d) + e), 0.0f, 1.0f),
                  clamp((toneMappedColor.b * (a * toneMappedColor.b + b)) / (toneMappedColor.b * (c * toneMappedColor.b + d) + e), 0.0f, 1.0f));
}

float4 PsMain(VSOutput input) : SV_Target
{
    float3 color = tex.Sample(wrapSampler, input.textureCoord).xyz;

    color = AcesNarkowicz(color);

    // Apply gamma correction and return result.
    return float4(pow(color.rgb, 1 / 2.2f), 1.0f);
}
