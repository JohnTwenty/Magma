
/////////////
// GLOBALS //
/////////////

cbuffer MatrixBuffer
{
	float4x4 posTransform;
};


//////////////
// TYPEDEFS //
//////////////
struct VertexInputType
{
    float3 position : POSITION;
	float2 tex : TEXCOORD0;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
};


////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PixelInputType main(VertexInputType input)
{
    PixelInputType output;
	float4 homopos = float4(input.position.x, input.position.y, input.position.z, 1);
	output.position = mul(posTransform, homopos);
	output.tex = input.tex;
    
    return output;
}