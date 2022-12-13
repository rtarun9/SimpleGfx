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

float4 PsMain(VSOutput input) : SV_Target
{
    const float3 color = tex.Sample(wrapSampler, input.textureCoord).xyz;
    return float4(pow(color.rgb, 1/2.2f), 1.0f);
}
