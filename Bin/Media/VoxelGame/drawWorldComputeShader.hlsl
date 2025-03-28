#include "../utils.hlsl"

Texture3D<uint> map  : register(t0);	//we use 8bpp textures but that isn't supported by HLSL so its upcast to an integer.

#include "../raytrace2.hlsl"

RWTexture2D<float4> outputImage  : register(u0);
//RWStructuredBuffer<float3> lighting : register(u1);	//3 faces per voxel (the 3 other faces belong to the next voxels)

//TODO: this needs to be dynamic somehow
#define NUM_INSTANCES 128			//must match number of instances in VoxelGame.cpp and cbuffer size declaration VoxelGame.Commands.ods
#define NUM_ASSETS 12

cbuffer Constants : register(b0)	//set by cmdSetCommonPShaderConstants
{
	float viewportWidth;
	float viewportHeight;
	float time;
	float padUnused;
	float4 camP;
	float4 camQ;
};

struct Instance
{
	float3 position;
	int    index;
	float4 scaledOrientation;
};

//cbuffer Instances : register(b1)
//{
//	Instance instances[NUM_INSTANCES];
//};

RWStructuredBuffer<Instance> instances : register(u2);  

struct AssetLocation	
{
	uint3 start;
	uint pack0;	//has to be multiples of vec4 for padding.
	uint3 dims;
	uint pack1;
};

cbuffer AssetMap : register(b2)
{
	AssetLocation assetMap[NUM_ASSETS];
};



float3 quatRotate(float4 q, float3 v)
{

	float vx = 2.0*v.x;
	float vy = 2.0*v.y;
	float vz = 2.0*v.z;
	float w2 = q.w*q.w-0.5;
	float dot2 = (q.x*vx + q.y*vy +q.z*vz);
	return float3
	(
		(vx*w2 + (q.y * vz - q.z * vy)*q.w + q.x*dot2), 
		(vy*w2 + (q.z * vx - q.x * vz)*q.w + q.y*dot2), 
		(vz*w2 + (q.x * vy - q.y * vx)*q.w + q.z*dot2)
	);

}


// axis aligned box centered at the origin, with size halfBounds; https://iquilezles.org/articles/intersectors/
bool rayVsAABB( float3 ro, float3 rd, float3 halfBounds, out float tnear, out float tfar) 
{
    float3 m = 1.0/rd;
    float3 n = m*ro;
    float3 k = abs(m)*halfBounds;
    float3 t1 = -n - k;
    float3 t2 = -n + k;
    tnear = max( max( t1.x, t1.y ), t1.z );//t near clip
    tfar = min( min( t2.x, t2.y ), t2.z );//t along ray far clip
    return (!(tnear > tfar || tfar < 0.0));
}




[numthreads(16, 16, 1)] //hardcoded to be used with 64x64 dispatch with 16x64 = 1024 ^2 OutputImage.
void main (uint3 threadId: SV_DispatchThreadID ) 
	{

    float3 fogColor = float3(0.1, 0.2, 0.6);
	float3 uv = float3(threadId.x/viewportWidth, 1-(threadId.y/viewportHeight), 1) - 0.5;
 
    float2 iMouse = float2(0.0,0.0);	//todo: add as input
    float2 mouse = float2(iMouse.x/viewportWidth, iMouse.y/viewportHeight) - float2(0.5, 0.5);
    
    float zNear = 1.0;

	//DISABLED TXAA: 
	//jitter ray a bit to feed different subpixel samples into temporal anti alias
	//the magnitude of this needs to be tuned pretty exactly!! Best way to do this is to visualize frame by frame diffs.
	float r0 = 0.0; // (nrand(float2(time + 13.3, mouse.x)) - 0.5) * 0.0004;
	float r1 = 0.0; // (nrand(float2(time + 21.7, mouse.y)) - 0.5) * 0.0004;

    //camera space ray, not normalized
    float3 rayDirC = float3(uv.x+r0, uv.y+r1, zNear);        
	float3 C2W_p = camP.xyz;

	//xform ray from cam space to world space:
	float3 rayDirWorld = quatRotate(camQ, rayDirC);  
    float3 rayOriginWorld = C2W_p;

	float3 sunDirW = normalize(float3(0.6, 0.7, 0.5));
	float skyDebug = 0.0;
	float t = 10000.0;
	float3 currCellG;
	float3 hitPointL;
	float4 hitNormalLi;
	float3 hitNormalW;
	int material;




	[loop] for (int inst = 0; inst < NUM_INSTANCES; inst++)	//instances of voxel assets
	{
		int assetIndex = instances[inst].index;	//the cost of this shader is dominated by the cost of they dynamic read access of instances.

		if (assetIndex < 0)
			break;

		float scale = length(instances[inst].scaledOrientation);
		float invScale = 1.0 / scale;
		float4 voxRotQuat = instances[inst].scaledOrientation * invScale;	//unpack quat from compressed data
		float3 voxPos = instances[inst].position;	//position of the instance

		//xform world space ray to voxel local space:
		float3 rayDirL = quatRotate(voxRotQuat, rayDirWorld);  //need to rotate in opposite direction of quat
		rayDirL = normalize(rayDirL);	//need to normalize for this call.

		float3 rayOriginL = quatRotate(voxRotQuat, rayOriginWorld - voxPos);

		float tnear = 0.0;
		float tfar = 0.0;


		//decide if ray hits voxvol local space BOUNDS / world space OBB
		bool boxhit = rayVsAABB(rayOriginL, rayDirL, assetMap[assetIndex].dims * 0.5 * scale, tnear, tfar);
		if (boxhit == 1.0)
		{
			skyDebug = 1.0;
			//clip the ray to the box:
			tnear -= 0.01f; //fudge to avoid clipping away the first pixel.
			tnear = max(tnear, 0.0);	//tnear is negative if we're inside the box, don't clip in that case!
			float3 rayOriginClipL = rayOriginL + rayDirL * tnear;
			//recompute tfar:
			tfar = tfar - tnear;
			//bias it a bit so we don't have artifacts at edges:  TODO: this is a bit tricky because it will either cause bleedover into nearby tiles or voxels being clipped!!
			tnear += 0.1f;
			tfar -= 0.1f;


			//trace vs dynamics
			//scale the raster inside the bounds
			rayOriginClipL *= invScale;//inverse scale -> larger to make it smaller
			float3 rayDirClipL = rayDirL * invScale;	//the ray needs to scale!


			rayOriginClipL += assetMap[assetIndex].dims * 0.5 + assetMap[assetIndex].start;	//the calling domain is [-dims/2..dims/2], the map is 0..dims, so this is to center it.
			//also add on the start of the sub asset in the asset map

			//trace vs map
			float3 currCellGhit;
			float4 hitNormalInstLi;	//w component has 'element index'
			int materialhit;

			//local space ray march through asset
			tfar = min(t, tfar); //do not trace beyond closest hit so far.
			float thit = raymarchVoxVol(rayOriginClipL, rayDirClipL, 250, tfar, currCellGhit, hitNormalInstLi, materialhit);//ray from eye into world.  Doesn't have to be normalized for this call.
			if (thit + tnear < t)//closer than last instance hit?
			{
				t = thit + tnear;	
				currCellG = currCellGhit;
				hitNormalLi = hitNormalInstLi;
				hitNormalW = quatRotate(float4(-voxRotQuat.x, -voxRotQuat.y, -voxRotQuat.z, voxRotQuat.w),hitNormalInstLi.xyz);
				material = materialhit;
				hitPointL = rayOriginL + rayDirL * t;
			}

		}
	}

	if (t < 10000.0) //note this is also a local space metric so we should not be doing an absolute compare without rescaling in general! But this should just be an infinity test.
	{

		float3 mpos = (hitPointL - floor(hitPointL)) * 16;				//in-cell coord in tex coord space.	

		float3 xcolor;
		getMaterialColor(material, float2(32., 32.) - mpos.zy, xcolor);//16x16 tex coords needed. 32 to 32+16 is grassy side.

		float3 ycolor;//top of block
		getMaterialColor(material, mpos.xz, ycolor);

		float3 zcolor;
		getMaterialColor(material, float2(32., 32.) - mpos.xy, zcolor);


		float3 albedo = abs(hitNormalLi[0]) * xcolor + abs(hitNormalLi[1]) * ycolor + abs(hitNormalLi[2]) * zcolor;	//branchless way to do this though it means fetching 3 different materials...not sure about tradeoff
		//float3 albedo = float3(0.8, 0.8, 0.8);	//uncomment to ignore texture

		//enable for shadow

		float3 sunColor = float3(1.64, 1.27, 0.99);	//in direct sunlight
		float3 p;
		float4 n;

		int3 currCellGi = int3(abs(mod(currCellG.x, 32.0)), abs(mod(currCellG.y, 32.0)), abs(mod(currCellG.z, 32.0)));
		//int3 currCellGi = int3(abs(currCellG))& 31; //the cell that we've hit
		//if normal component is nonnegative, the light info is with the next cell
		int isPositive = (hitNormalLi.x + hitNormalLi.y + hitNormalLi.z > 0); //only one elem is nonzero -- this is a bit ugly but works.
		currCellGi += int3(hitNormalLi.xyz) * isPositive;

		int voxOffset = (currCellGi.z * 32 * 32 + currCellGi.y * 32 + currCellGi.x) * 3;

		//skip lightmap, its disabled for now..
		//float3 lightMapColor = lighting[voxOffset + int(hitNormalLi.w)];
		float3 ambientColor = float3(0.2, 0.3, 0.4);


		//TODO: add sky light terms.
		float3 incomingLight =
			sunColor * max(dot(sunDirW, hitNormalW),0.0)		//diffuse lighting by sun light -- since the hit normal is one of 6 different discrete values, we could just precompute this for 6 different normals and make it more interesting than a single color.
			+
			ambientColor
			;

		//how can we improve on lighting here?
		// 1. we remember which asset we hit where, so we could go back to asset local space and just trace some ambient occlusion rays / do local occlusion estimation just for that asset.  That will not properly occlude areas where assets touch though.
		// 2. possibly shadowed sun: we could trace a ray to the sun, but that could hit any of the assets so we need to check all of them again.  Even if we have AABB early outs. Can use noisy sun direction for noisy result that we can filter into soft area sun.
		// 3. possibly shadowed sky: we can try to coarsely estimate this with random rays into upper hemisphere but its a cost of N times sun ray for N samples.  :(  
		// 4. direct lighting from other lights - RESTIR madness? - brute force is like sun only one ray per light. :( 

		//albedo = pow( albedo, float3(2.2,2.2,2.2) );//inverse gamma on materials because I suspect they have baked gamma.

		float3 lightToEye = albedo * incomingLight;	//light reflected to camera from our diffuse surface

		lightToEye = lerp( lightToEye, fogColor, 1.0-exp(-0.000001*t*t*t) );//this light goes through a volume of fog to reach eye

		// saturate and gamma correction for display
		//no -- we will do gamma after txaa
		//lightToEye = pow( saturate(lightToEye), float3(1.0/2.2,1.0/2.2,1.0/2.2) );
		lightToEye = saturate(lightToEye);

		//TXAA: For this to work well we have to be reading/writing to a high precision buffer!  Otherwise we get artifacts!!!  At least 16 bit / channel seems to work.
		//for proper TAA we would need to:
		//1) use motion vectors to sample from a better position than this exact pixel.
		//2) filter nearby old image pixels, not just take a single sample!  
		//but that needs double buffering ofc!
		//then, we need to REJECT the old color in cases where its too far from the new color(s).  Best way to do this seems to involve multiple current samples to know what range of colors still make sense.  
		//if we end up rejecting history, we can optionally shoot more samples in this frame to not show unconverged 

		//also, isn't it a problem that we use DXGI_SWAP_EFFECT_FLIP_DISCARD  / DXGI_SWAP_EFFECT_DISCARD swap chains yet we read from the back buffer again?  That seems to work but is questionable.
		//will need motion vectors: https://www.shadertoy.com/view/WlSSWc

		outputImage[threadId.xy] = float4(lightToEye.xyz, 1.0) ;// * 0.1 + outputImage[threadId.xy] * 0.9;	//temporal AA

	}	
	else 
		outputImage[threadId.xy] = float4(fogColor, 1.0) * skyDebug;// * 0.1 + outputImage[threadId.xy] * 0.9;	//temporal AA
	//smallest possible increment is about 1 part in 500, else it gets rounded to 0:
	//outputImage[threadId.xy] = outputImage[threadId.xy] + float4(0.0021,0.0,0.0, 1.0);	//test for accumulative precision
	//outputImage[threadId.xy] = outputImage[threadId.xy] + float4(0.001, 0.0, 0.0, 1.0);	//test for accumulative precision

	}
	