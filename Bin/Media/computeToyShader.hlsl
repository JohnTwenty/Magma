RWTexture2D<float4> outputImage  : register(u0);




[numthreads(16, 16, 1)] //hardcoded to be used with 64x64 dispatch with 16x64 = 1024 ^2 OutputImage.
void main (uint3 threadId: SV_DispatchThreadID ) //threadId is 0 .. (numThreads.x * numBlocks.x)
	{
	uint2 outPixelCoord;
	float4 outColor = float4(threadId.x / (16.0*64.0) , threadId.y / (16.0*64.0), 0.8, 1.0);

	outPixelCoord.x = threadId.x * threadId.x;
	outPixelCoord.y = threadId.y;

	if (outPixelCoord.x < 1024)
		outputImage[outPixelCoord.xy] = outColor;
	}
	