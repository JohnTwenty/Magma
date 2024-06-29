//***********
//RWTexture2D<float4> grid  : register(u2);				//{mass, velx, vely, ??? }
//RWStructuredBuffer<float4> grid : register(u2);

struct GCell
{
	uint mass;
	int velx;
	int vely;
	uint unused;
};

RWStructuredBuffer<GCell> grid : register(u2);

//***********


[numthreads(32, 32, 1)]		//launch this 2x2 to clear 64x64 grid
void main (uint3 threadId: SV_DispatchThreadID ) 
	{
	//***********
	//grid[threadId.xy] = float4(0.0, 0.0, 0.0, 0.0);
	//grid[threadId.x + threadId.y * 64] = float4(0.0, 0.0, 0.0, 0.0);
	 
	uint index = threadId.x + threadId.y * 64;
	 grid[index].mass = 0;
	 //don't clear mass because we need to advect it 
	 //grid[index].velx = 0;
	 //grid[index].vely = 0;

	//***********

	}
	