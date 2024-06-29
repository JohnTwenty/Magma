#include "stdafx.h"
#include "Foundation\CommandManager.h"
#include "Foundation\VariableManager.h"
#include "Resource\ResourceManager.h"

#pragma warning(push)
#undef free
#undef realloc
#pragma warning( disable : 4365) //C4365: 'argument' : conversion from 'unsigned int' to 'int'
#pragma warning( disable : 4061) //warning C4061: enumerator 'CompressionMask' in switch of enum
#include "PxPhysicsAPI.h"
using namespace physx;
#pragma warning(pop)


extern unsigned gTime;	//milliseconds since the SDL library initialized.


struct Instance
{
	PxVec3 position;
	int    index;
	PxQuat scaledOrientation;
};



void cmdVoxelGameTick(const char* scriptVar)
{

	//This is just temp testing hack, we need a proper data driven asset instance system using ODS files.
	Instance consts[12];//we should sizecheck allocated buffer somehow!  We allocate 128 / 8 = 16 in commands.ods.  Also hardcoded in drawWorldComputeShader.

	//terrain:
	consts[0].position = PxVec3(0.0f, -20.0f, 0.0f);
	consts[0].index = 0;
	consts[0].scaledOrientation = PxQuat(PxIdentity);

	consts[1].position = PxVec3(32.0f, -20.0f, 0.0f);
	consts[1].index = 1;
	consts[1].scaledOrientation = PxQuat(PxIdentity);

	consts[2].position = PxVec3(64.0f, -20.0f, 0.0f);
	consts[2].index = 2;
	consts[2].scaledOrientation = PxQuat(PxIdentity);


	consts[3].position = PxVec3(0.0f, -20.0f, 32.0f);
	consts[3].index = 3;
	consts[3].scaledOrientation = PxQuat(PxIdentity);

	consts[4].position = PxVec3(32.0f, -20.0f, 32.0f);
	consts[4].index = 4;
	consts[4].scaledOrientation = PxQuat(PxIdentity);

	consts[5].position = PxVec3(64.0f, -20.0f, 32.0f);
	consts[5].index = 5;
	consts[5].scaledOrientation = PxQuat(PxIdentity);

	//planet:
	consts[6].position = PxVec3(56.0f, -10.0f, 16.0f);
	consts[6].index = 6;
	consts[6].scaledOrientation = PxQuat(2 * 3.14159f * (gTime % 10000)*0.0001f, PxVec3(0.0f, 1.0f, 0.0f));
	consts[6].scaledOrientation *= 0.5f;

	//castle:
	consts[7].position = PxVec3(11.0f, -20.0f, 0.0f);
	consts[7].index = 7;
	consts[7].scaledOrientation = PxQuat(3.14159f/2.0f , PxVec3(1.0f, 0.0f, 0.0f));
	//consts[7].scaledOrientation *= 0.1f;

	//deer frame 0:
	float animSeconds = 4.0f * (gTime % 1000) * 0.001f;
	unsigned deerFrame = ((unsigned)animSeconds) % 4;
	consts[8].position = PxVec3(55.0f, -6.4f, 15.0f);
	consts[8].index = 8 + deerFrame;
	consts[8].scaledOrientation = PxQuat(3.14159f/2.0f , PxVec3(1.0f, 0.0f, 0.0f));
	consts[8].scaledOrientation *= 0.15f;

	//deer frame 1:
	consts[9].position = PxVec3(22.0f, -10.0f, 5.0f);
	consts[9].index = 9;
	consts[9].scaledOrientation = PxQuat(PxIdentity);
	consts[9].scaledOrientation *= 0.15f;

	//deer frame 2:
	consts[10].position = PxVec3(22.0f, -10.0f, 10.0f);
	consts[10].index = 10;
	consts[10].scaledOrientation = PxQuat(PxIdentity);
	consts[10].scaledOrientation *= 0.15f;

	//deer frame 3:
	consts[11].position = PxVec3(22.0f, -10.0f, 15.0f);
	consts[11].index = 11;
	consts[11].scaledOrientation = PxQuat(PxIdentity);
	consts[11].scaledOrientation *= 0.15f;

	//store as shader buffer:
	ResourceId rid = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));
	resourceManager.writeResource(rid, consts, sizeof(Instance) * 12);

}


void derivedAppRegisterCommands()
{
//commandManager.addCommand("voxelGameAssetMap", cmdVoxelGameAssetMap);
commandManager.addCommand("voxelGameTick", cmdVoxelGameTick);

}
