RWTexture2D<unorm float4> outputImage  : register(u0);
//**************
//RWTexture2D<float4> grid  : register(u2);				//{mass, velx, vely, }		//this is 64x64
//RWStructuredBuffer<float4> grid : register(u2);

struct GCell
{
	uint mass;
	int velx;
	int vely;
	uint unused;
};

RWStructuredBuffer<GCell> grid : register(u2);

//**************

cbuffer Constants : register(b0)	//set by cmdSetCommonPShaderConstants
{
	float viewportWidth;
	float viewportHeight;
	float time;						//time in seconds
	float padUnused;
	float4 camP;
	float4 camQ;
};


//stolen from shadertoy

float smoothstep(float edge0, float edge1, float x)
{
	float t;
	t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
	return t * t * (3.0 - 2.0 * t);
}


float sdSegment( float2 p, float2 a, float2 b )
{
    float2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h );
}

float sdArrow( float2 p, float2 a, float2 b )
{
    float sdl = sdSegment(p,a,b);
    float2 delta = normalize(b-a);
    //sdl = min(sdl, sdSegment(p,b,b-delta*0.05 + 0.05*delta.yx*float2(-1,1)));
    //sdl = min(sdl, sdSegment(p,b,b-delta*0.05 - 0.05*delta.yx*float2(-1,1)));
    return sdl;
}



[numthreads(16, 16, 1)] //hardcoded to be used with 64x64 dispatch with 16x64 = 1024 ^2 OutputImage.
void main (uint3 threadId: SV_DispatchThreadID ) //threadId is 0 .. (numThreads.x * numBlocks.x)
	{
	//blend on top of already drawn particles!
	uint2 gridCell = threadId.xy / 16;
	uint gridIndex = gridCell.x + gridCell.y * 64;

	uint2 inCellOffset = threadId.xy % 16;

	uint2 gridCellCenter = gridCell * 16 + 8;

	//**************
	//float isOccupied = (grid[gridIndex].x >= 1.0) ? 1.0 : 0.0;
	//float isOccupied = (grid[gridCell.xy].x >= 1.0) ? 1.0 : 0.0;
	//**************

	if ((gridCell.x < 64) && (gridCell.y < 64)) //bounds check -- not needed with just correctly sized frame buffer.
		{
		//green is mass
		//blue is cell shading


		//if I try to read the outputImage I get error: D3D11 ERROR: ID3D11DeviceContext::Dispatch: The resource return type for component 0 declared in the shader code (FLOAT) is not compatible with the resource type bound to Unordered Access View slot 0 of the Compute Shader unit (UNORM). This mismatch is invalid if the shader actually uses the view (e.g. it is not skipped due to shader code branching). [ EXECUTION ERROR #2097372: DEVICE_UNORDEREDACCESSVIEW_RETURN_TYPE_MISMATCH]
		GCell g = grid[gridIndex];
		uint gridMass = g.mass;
		float massColor = gridMass / 5.0;	//let's assume 10 is maximum occupancy
		float4 oldColor = outputImage[threadId.xy];
		//float4 oldColor = float4(0.0, 0.0, 0.0, 0.0);
		int2 gridVel = int2(g.velx, g.vely);

		//per grid velocity arrow:
		float arrow = smoothstep(0.03, 0.0, sdArrow(float2(threadId.xy), float2(gridCellCenter), float2(gridCellCenter)+20.0*float2(gridVel) ));


		outputImage[threadId.xy] = oldColor + float4(0.0, massColor, arrow, 0.0);	//(inCellOffset.x + inCellOffset.y)  / (16.0 + 16.0)
		}
	}
	