RWTexture2D<float4> outputImage  : register(u0);

cbuffer Constants : register(b0)	//set by cmdSetCommonPShaderConstants
{
	float viewportWidth;
	float viewportHeight;
	float time;						//time in seconds
	float padUnused;
	float4 camP;
	float4 camQ;
};



[numthreads(16, 16, 1)] //hardcoded to be used with 64x64 dispatch with 16x64 = 1024 ^2 OutputImage.  
void main (uint3 threadId: SV_DispatchThreadID ) //threadId is 0 .. (numThreads.x * numBlocks.x)
	{
	float4 outColor = float4(threadId.x / (16.0*64.0) , threadId.y / (16.0*64.0), sin(time), 1.0);

	uint2 outPixelCoord;
	//This simple compute shader gets to choose which pixel(s) it writes from this thread, which is the major benefit over pixel shaders.
	//You still cannot read the outputImage as it has probably been bound as a write only resource.
	outPixelCoord.x = threadId.x * threadId.x;
	outPixelCoord.y = threadId.y;

	if (outPixelCoord.x < viewportWidth)
		outputImage[outPixelCoord.xy] = outColor;


	}
	