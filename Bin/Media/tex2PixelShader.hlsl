
Texture2D textureObject;
SamplerState samplerObject
{
  Filter = MIN_MAG_MIP_POINT;	//D3D11_FILTER_MIN_MAG_MIP_LINEAR
  AddressU = Wrap;
  AddressV = Wrap;
};


//////////////
// TYPEDEFS //
//////////////
struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};



////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 main(PixelInputType input) : SV_TARGET
{

	return textureObject.Sample(samplerObject, input.tex);
}
