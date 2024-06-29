RWTexture2D<unorm float4> outputImage  : register(u0);
RWStructuredBuffer<float4> particles : register(u1);	//{pos.xy, vel.xy} [>64]
//**************
//RWTexture2D<float4> grid  : register(u2);				//{mass, velx, vely, }	//write doesn't work at all
//RWStructuredBuffer<float4> grid : register(u2);								//cannot do atomic add

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

//we have N particles i at xy coordinates particles[i].xy with velocity zw, so they all start at the origin with velocity zero
//we apply constant gravity, bounce constraint with static walls of container (image)
//and repulsive force such that they try to stay away from eachother.


[numthreads(64, 1, 1)]	//TODO: we can raise this again to e.g. 256, temporarily reduced for debugging
void main (uint3 threadId: SV_DispatchThreadID ) //threadId is 0 .. (numThreads.x * numBlocks.x)
	{
	float4 particleState = particles[threadId.x];

	float2 oldPos = particleState.xy;
	float2 velocity = float2(particleState.z, particleState.w + 0.6);	//gravity

	
	//advect using vel from grid
	uint2 gridCell = float2(oldPos.x / 16, oldPos.y / 16);
	uint gridIndex = gridCell.x + gridCell.y * 64;
	GCell g = grid[gridIndex];

	velocity += int2(g.velx, g.vely ) * 0.005; //g.vely = -1 works


	float2 newPos = particleState.xy + velocity * 0.01; //velocity integrate

	//position based bounce on ground - simple example constraint
	/*
	if (newPos.y > 500.0)
		{
		newPos.y = 499.0;
		velocity.y = -velocity.y;
		}
	if (newPos.x > 1000.0)
		{
		newPos.x = 999.0;
		velocity.x = -velocity.x;
		}
	*/
	//clip to box
	if (newPos.y < 0.0)
		{
		newPos.y = 0.0;
		velocity.y = 0.0;
		}
	if (newPos.x < 0.0)
		{
		newPos.x = 0.0;
		velocity.x = 0.0;
		}
	if (newPos.y > 1023.0)
		{
		newPos.y = 1023.0;
		velocity.y = 0.0;
		}
	if (newPos.x > 1023.0)
		{
		newPos.x = 1023.0;
		velocity.x = 0.0;
		}

	//recompute velocity
	//velocity = (newPos - oldPos) / 0.01;



  
	//write particle
	particles[threadId.x] = float4(newPos.x, newPos.y, velocity.x, velocity.y);	//write back

	//accumulate particle to grid - this is just nearest neighbor for now; let's say grid cells are 16x16
	if ((newPos.x >= 0) && (newPos.x < (16*64)) && (newPos.y >= 0) && (newPos.y < (16*64)))	//in grid domain check - we already did by clipping above.
		{
		uint2 gridCell = float2(newPos.x / 16, newPos.y / 16);

		//***********
		//uint status;
		//float4 gridValue = grid.Load(gridCell.xy, status);
		//float4 gridValue = grid[gridCell.xy];
		uint gridIndex = gridCell.x + gridCell.y * 64;
		//float4 gridValue = grid[gridIndex];
		//***********
		int2 ivelocity = int2(velocity.x * 1024.0, velocity.y * 1024.0);

		InterlockedAdd(grid[gridIndex].mass, 1.0);	//only works for integers without NVIDIA extension ... argh.  So we would need to make the grid into an integer type.

		//actually let's not put the particle velocitties into the grid so that the grid is pure density correction advection
		//InterlockedAdd(grid[gridIndex].velx, ivelocity.x);
		//InterlockedAdd(grid[gridIndex].vely, ivelocity.y);

		//gridValue.x += 1.0;
		//gridValue.y += velocity.x;
		//gridValue.z += velocity.y;

		//***********
		//grid[gridIndex] = gridValue;
		//grid[gridCell.xy] = gridValue;
		//***********

		//if (gridValue.x > 8.0)	//DEBUG
		{
			//draw particle
			uint2 rasterPosition = uint2(newPos.x, newPos.y);	//transform to screen - currently just trivially screen space is world space.
			if ((rasterPosition.x >= 0) && (rasterPosition.x < 1024) && (rasterPosition.y >= 0) && (rasterPosition.y < 1024))
				outputImage[rasterPosition] = float4(1.0, 0.0, 0.0, 0.0);
		}

		}



	}
	