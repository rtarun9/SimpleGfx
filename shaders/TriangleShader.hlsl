struct VSInput
{
	float3 position : POSITION;
	float3 color : COLOR;
};

struct VSOutput
{
	float4 position : SV_Position;
	float3 color : COLOR;
};

VSOutput VsMain(VSInput input)
{
	VSOutput output;
	output.position = float4(input.position, 1.0f);
	output.color = input.color;

	return output;
}

float4 PsMain(VSOutput input) : SV_Target
{

	float4 output = float4(input.color, 1.0f);
	return output;
}