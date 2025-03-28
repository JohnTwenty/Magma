#include "App\stdafx.h"
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


/*
* New experiment to test PhysX for deer animation
*/

using namespace physx;

static PxDefaultAllocator		gAllocator;
static PxDefaultErrorCallback	gErrorCallback;
static PxFoundation* gFoundation = NULL;
static PxPhysics* gPhysics = NULL;
static PxDefaultCpuDispatcher* gDispatcher = NULL;
static PxScene* gScene = NULL;
static PxMaterial* gMaterial = NULL;
static PxPvd* gPvd = NULL;
static PxRigidDynamic* gDynamic = NULL;

static PxRigidDynamic* createDynamic(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity = PxVec3(0))
{
	PxRigidDynamic* dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, 10.0f);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	gScene->addActor(*dynamic);
	return dynamic;
}


static void initPhysX()
{
	gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
	gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true, gPvd);
	PxSceneDesc sceneDesc(gPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	gDispatcher = PxDefaultCpuDispatcherCreate(2);
	sceneDesc.cpuDispatcher = gDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	gScene = gPhysics->createScene(sceneDesc);
	gMaterial = gPhysics->createMaterial(0.5f, 0.5f, 0.6f);

	PxRigidStatic* groundPlane = PxCreatePlane(*gPhysics, PxPlane(0, 1, 0, 0), *gMaterial);
	gScene->addActor(*groundPlane);
	gDynamic = createDynamic(PxTransform(PxVec3(0, 40, 100)), PxSphereGeometry(10));

}

static void tickPhysX()
{
	if (gFoundation == NULL)
		initPhysX();
	if (gScene)
	{
		gScene->simulate(1.0f / 60.0f);
		gScene->fetchResults(true);
	}

	//get xforms:


	/*PxU32 nbActors = gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
	if (nbActors)
	{
		PxArray<PxRigidActor*> actors(nbActors);
		scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(&actors[0]), nbActors);
		Snippets::renderActors(&actors[0], static_cast<PxU32>(actors.size()), true);
	}
	*/
}
void cmdVoxelGameShutdown()
{
	PX_RELEASE(gScene);
	PX_RELEASE(gDispatcher);
	PX_RELEASE(gPhysics);
	PX_RELEASE(gFoundation);

	foundation.printLine("VoxelGame cleaned up.\n");
}

void cmdVoxelGameTick(const char* scriptVar)	//scriptVar is the handle to constant buffer resource.
{

	tickPhysX();

	//This is just temp testing hack, we need a proper data driven asset instance system using ODS files.
	const unsigned nInstanceSlots = (1024 * sizeof(float)) / sizeof(Instance);	//We allocate this const size in floats in commands.ods.  Also hardcoded in drawWorldComputeShader.
	Instance consts[nInstanceSlots];//we should sizecheck allocated buffer somehow!  

	//we should sizecheck allocated buffer somehow!  We allocate 1024 / 8 = 128 in commands.ods.  Also hardcoded in drawWorldComputeShader.

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

	if (gDynamic)
	{
		PxTransform t = gDynamic->getGlobalPose();
		consts[6].position = t.p;
		consts[6].index = 6;
		consts[6].scaledOrientation = t.q;
		consts[6].scaledOrientation *= 0.5f;
	}
	else
	{
		consts[6].position = PxVec3(56.0f, -10.0f, 16.0f);
		consts[6].index = 6;
		consts[6].scaledOrientation = PxQuat(2 * 3.14159f * (gTime % 10000) * 0.0001f, PxVec3(0.0f, 1.0f, 0.0f));
		consts[6].scaledOrientation *= 0.5f;
	}



	//castle:
	consts[7].position = PxVec3(11.0f, -20.0f, 0.0f);
	consts[7].index = 7;
	consts[7].scaledOrientation = PxQuat(3.14159f/2.0f , PxVec3(1.0f, 0.0f, 0.0f));
	//consts[7].scaledOrientation *= 0.1f;

	//animating deer:
	float animSeconds = 4.0f * (gTime % 1000) * 0.001f;
	unsigned deerFrame = ((unsigned)animSeconds) % 4;
	consts[8].position = PxVec3(55.0f, -6.4f, 15.0f);
	consts[8].index = 8 + deerFrame;
	consts[8].scaledOrientation = PxQuat(3.14159f/2.0f , PxVec3(1.0f, 0.0f, 0.0f));
	consts[8].scaledOrientation *= 0.15f;

	//stress test -- add 100 more planet instances: 
	unsigned nPlanets = 100;

	for (unsigned i = 0; i < nPlanets; i++)
	{
		consts[9 + i].position = PxVec3(56.0f + (i / 10) * 10.0f, -10.0f, 16.0f + (i % 10) * 10.0f);
		consts[9 + i].index = 6;
		consts[9 + i].scaledOrientation = PxQuat(2 * 3.14159f * (gTime % 10000) * 0.0001f, PxVec3(0.0f, 1.0f, 0.0f));
		consts[9 + i].scaledOrientation *= 0.5f;
	}


	//sentinel for end of buffer:
	consts[nPlanets + 9].index = -1;

	unsigned nInstances = 10 + nPlanets;	//include sentinel!

	ASSERT(nInstances <= nInstanceSlots);

	//store as shader buffer:
	ResourceId rid = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));
	resourceManager.writeResource(rid, consts, sizeof(Instance) * nInstances);

}


void derivedAppRegisterCommands()
{
//commandManager.addCommand("voxelGameAssetMap", cmdVoxelGameAssetMap);
commandManager.addCommand("voxelGameTick", cmdVoxelGameTick);
commandManager.addCommand("voxelGameShutdown", cmdVoxelGameShutdown);
}
