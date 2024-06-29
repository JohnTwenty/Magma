
RWStructuredBuffer<float4> particles : register(u1);	//{pos.xy, vel.xy}


//we have N particles i at xy coordinates particles[i].xy with velocity zw, so they all start at the origin with velocity zero
//we apply constant gravity, bounce constraint with static walls of container (image)
//and repulsive force such that they try to stay away from eachother.


[numthreads(64, 1, 1)]	//x is number of particles we write to buffer
void main (uint3 threadId: SV_DispatchThreadID ) //threadId is 0 .. (numThreads.x * numBlocks.x)
	{

	particles[threadId.x] = float4(32.0 + (threadId.x % 8) * 10.0,     0.0 + (threadId.x / 8) * 10.0 ,       0.0, 0.0);

	}
	