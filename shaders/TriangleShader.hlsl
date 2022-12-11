struct VSInput
{
	float3 position : POSITION;
	float3 color : COLOR;
    float2 texCoords : TEX_COORD;
};

struct VSOutput
{
	float4 position : SV_Position;
	float3 color : COLOR;
    float2 texCoords : TEX_COORD;
};

VSOutput VsMain(VSInput input)
{
	VSOutput output;
	output.position = float4(input.position, 1.0f);
	output.color = input.color;
    output.texCoords = input.texCoords;

	return output;
}

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

float4 PsMain(VSOutput input) : SV_Target
{
	float4 output = float4(input.color, 1.0f) * tex.Sample(smp, input.texCoords);
	return output;
}