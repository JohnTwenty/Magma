
interface VolTex
{
	int f(in float3 p);
};

class VolTexClamp : VolTex
{
	int f(in float3 p)
	{
		return map.Load(int4(p.x, p.y, p.z, 0));//last coord is mip level. 
	}
};

class VolTexArray : VolTex
{
	int f(in float3 p)
	{
		return map.Load(int4(p.x, p.y, p.z, 0));//last coord is mip level. 
	}
};


class VolTexWrap : VolTex
{
	int f(in float3 p)
	{
		return map.Load(int4(abs(mod(p.x, 32.0)), abs(mod(p.y, 32.0)), abs(mod(p.z, 32.0)), 0));//last coord is mip level.  32 is texture size.  we should not hardcode that.
	}

};



//this one also traces into voxel blocks
float raymarchVoxVol(float3 rayOriginW, float3 rayDirW, int numSteps, float maxt, out float3 currCellG, out float4 hitNormalW, out int material)	//hit normal w is index of nonzero component.
{
	//DDA line through world space in visitation order without diagonal skips.
	float3 rayDirSign = sign(rayDirW);

	float3 rayInvW = 1.0 / rayDirW;

	float3 rayDirStep = max(rayDirSign, float3(0.0, 0.0, 0.0));//0 or 1 not -1 or 1

	//find starting grid cell
	//float3 
	currCellG = floor(rayOriginW);//(0,1)

	//visit the cell

	[loop] for (int i = 0; i < numSteps; i++)
	{
		//what is next cell?  
		//line equation -- what are the 2 range values when domain value (axis of largest extent) increases (or decreases) by 1.0?
		//value=line(theta)=
		//rayOriginW + theta*rayDirW = currCellG + rayDirStep
		//solve that scalar equation on all three axes for the theta where we advance to next grid incercept.

		float3 theta = (currCellG - rayOriginW + rayDirStep) * rayInvW;
		//find smallest positive theta axis -- that is the grid intercept happening next on line
		int minThetaIndex = (abs(theta.x) > abs(theta.y)) ? 1 : 0;
		//minThetaIndex = (abs(theta[minThetaIndex]) > abs(theta.z)) ? 2 : minThetaIndex;

		hitNormalW = (abs(theta.x) > abs(theta.y)) //hit normal in xyz, nonzero element index is in w
			?
			((abs(theta.z) > abs(theta.y)) ? float4(0.0, -rayDirSign[1], 0.0, 1.0) : float4(0.0, 0.0, -rayDirSign[2], 2.0))
			:
			((abs(theta.z) > abs(theta.x)) ? float4(-rayDirSign[0], 0.0, 0.0, 0.0) : float4(0.0, 0.0, -rayDirSign[2], 2.0))
			;


		//move to neighbor
		currCellG -= hitNormalW.xyz;

		//VolTexWrap map;
		//VolTexClamp map;
		//material = map.f(currCellG, 1);
		VolTexArray map;
		material = map.f(currCellG);

		float t = theta[int(hitNormalW.w)];
		if (t > maxt)
		{
			currCellG = float3(0.0, 0.0, 0.0);
			hitNormalW = float4(0.0, 0.0, 0.0, 0.0);
			return 100000.0;	//inf
		}

		if (material > 0)
		{

			//do this branchless.
			//for texturing: compute world hit pos.
			//float3 hitPointW = rayOriginW + rayDirW * t;	//world pos

			return t;		//hit opaque block
		}
	}


	currCellG = float3(0.0, 0.0, 0.0);
	hitNormalW = float4(0.0, 0.0, 0.0, 0.0);
	return 100000.0;	//inf


}

