#pragma once

//resource types
struct ResourceType
{
	enum Enum
	{
	eVOID,			//nonexistent type
	eODSCRIPT_SHADER,
	eODSCRIPT_MESH,
	eODSCRIPT_COMMANDS,
	eGENERIC_HLSL,	//any sort of HLSL file, need to read it to see what it is.
	eIMAGE,			//image or texture that can have interpolated sampling 
	eVOX_VOL,		//some sort of voxel volume, basically volume texture
	eARRAY,			//array of linear data, structured buffer, e.g. lightmap volume
	eCS_CONST,		//GPU constant buffer for compute shaders
	ePS_CONST,		//GPU constant buffer for pixel shaders
	eVS_CONST,		//GPU constant buffer for vertex shaders

	};
};

//we pass this by value so don't make it too big.
typedef unsigned ResourceId;	//index into resources[]

struct Resource;
struct ShaderResource;
struct MeshResource;
struct ScriptResource;
class ODBlock;
class String;

class ResourceManager
{
public:
	ResourceManager(void);
	~ResourceManager();

	void registerCommands();

	void shutDown();	//free all resources at exit

	void setResourceDirectory(const char * dir);		//should include a trailing slash for easy appending of file names.
	ResourceId loadFromResourceDir(const char * path);	//works for all resource types.  Works relative to resource directory.	This talks directly to the renderer because
														//the idea is that the resource manager transparently loads and unloads stuff like shaders on demand.

	void loadSubResource(ResourceId, const char* path);
	ResourceId genAssetMap(ResourceId texWithAtlas);	//generates a GPU loadable const resource that acts as a lookup table from asset indices to subresource bounds.
	ResourceType::Enum getResourceType(ResourceId);
	void reloadAll();
	void reload(ResourceId);

	MeshResource * getMesh(ResourceId);
	void writeResource(ResourceId, const void * data, size_t nBytes);

	//resource usage:	these could be replaced with a unified 'use resource' call.
	void bindResource(ResourceId, unsigned slot = 0, bool unbind = false);		//requests this resource to be used in a certain shader slot for the next draw prim or compute invocation.
	void clearResource(ResourceId);
	void drawMesh(ResourceId);
	void executeCommands(ResourceId);

private:
	ResourceType::Enum resourceTypeFromFileName(const String & fname);

	void loadShader(Resource &, ODBlock &);
	void loadMesh(Resource &, ODBlock &);
	void loadCommands(Resource &, ODBlock &);




};


extern ResourceManager & resourceManager;


void cmdRunCommands(const char * path);							//this is in this file because a command resource is first loaded from that path.
void cmdSetResourceDirectory(const char * dir);

void cmdLoadResource(const char * scriptVar, const char * file);
void cmdBindResource(const char * scriptVar);	//bind resource to shader slot 0 -- legacy
void cmdClearResource(const char * scriptVar);
void cmdBindResourceSlot(const char * scriptVar, int slot);	//bind resource to a shader slot
void cmdDrawResource(const char * scriptVar);
void cmdReloadResources();
void cmdloadSubResource(const char* scriptVar, const char* file);
void cmdGenAssetMap(const char* scriptVarDest, const char* scriptVarTiledTexture);
