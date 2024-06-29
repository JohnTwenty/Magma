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


int2 processNeighbor(int2 offs, uint2 cCoord, uint myMass)
{
	int2 nCoord = cCoord + offs;

	
	if (nCoord.x >= 0 && nCoord.x < 64 && nCoord.y >= 0 && nCoord.y < 64)	//bounds check
	{
		uint nIndex = (cCoord.x + offs.x) + (cCoord.y + offs.y) * 64;
		GCell nCell = grid[nIndex];

		if (nCell.mass < myMass)
		{
			//we can send some mass toward this neighbor
			int massDiff = myMass - nCell.mass;
			return offs * massDiff;	//caution, this is not normalized which will likely cause artifacts!

		}

	}

	return int2(0, 0);

}

[numthreads(32, 32, 1)]		//launch this 2x2 to clear 64x64 grid
void main (uint3 threadId: SV_DispatchThreadID )	//threadId.xy is grid coordinate.
	{
	uint index = threadId.x + threadId.y * 64;
	GCell cell = grid[index];

	//is this cell over max density? -> basically detect density constraint violation
	//let's say 3 is max density
	if (cell.mass > 2)	//caution - this will make things nonsmooth?
	{
		int2 velDelta = int2(0, 0);
		//process all 8 neighbors - it may be smart to order these top to bottom
		velDelta += processNeighbor(int2(-1, -1), threadId, cell.mass);
		velDelta += processNeighbor(int2( 0, -1), threadId, cell.mass);
		velDelta += processNeighbor(int2( 1, -1), threadId, cell.mass);

		velDelta += processNeighbor(int2(-1, 0), threadId, cell.mass);
		velDelta += processNeighbor(int2( 1, 0), threadId, cell.mass);

		velDelta += processNeighbor(int2(-1, 1), threadId, cell.mass);
		velDelta += processNeighbor(int2( 0, 1), threadId, cell.mass);
		velDelta += processNeighbor(int2( 1, 1), threadId, cell.mass);

		cell.velx += velDelta.x;
		cell.vely += velDelta.y;
	}
	else
	{
		cell.velx = 0;
		cell.vely = 0;
	}
		//write back - we never clear the grid so we need to clear it to zero even if we didn't induce a velocity!
		grid[index] = cell;
	//
	// grid[index].mass = 0;
	// grid[index].velx = 0;
	// grid[index].vely = 0;


	}
	