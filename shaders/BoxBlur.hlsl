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

Texture2D<float> tex : register(t0);

SamplerState clampSampler : register(s0);
SamplerState wrapSampler : register(s1);

float PsMain(VSOutput input) : SV_Target
{
    float width;
    float height;

    tex.GetDimensions(width, height);
    
    // Perform simple average box blur of filter size 9x9.
    const float pixelSize = float2(1.0f / width, 1.0f / height);
    float sum = 0.0f;

    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            const float2 offset = float2(float(x), float(y)) * pixelSize;
            sum += tex.Sample(clampSampler, input.textureCoord).x;
        }
    }

    return sum / (25.0f);
}