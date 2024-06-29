
/////////////
// GLOBALS //
/////////////

cbuffer MatrixBuffer
{
	float4x4 xform;
};


//////////////
// TYPEDEFS //
//////////////
struct VertexInputType
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};


////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PixelInputType main(VertexInputType input)
{
    PixelInputType output;
	float4 homopos = float4(input.position.x, input.position.y, input.position.z, 1);
	output.position = mul(xform, homopos);
    output.color = input.color;
    
    return output;
}