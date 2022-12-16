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

cbuffer ssaoBuffer : register(b1)
{
    row_major matrix projectionMatrix;
    float4 sampleVectors[64];
    float radius;
    float bias;
    float2 padding;
}; 

VSOutput VsMain(uint vertexID : SV_VertexID)
{
    static const float3 VERTEX_POSITIONS[3] = {float3(-1.0f, 1.0f, 0.0f), float3(3.0f, 1.0f, 0.0f), float3(-1.0f, -3.0f, 0.0f)};

    VSOutput output;
    output.position = float4(VERTEX_POSITIONS[vertexID], 1.0f);
    output.textureCoord = output.position * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
    return output;
}

Texture2D<float2> noiseTexture : register(t0);
Texture2D<float4> positionTexture : register(t1);
Texture2D<float4> normalTexture : register(t2);
Texture2D<float> depthTexture : register(t3);

SamplerState clampSampler : register(s0);
SamplerState wrapSampler : register(s1);

float PsMain(VSOutput input) : SV_Target
{
    float width;
    float height;

    positionTexture.GetDimensions(width, height);

    // The input.textureCoords vary from 0.0f to 1.0, but we want to tile the noise texture (which is only of size 4x4).

    const float2 noiseScale = float2((float)width / 4.0f, (float)height / 4.0f);

    const float3 normal = normalize(normalTexture.Sample(clampSampler, input.textureCoord).xyz);
    const float3 viewSpacePixelPosition = positionTexture.Sample(clampSampler, input.textureCoord).xyz;
    const float3 randomVector = float3(noiseTexture.Sample(wrapSampler, input.textureCoord * noiseScale).xy, 1.0f);

    // Calculate the TBN matrix so we can take tangent space sample vectors to view space.
    // Uses Gramm-Schmidt process for this.

    // Generate tangent from normal.
    static const float MIN_FLOAT_VALUE = 0.00001f;

    float3 tangent = cross(normal, float3(0.0f, 1.0f, 0.0f));
    tangent = normalize(lerp(cross(normal, float3(1.0f, 0.0f, 0.0f)), tangent, step(MIN_FLOAT_VALUE, dot(tangent, tangent))));

    const float3 biTangent = normalize(cross(normal, tangent));

    const row_major float3x3 tbn = float3x3(tangent, biTangent, normal);

    float occlusion = 0.0f;
    for (int i = 0; i < 64; ++i)
    {
        const float3 viewSpaceSamplePosition = mul(sampleVectors[i].xyz, tbn);
        const float3 samplePosition = viewSpacePixelPosition + viewSpaceSamplePosition * radius;

        // Transform sample position from view space to screen space.
        // Transform to clip space first.

        float4 offset = float4(samplePosition, 1.0f);
        offset = mul(offset, projectionMatrix);

        // Perspective divide + converting x and y to appropriate range of 0.0 to 1.0f.

        offset.xy = ((offset.xy / offset.w) * float2(1.0f, -1.0f)) * 0.5f + 0.5f;

        // From the depth texture, get the depth value of the closest pixel at the tex coords offset.xy.

        const float sampleDepth = depthTexture.Sample(clampSampler, offset.xy).x;
        
        // Required if the edges of the object are being taken into account : if the difference between sample depth and fragment position is very large, make that sample contribute very 
        // less towards occlusion factor.
        const float rangeCheck = smoothstep(0.0, 1.0, radius / abs(viewSpaceSamplePosition.z - sampleDepth));
        
        // If the sample depth is more than the sample position, it means at this point there is no occlusion as the sample position is more closer to the camera than the 
        // sample position in depth texture.
        occlusion += rangeCheck * step(sampleDepth, samplePosition.z - bias);
    }
    
    return 1.0f - (occlusion / 64.0f);
}