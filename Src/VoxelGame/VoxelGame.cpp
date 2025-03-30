#include "App\stdafx.h"
#include "Foundation\CommandManager.h"
#include "Foundation\VariableManager.h"
#include "Resource\ResourceManager.h"
#include "Foundation\FileManager.h"
#include "Foundation\StringManager.h"
#include "Foundation\ODBlock.h"

#pragma warning(push)
#undef free
#undef realloc
#pragma warning( disable : 4365) //C4365: 'argument' : conversion from 'unsigned int' to 'int'
#pragma warning( disable : 4061) //warning C4061: enumerator 'CompressionMask' in switch of enum
#include "PxPhysicsAPI.h"
using namespace physx;
#pragma warning(pop)


extern unsigned gTime;	//milliseconds since the SDL library initialized.


struct SceneInstance
{
	PxVec3 position;
	unsigned assetIndex;
	unsigned currentFrameIndex;
	PxQuat scaledOrientation;
};
#define MAX_INSTANCES 128	//this has to match the size of the instance buffer in the compute shader and in VoxelGame.Commands.ods TODO: make dynamic somehow.
SceneInstance sceneInstances[MAX_INSTANCES];
static unsigned numSceneInstances = 0;



//this is the instance buffer that gets written to the compute shader, and should be frustum culled.
struct DrawInstance
{
	PxVec3 position;
	int    volumeAtlasDrawIndex;	//index into the volume atlas - basically the frame of the asset to draw.  Also used as end of array sentinel when -1.
	PxQuat scaledOrientation;
};
DrawInstance drawInstances[MAX_INSTANCES];
static unsigned numDrawInstances = 0;


/*
* New experiment to test PhysX
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
static ResourceId tmpVoxMap = 0;	//dirty way of passing the voxmap resource id to the iterateFiles callback. 

struct Asset
{
	Asset(unsigned name, unsigned volumeAtlasFirstFrameIndex, unsigned numFrames) : name(name), volumeAtlasFirstFrameIndex(volumeAtlasFirstFrameIndex), numFrames(numFrames) {}
	unsigned name;	//name handle in StringManager
	unsigned volumeAtlasFirstFrameIndex;	//voxvol specific index
	unsigned numFrames;
};

Array<Asset, Allocator> assets;


static PxRigidDynamic* createDynamic(const PxTransform& t, const PxGeometry& geometry, const PxVec3& velocity = PxVec3(0))
{
	PxRigidDynamic* dynamic = PxCreateDynamic(*gPhysics, t, geometry, *gMaterial, 10.0f);
	dynamic->setAngularDamping(0.5f);
	dynamic->setLinearVelocity(velocity);
	gScene->addActor(*dynamic);
	return dynamic;
}


static void cmdVoxelGameInitPhysics()
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

static void parseAsset(ODBlock & odBlock, const char * name) 
{
	/*
	Asset
	{
	voxvol { "castle.magica.voxvol";}	
	physics
		{
		collider { voxel; }
		}
	}
	*/
	const char * voxvol;
	odBlock.getBlockString("voxvol", &voxvol);
	foundation.printLine("  voxvol:", voxvol);
	ODBlock * physicsBlock = odBlock.getBlock("physics");
	if (physicsBlock)
		{
		const char * collider;
		physicsBlock->getBlockString("collider", &collider);
		foundation.printLine("  collider:", collider);
		}	

	unsigned firstFrameIndex = 0;
	unsigned numFrames = resourceManager.loadVolumeIntoAtlas(tmpVoxMap, voxvol, firstFrameIndex);

	assets.pushBack(Asset(stringManager.addString(name), firstFrameIndex, numFrames));
	Asset & a = assets.back();
	foundation.printf("  asset: %s, firstFrameIndex: %u, numFrames: %u\n", stringManager.lookupString(a.name), a.volumeAtlasFirstFrameIndex, a.numFrames);	
}

static void loadAsset(const char* filename) 
{
    foundation.printLine("Loading asset:", filename);
	size_t length;
	const void* buffer = fileManager.loadFile(filename, &length, true);

	ODBlock odBlock;	//points into buffer
	bool ok = odBlock.loadScript(static_cast<const char*>(buffer), static_cast<unsigned>(length));
	if (!ok)
		{
		foundation.printLine("loadAsset(): error loading ODS: ", filename);
		foundation.printLine("OdBlock::lastError:", ODBlock::lastError);
		}
	else 
		{
		//strip off the .asset.ods extension:
		char * pptr = const_cast<char*>(filename);	//this string is stored on the stack in a WIN32_FIND_DATAA struct, so this is fine, even if totally evil.
		while (*pptr && *pptr != '.')
			pptr++;
		if (*pptr == '.')
			*pptr = '\0';

		parseAsset(odBlock, filename);	
		}

	fileManager.unloadFile(buffer);

}


static void cmdLoadAssets(const char * scriptVar)
{
	//scriptVar is the handle to the voxmap resource.
	tmpVoxMap = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));
	//load all assets in resource directory, store voxvols in voxmap.
	//assets are a resource but the resource manager is bursting at the seams with all the other resources.
	//so we'll just load them directly for now.

	fileManager.iterateFiles("*.asset.ods", loadAsset);
	tmpVoxMap = 0;

}

static unsigned findAsset(const char* name)	//returns index in assets array
{
	for (unsigned i = 0; i < assets.size(); i++)
		{
		unsigned stringId = assets[i].name;
		const char * str = stringManager.lookupString(stringId);
		if (strcmp(str, name) == 0)
			return i;
		}
	return ~0u;
}

static void parseScene(ODBlock & odBlock)
{
/*
Scene   #aka map or world - instances assets into instances.
    {
    # can reference all assets in resource directory.
    physics
        {
        gravity { 0; -9.8; 0; }
        }
    instances
        {       #x y z rx ry rz s
        terrain { 0; -20; 0; 0; 0; 0; 1;}
        planet  { 0; 0; 0; 0; 0; 0; 1;}
        deer    { 0; 0; 0; 0; 0; 0; 1;}
        castle  { 0; 0; 0; 0; 0; 0; 1;}
        }
    }
	*/
	ODBlock * instancesBlock = odBlock.getBlock("instances");
	if (instancesBlock)
		{
		//TODO: parse instances
		instancesBlock->reset();
		while (instancesBlock->moreSubBlocks())
		{
			ODBlock* instBlock = instancesBlock->nextSubBlock();
			foundation.printLine("  inst:", instBlock->ident());
			instBlock->reset();
			float  x = instBlock->moreTerminals() ? instBlock->nextTerminalAsFloat() : 0.0f;
			float  y = instBlock->moreTerminals() ? instBlock->nextTerminalAsFloat() : 0.0f;
			float  z = instBlock->moreTerminals() ? instBlock->nextTerminalAsFloat() : 0.0f;
			float rx = instBlock->moreTerminals() ? instBlock->nextTerminalAsFloat() : 0.0f;
			float ry = instBlock->moreTerminals() ? instBlock->nextTerminalAsFloat() : 0.0f;
			float rz = instBlock->moreTerminals() ? instBlock->nextTerminalAsFloat() : 0.0f;
			float  s = instBlock->moreTerminals() ? instBlock->nextTerminalAsFloat() : 1.0f;
			foundation.printf(" p(%.2f %.2f %.2f) r(%.2f %.2f %.2f) s(%.2f)\n", x, y, z, rx, ry, rz, s);

			//need to get the index of the asset.  This is an index into VolumeAtlas::assetMap.  That gets allocated in ResourceManager::loadSubResource(), called from cmdLoadAssets().
			unsigned assetIndex = findAsset(instBlock->ident());
			if (assetIndex == ~0u)
			{
				foundation.printLine("parseScene(): asset not found:", instBlock->ident());
			}
			else
			{
				//add instance to instances buffer

				if (numSceneInstances < MAX_INSTANCES && assetIndex != ~0u)
				{
					SceneInstance& inst = sceneInstances[numSceneInstances];

					inst.assetIndex = assetIndex;
					inst.currentFrameIndex = 0;
					inst.position = PxVec3(x, y, z);
					inst.scaledOrientation = PxQuat(rx * 3.14159f / 180.0f, PxVec3(0.0f, 1.0f, 0.0f));
					inst.scaledOrientation *= PxQuat(ry * 3.14159f / 180.0f, PxVec3(1.0f, 0.0f, 0.0f));
					inst.scaledOrientation *= PxQuat(rz * 3.14159f / 180.0f, PxVec3(0.0f, 0.0f, 1.0f));
					inst.scaledOrientation *= s;
					numSceneInstances++;
				}
			}
		}
		}	


}

static void cmdLoadScene(const char * scriptVar)
{
    foundation.printLine("Loading scene:", scriptVar);
	size_t length;
	const void* buffer = fileManager.loadFile(scriptVar, &length, true);

	ODBlock odBlock;	//points into buffer
	bool ok = odBlock.loadScript(static_cast<const char*>(buffer), static_cast<unsigned>(length));
	if (!ok)
		{
		foundation.printLine("loadScene(): error loading ODS: ", scriptVar);
		foundation.printLine("OdBlock::lastError:", ODBlock::lastError);
		}
	else 
		{
		parseScene(odBlock);	
		}

	fileManager.unloadFile(buffer);
}

static void cmdVoxelGameTickPhysics()
{
	if (gFoundation == NULL)
		cmdVoxelGameInitPhysics();
	if (gScene)
	{
		gScene->simulate(1.0f / 60.0f);
		gScene->fetchResults(true);
	}

	//TODO: get xforms later:


	/*PxU32 nbActors = gScene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
	if (nbActors)
	{
		PxArray<PxRigidActor*> actors(nbActors);
		scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(&actors[0]), nbActors);
		Snippets::renderActors(&actors[0], static_cast<PxU32>(actors.size()), true);
	}
	*/
}

void cmdVoxelGameTick(const char* scriptVar)	//scriptVar is the handle to constant buffer resource.
{

	//write instances to shader buffer:
	//this has to be written otherwise the buffer will contain garbage.
	ASSERT(numSceneInstances < MAX_INSTANCES);	//need a space for sentinel -- TODO:  pass the array length explicitly and get rid of the sentinel.

	//TODO: frustum cull scene instances and copy to drawInstances:
	numDrawInstances = 0;
	for (unsigned i = 0; i < numSceneInstances; i++)
		{
		SceneInstance& inst = sceneInstances[i];
		DrawInstance& drawInst = drawInstances[i];
		drawInst.position = inst.position;

		//hack to animate all animated instances: 
		if (assets[inst.assetIndex].numFrames > 1)
		{
			float animSeconds = 4.0f * (gTime % 1000) * 0.001f;
			unsigned deerFrame = ((unsigned)animSeconds) % assets[inst.assetIndex].numFrames;
			inst.currentFrameIndex = deerFrame;
		}
		//hack for physics based planet: 
		if (i == 0)
		{
			PxTransform t = gDynamic->getGlobalPose();
			inst.position = t.p;
			inst.scaledOrientation = t.q;
			inst.scaledOrientation *= 0.5f;
		}
		//hack to spin planet: (yes we have no way of knowing the index at compile time)
		if (i == 1)
		{
			inst.scaledOrientation = PxQuat(2 * 3.14159f * (gTime % 10000) * 0.0001f, PxVec3(0.0f, 1.0f, 0.0f)) * 0.5f;
		}
		

		drawInst.volumeAtlasDrawIndex = assets[inst.assetIndex].volumeAtlasFirstFrameIndex + inst.currentFrameIndex;
		drawInst.scaledOrientation = inst.scaledOrientation;
		numDrawInstances++;
		}

	drawInstances[numDrawInstances].volumeAtlasDrawIndex = -1;	//sentinel for end of buffer.

	ResourceId rid = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));
	resourceManager.writeResource(rid, drawInstances, sizeof(DrawInstance) * numDrawInstances);
}


void cmdVoxelGameShutdown()
{
	PX_RELEASE(gScene);
	PX_RELEASE(gDispatcher);
	PX_RELEASE(gPhysics);
	PX_RELEASE(gFoundation);

	foundation.printLine("VoxelGame cleaned up.");
}

void derivedAppRegisterCommands()
{
//commandManager.addCommand("voxelGameAssetMap", cmdVoxelGameAssetMap);
commandManager.addCommand("voxelGameTick", cmdVoxelGameTick);
commandManager.addCommand("voxelGameShutdown", cmdVoxelGameShutdown);
commandManager.addCommand("voxelGameInitPhysics", cmdVoxelGameInitPhysics);
commandManager.addCommand("voxelGameTickPhysics", cmdVoxelGameTickPhysics);

//maybe move these to a separate file / make generic?
commandManager.addCommand("loadAssets", cmdLoadAssets);
commandManager.addCommand("loadScene", cmdLoadScene);	

}
