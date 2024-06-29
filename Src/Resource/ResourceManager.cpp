#include "StdAfx.h"
#include "Foundation\Foundation.h"
#include "Foundation\MemoryManager.h"
#include "Foundation\StringManager.h"
#include "Foundation\VariableManager.h"
#include "ResourceManager.h"
#include "Foundation\String.h"
#include "Foundation\Allocator.h"
#include "Foundation\Array.h"
#include "Foundation\FileManager.h"


#pragma warning(push)
#pragma warning( disable : 4548) //expression before comma has no effect
#pragma warning( disable : 4668)//x  not defined as a preprocessor macro
#pragma warning( disable : 4365)//signed/unsigned mismatch
#pragma warning( disable : 4987)//nonstandard extension used
#include <SDL.h>
#pragma warning(pop)

#include <SDL_image.h>
#pragma comment (lib, "SDL2_image.lib") 


#include "Foundation\ODBlock.h"
#include "Foundation\CommandManager.h"
#include "Renderer\Renderer.h"	//not sure about this dependency, but we need it to compile our shaders.  I would rather have the renderer depend on the resource manager actually.


#include "VoxVol.h"
#include "VolumeAtlas.h"

const unsigned maxStreamsPerMesh = 8;			//arbitrary

#pragma warning( disable : 4061) //enumerator 'xx' in switch of enum 'yy' is not explicitly handled by a case label


//system to map a file extension to an enum type code
struct ExtensionsMap
	{
	const char * extension;
	unsigned	 strlen;
	ResourceType::Enum type;
	};


static const ExtensionsMap extMap[] =
	{
#define STRING_AND_LENGTH(str) str, sizeof(str)-1
		{ STRING_AND_LENGTH("Shader.ods"), ResourceType::eODSCRIPT_SHADER },
		{ STRING_AND_LENGTH("Mesh.ods"), ResourceType::eODSCRIPT_MESH },
		{ STRING_AND_LENGTH("Commands.ods"), ResourceType::eODSCRIPT_COMMANDS },
		{ STRING_AND_LENGTH("hlsl"), ResourceType::eGENERIC_HLSL },
		{ STRING_AND_LENGTH("bmp"), ResourceType::eIMAGE },
		{ STRING_AND_LENGTH("png"), ResourceType::eIMAGE },//TODO: can add all other image formats supported by our image library  
															//	This library supports BMP, PNM (PPM/PGM/PBM), XPM, LBM, PCX, GIF, JPEG, PNG,
															//	TGA, TIFF, and simple SVG formats.
		{ STRING_AND_LENGTH("voxvol"), ResourceType::eVOX_VOL},	// voxel volume
		{ STRING_AND_LENGTH("array"), ResourceType::eARRAY},// array, e.g. light volume
		{ STRING_AND_LENGTH("cs_const"), ResourceType::eCS_CONST},// compute shader constant buffer
		{ STRING_AND_LENGTH("ps_const"), ResourceType::ePS_CONST},// pixel shader constant buffer
		{ STRING_AND_LENGTH("vs_const"), ResourceType::eVS_CONST},// vertex shader constant buffer
#undef STRING_AND_LENGTH
	};

//resources typically contain subsystem specific handles.
//they should not have destructors that free resources because they are stored by value in a growable array that would keep 
//destroying and recreating them.

struct Resource
	{
	Resource() : pathId(~0u) {}		//invalid resource
	Resource(unsigned sid) : pathId(sid), lastAccess(0), type(ResourceType::eVOID), resourceSpecificIndex(~0u) {}
	Resource(unsigned p, unsigned la, ResourceType::Enum t, unsigned rsi) : pathId(p), lastAccess(la), type(t), resourceSpecificIndex(rsi) {}

	unsigned			pathId;	//-1 == ~0u if invalid, else index for StringManager, should have file path we can reload from.
	time_t			lastAccess;	//actually size for now until we implement portable way to get file date
	ResourceType::Enum	type;
	unsigned			resourceSpecificIndex;	//indexes into e.g. shaderResources; -1 if unassigned.
	};

struct ShaderResource //: public Resource
	{
	ShaderResource(ShaderPtr p) : ptr(p) {}
	ShaderResource() {}	//ctor used in array resize, before assignment operator.

	ShaderPtr ptr;
	//etc
	};

struct TextureResource
	{
	TextureResource(TexturePtr p, VolumeAtlas * v = NULL) : ptr(p), va(v) {}
	TextureResource(): va(NULL) {}
	

	void freeVA()	//intentionally not a destructor because we need more control.
		{
		if (va) 
			delete va;
		va = NULL; 
		}

	TexturePtr ptr;
	VolumeAtlas* va;	//optional volume atlas
	};

struct MeshResource //: public Resource
	{
	MeshResource() : numVertexArrays(0) {}

	VertexAttributes attributes;
	GeometryArray vertexArrays[maxStreamsPerMesh];	//TODO: tune inline allocator size for whatever will become our typical vertex size
	unsigned numVertexArrays;
	GeometryArray indexArray;	//this can be NULL/0/0 for a non-indexed resource.
	};

struct ScriptResource //: public Resource
	{
	ScriptResource(ScriptPtr p) : ptr(p) {}
	ScriptResource() : ptr(0) {}	//ctor used during array resize.

	ScriptPtr ptr;
	};


Array<Resource, Allocator>			resources;
Array<ShaderResource, Allocator>	shaderResources;	//TODO: at index 0 we should have default / error / unloaded texture.
Array<TextureResource, Allocator>	textureResources;	//TODO: at index 0 we should have default / error / unloaded texture.
Array<MeshResource, Allocator>		meshResources;
Array<ScriptResource, Allocator>	scriptResources;

//functions:
void cmdRunCommands(const char * path)
	{
	ResourceId commands = resourceManager.loadFromResourceDir(path);
	//if (commands.type == ResourceType::eODSCRIPT_COMMANDS)	//this will be false if loading fails.
	resourceManager.executeCommands(commands);
	}

void cmdSetResourceDirectory(const char * path)
	{
	resourceManager.setResourceDirectory(path);
	}

void cmdLoadResource(const char * scriptVar, const char * file)
	{
	ResourceId r = resourceManager.loadFromResourceDir(file);
	variableManager.setVariable(scriptVar, static_cast<int>(r));
	}

void cmdloadSubResource(const char* scriptVar, const char* file)
{
	ResourceId r = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));
	resourceManager.loadSubResource(r, file);
}

void cmdBindResource(const char * scriptVar)
	{
	ResourceId r = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));
	resourceManager.bindResource(r);
	}

void cmdClearResource(const char * scriptVar)
	{
	ResourceId r = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));
	resourceManager.clearResource(r);
	}

void cmdBindResourceSlot(const char * scriptVar, int slot)
	{
	ResourceId r = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));
	resourceManager.bindResource(r, static_cast<unsigned>(slot));
	}

void cmdUnbindResourceSlot(const char* scriptVar, int slot)
{
	ResourceId r = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));
	resourceManager.bindResource(r, static_cast<unsigned>(slot), true);
}

void cmdDrawResource(const char * scriptVar)
	{
	ResourceId r = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));

	MeshResource * mr = resourceManager.getMesh(r);
	if (mr)
		renderer.draw(mr->vertexArrays, mr->numVertexArrays, mr->indexArray);
	}

void cmdReloadResources()
	{
	resourceManager.reloadAll();
	}

void cmdGenAssetMap(const char* scriptVarDest, const char* scriptVarTiledTexture)
	{
	ResourceId rin = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVarTiledTexture));
	
	ResourceId r = resourceManager.genAssetMap(rin);
	variableManager.setVariable(scriptVarDest, static_cast<int>(r));
	}



ResourceManager::ResourceManager(void)
	{
	}

ResourceManager::~ResourceManager()
	{
	}

void ResourceManager::registerCommands()
	{
	commandManager.addCommand("runCommands", cmdRunCommands);
	commandManager.addCommand("setResourceDirectory", cmdSetResourceDirectory);
	commandManager.addCommand("loadResource", cmdLoadResource);
	commandManager.addCommand("drawResource", cmdDrawResource);
	commandManager.addCommand("bindResource", cmdBindResource);
	commandManager.addCommand("clearResource", cmdClearResource);
	commandManager.addCommand("bindResourceSlot", cmdBindResourceSlot);
	commandManager.addCommand("unbindResourceSlot", cmdUnbindResourceSlot);
	commandManager.addCommand("reloadResources", cmdReloadResources);
	commandManager.addCommand("loadSubResource", cmdloadSubResource);
	commandManager.addCommand("genAssetMap", cmdGenAssetMap);

	IMG_Init(IMG_INIT_PNG);
	}


void ResourceManager::shutDown()	//free all resources at exit
	{
	for (unsigned i = 0; i < shaderResources.size(); i++)
		renderer.releaseShader(shaderResources[i].ptr);

	for (unsigned i = 0; i < textureResources.size(); i++)
	{
		renderer.releaseTexture(textureResources[i].ptr);
		textureResources[i].freeVA();

	}


	for (unsigned i = 0; i < meshResources.size(); i++)
		{
		for (unsigned v = 0; v < meshResources[i].numVertexArrays; v++)
			renderer.releaseGeometryArray(meshResources[i].vertexArrays[v]);
		renderer.releaseGeometryArray(meshResources[i].indexArray);
		}

	for (unsigned i = 0; i < scriptResources.size(); i++)
		commandManager.releaseScript(scriptResources[i].ptr);


	resources.reset();
	shaderResources.reset();		//have to release memory here explicitly because our memory system shuts down before global destructors run :(
	textureResources.reset();
	meshResources.reset();
	scriptResources.reset();
	}

void ResourceManager::reloadAll()
	{
	for (unsigned i = 0; i < resources.size(); i++)
		reload(i);
	}

void ResourceManager::setResourceDirectory(const char * dir)
	{
	fileManager.setResourceDir(dir);
	}


void ResourceManager::reload(ResourceId id)
	{

	//single entry point load or reload for all resources
	ASSERT(id < resources.size());
	Resource & r = resources[id];

	const char * fullPath = stringManager.lookupString(r.pathId);

	if (r.type == ResourceType::eVOID)
		{
		foundation.printLine("ResourceManager::reload():void resource type referenced:", fullPath);
		return;
		}

	//try to early out if already loaded and file hasn't changed
	//TODO: resources that are not file based always return true which makes them discard and recreate all the time, doesn't seem very smart!
	if (!fileManager.wasModifiedSince(fullPath, r.lastAccess, &r.lastAccess, false))
		return;

	switch (r.type)
		{
		//all ODS based resources:
		case	ResourceType::eODSCRIPT_SHADER:
		case	ResourceType::eODSCRIPT_MESH:
		case	ResourceType::eODSCRIPT_COMMANDS:
			{
			size_t length;
			const void* buffer = fileManager.loadFile(fullPath, &length, false);

			ODBlock odBlock;	//points into buffer
			bool ok = odBlock.loadScript(static_cast<const char*>(buffer), length);
			if (!ok)
				{
				foundation.printLine("ResourceManager::reload(): error loading ODS: ", fullPath);
				foundation.printLine("OdBlock::lastError:", ODBlock::lastError);
				}
			else switch (r.type)
				{
				case	ResourceType::eODSCRIPT_SHADER:
					loadShader(r, odBlock);
					break;
				case	ResourceType::eODSCRIPT_MESH:
					loadMesh(r, odBlock);
					break;
				case	ResourceType::eODSCRIPT_COMMANDS:
					loadCommands(r, odBlock);
					break;
				default: //fall through
					ASSERT(0);	//unknown resource? handle this
				}

			fileManager.unloadFile(buffer);
			}
			break;
		case	ResourceType::eGENERIC_HLSL:
			{
			size_t length;
			//infer shader type from file name.
			ShaderType::Enum shaderType = strstr(fullPath, "Pixel") ? ShaderType::ePIXEL : (strstr(fullPath, "Vertex") ? ShaderType::eVERTEX : ShaderType::eCOMPUTE);			

			ShaderPtr sptr;

/*
			mod_txt = modified date of source hlsl
			mod_cache = exists compiled file ? modified date of compile file : ancient times;

			if (mod_txt > mod_cache) 
				data = loadFile(src)
				create shader(data, flag SOURCE = implicitly save to cache cache);
			else
				data = cache;
				create shader(data, flag COMPILED).
*/

			String compiledPath = fullPath;
			compiledPath = compiledPath + ".obj";


			time_t sourceModTime = 0, objModTime = 0;
			fileManager.wasModifiedSince(compiledPath, 0, &objModTime, false);	//get time stamp of compiled source if it exists, otherwise leave at 0.
			if (fileManager.wasModifiedSince(fullPath, objModTime, &sourceModTime, false))
				{
				//source code more recent!
				foundation.printLine("ResourceManager::reload():compiling shader:", fullPath);
				const void* buffer = fileManager.loadFile(fullPath, &length, false);
				sptr = renderer.createShader(buffer, length, fullPath, shaderType, true, compiledPath);
				fileManager.unloadFile(buffer);
				}
			else
				{
				//obj code more recent!
				const void* buffer = fileManager.loadFile(compiledPath, &length, false);

				sptr = renderer.createShader(buffer, length, fullPath, shaderType, false, NULL);

				fileManager.unloadFile(buffer);
				}




		

			if (r.resourceSpecificIndex != ~0u)
				{
				//this was already loaded previously, release the old instance, and replace with new
				ShaderResource & oldr = shaderResources[r.resourceSpecificIndex];
				renderer.releaseShader(oldr.ptr);
				oldr.ptr = sptr;
				}
			else
				{
				//create new instance.
				r.resourceSpecificIndex = shaderResources.size();
				shaderResources.pushBack(ShaderResource(sptr));
				}
			}
		break;
		case ResourceType::eIMAGE:
			{
			SDL_Surface* sdlSurface = NULL;
			TexturePtr tptr;

			if (strstr(fullPath, "f.swapchain"))
				{
				tptr = renderer.createSwapChain();
				}
			else if (strstr(fullPath, "f.rendertarget"))
				{
				tptr = renderer.createRenderTarget();
				}
			else if (strstr(fullPath, "f.tex2d"))
				{
				//empty buffer
				const char* p = fullPath + 7;//at least this offset until end of f.tex2d.
				while (*p != 0 && *p != '(')
					p++;
				unsigned w = 0, h = 0;
				sscanf(p, "(%d %d)", &w, &h);
				tptr = renderer.createTexture(w, h, NULL, 0, PixelFormat::e_Rf32_Gf32_Bf32_Af32);//let's default to float textures here.
				}
			else
				{
				//from file
				sdlSurface = IMG_Load(fullPath);
				if (!sdlSurface)
					{
					foundation.printLine("IMG_Load: ", IMG_GetError());
					// handle error
					}
				else
					{
					//load image into a texture: (note: this is crated as a read-only immutable surface for now) 
					SDL_LockSurface(sdlSurface);
					if (sdlSurface->format->format != SDL_PIXELFORMAT_ABGR8888)
						foundation.printLine("ResourceManager::reload():unsupported pixel format in image:", fullPath);
					else
						tptr = renderer.createTexture(sdlSurface->w, sdlSurface->h, sdlSurface->pixels, sdlSurface->pitch, PixelFormat::e_Ru8_Gu8_Bu8_Au8);
					SDL_UnlockSurface(sdlSurface);

					//can I free the image here?  will it be copied above?
					SDL_FreeSurface(sdlSurface);
					}
				}


				if (r.resourceSpecificIndex != ~0u)
					{
					//this was already loaded previously, release the old instance, and replace with new
					TextureResource & oldr = textureResources[r.resourceSpecificIndex];
					renderer.releaseTexture(oldr.ptr);
					oldr.freeVA();
					oldr.ptr = tptr;
					}
				else
					{
					//create new instance.
					r.resourceSpecificIndex = textureResources.size();
					textureResources.pushBack(TextureResource(tptr));
					}


			}
		break;
		case ResourceType::eVOX_VOL:
			{
			TexturePtr tptr;
			VolumeAtlas* va = NULL;
			
			if (strstr(fullPath, "f.tiled"))
				{
				//allocate an empty tiled volume texture
				unsigned w = 0, h = 0, d = 0;
				const char* p = fullPath + 7;//at least this offset until end of f.tiled.
				while (*p != 0 && *p != '(')
					p++;				
				sscanf(p, "(%d %d %d)", &w, &h, &d);
				tptr = renderer.createTiledVolumeTexture(w, h, d);
				va = new VolumeAtlas(w, h, d);
				}
			else
				{
				//load file
				VoxVol volume;
				volume.load(fullPath);

				//create texture from it
				unsigned char* data = volume.lockForRead();	//we changed this recently to fail for multichunk volumes, because it is inefficient; if that ends up being a bad decision, go and revert it.
				tptr = renderer.createVolumeTexture(volume.getWidth(), volume.getHeight(), volume.getDepth(), data);
				volume.unlock();

				if (data == NULL)
					{
					//this is a multipart volume that cannot just be locked ... it will write itself.
					unsigned rowPitch, depthPitch;
					void* p = renderer.lockTexture(tptr, rowPitch, depthPitch);
					volume.writeToBuffer(p, rowPitch, depthPitch);
					renderer.unlockTexture(tptr);
					}
				}


			//same texture management stuff as for 2D
			if (r.resourceSpecificIndex != ~0u)
				{
				//this was already loaded previously, release the old instance, and replace with new
				TextureResource & oldr = textureResources[r.resourceSpecificIndex];
				renderer.releaseTexture(oldr.ptr);
				oldr.freeVA();
				oldr.va = va;
				oldr.ptr = tptr;
				}
			else
				{
				//create new instance.
				r.resourceSpecificIndex = textureResources.size();
				textureResources.pushBack(TextureResource(tptr)).va = va;	//only assign va after add and not in ctor, otherwise the destructor of the temp object will destroy it as it takes ownership.
				}


			}
		break;
		case ResourceType::eARRAY:
			{
			bool clear = false;
			unsigned sizeOffset = 2;
			if (strstr(fullPath, "f.zero"))
			{
				//initialize to zero
				clear = true;
				sizeOffset = 6;
			}
			else if (strstr(fullPath, "f.uninit"))
			{
				//don't initialize
				sizeOffset = 8;
			}
			else
			{
				foundation.printLine("ResourceManager::reload():invalid array source.");
				break;
			}

			const char* p = fullPath + sizeOffset;
			while (*p != 0 && *p != '(')
				p++;				
			unsigned structSize = 0, numElems = 0;
			sscanf(p, "(%d %d)", &structSize, &numElems);

			unsigned char* buffer = NULL;
			if (clear)
				{
				size_t size = structSize * numElems;
				buffer = new unsigned char[size];
				memset(buffer, 0, size);
				}

			TexturePtr tptr = renderer.createStructuredBuffer(structSize, numElems, buffer);

			if (buffer)
				delete[] buffer;

			//same texture management stuff as for 2D
			if (r.resourceSpecificIndex != ~0u)
				{
				//TODO: what is the point of this, we just memset it to zero again!?
				//this was already loaded previously, release the old instance, and replace with new
				TextureResource & oldr = textureResources[r.resourceSpecificIndex];
				renderer.releaseTexture(oldr.ptr);
				oldr.freeVA();
				oldr.ptr = tptr;
				}
			else
				{
				//create new instance.
				r.resourceSpecificIndex = textureResources.size();
				textureResources.pushBack(TextureResource(tptr));
				}


			}
		break;
		case ResourceType::eCS_CONST:
		case ResourceType::ePS_CONST:
		case ResourceType::eVS_CONST:
		{

			unsigned size = 16;			//default
			const char * fdot = strstr(fullPath, "f.");
			if (fdot)
				{
				sscanf(fdot + 2, "%d", &size);
				}
			if (size < 1)
			{
				foundation.printLine("ResourceManager::reload():invalid resource size.");
				break;
			}				
			TextureType::Enum t;
			switch (r.type)
				{
				case ResourceType::eCS_CONST:
					t = TextureType::eCSConstantBuffer;
					break;
				case ResourceType::ePS_CONST:
					t = TextureType::ePSConstantBuffer;
					break;
				case ResourceType::eVS_CONST:
					t = TextureType::eVSConstantBuffer;
					break;
				}
			TexturePtr tptr = renderer.createMappableBuffer(sizeof(float), size, t);
			if (r.resourceSpecificIndex != ~0u)
				{
				//TODO: what is the point of this, we just memset it to zero again!?
				//this was already loaded previously, release the old instance, and replace with new
				TextureResource & oldr = textureResources[r.resourceSpecificIndex];
				renderer.releaseTexture(oldr.ptr);
				oldr.freeVA();
				oldr.ptr = tptr;
				}
			else
				{
				//create new instance.
				r.resourceSpecificIndex = textureResources.size();
				textureResources.pushBack(TextureResource(tptr));
				}
		}
		break;
		default:
			foundation.printLine("ResourceManager::reload():invalid resource type.");
		}	
	}

MeshResource * ResourceManager::getMesh(ResourceId id)
	{
	if (id < resources.size())
		{
		Resource & r = resources[id];
		if (r.type == ResourceType::eODSCRIPT_MESH)
			if (r.resourceSpecificIndex < meshResources.size())
				return &(meshResources[r.resourceSpecificIndex]);
		}

	foundation.printLine("ResourceManager::getMesh():invalid mesh resource id.");
	return 0;
	}

void ResourceManager::writeResource(ResourceId id, const void* data, size_t nBytes)
{
	if (id < resources.size())
	{
		Resource& r = resources[id];
		if ((r.type == ResourceType::eCS_CONST)
			||(r.type == ResourceType::ePS_CONST)
			||(r.type == ResourceType::eVS_CONST)
			)
			if (r.resourceSpecificIndex < textureResources.size())
			{
				TextureResource& res = textureResources[r.resourceSpecificIndex];
				renderer.setBufferData(res.ptr, 1, nBytes, data);
			}

	}

}

ResourceId ResourceManager::loadFromResourceDir(const char * path)
	{
	String fullPath = fileManager.getResourceDir() + path;
	//save this path in the StringManager - the sad thing is that the path and the resourcedir are already in the string manager separately but they get passed in here without the manager index 
	//which we would really need.  Plus its better to just combine them under a single index.  TODO
	
	ResourceId id = resources.size();
	resources.pushBack(Resource(stringManager.addString(fullPath), 0, resourceTypeFromFileName(fullPath), ~0u));

	reload(id);	//TODO: we could put this call on a worker thread(s) and have async loading!
	return id;
	}

void ResourceManager::loadSubResource(ResourceId id, const char* path)
	{
	if (id >= resources.size())
		{
		foundation.printLine("ResourceManager::loadSubResource():invalid resource id.");
		return;
		}
	Resource & r = resources[id];

	//only some resource types supported as subresources
	//NOTE: the way we handle subresources makes them not able to be hot reloaded if they change on file.
	//I don't think that's a problem.  If it is, we could have subresouces be resources themselves with a parent resource that points to the texture resource.
	switch (r.type)
		{
		case ResourceType::eVOX_VOL:
			{
			ASSERT(r.resourceSpecificIndex < textureResources.size());
			TextureResource& res = textureResources[r.resourceSpecificIndex];
			TexturePtr tptr = res.ptr;
			VoxVol volume;
			String fullPath = fileManager.getResourceDir() + path;

			volume.load(fullPath);

			unsigned numChunks = volume.getNumChunks();

			for (unsigned i = 0; i < numChunks; i++)
				{
				unsigned char* data = volume.lockChunkForRead(i);
				unsigned x = 0, y = 0, z = 0;
				if (!res.va)
					foundation.printLine("ResourceManager::loadSubResource():cannot use on atlas-less resource.");
				else if (res.va->findSpace(volume.getChunkWidth(i), volume.getChunkHeight(i), volume.getChunkDepth(i), x, y, z))
					renderer.writeToTiledVolumeTexture(tptr, x, y, z, volume.getChunkWidth(i), volume.getChunkHeight(i), volume.getChunkDepth(i), data);
				else
					foundation.printLine("ResourceManager::loadSubResource():cannot find space for subResource.");
				volume.unlockChunk();
				}

			}
		break;
		default:
			foundation.printLine("ResourceManager::loadSubResource():invalid resource type.");
		}
	}

ResourceId ResourceManager::genAssetMap(ResourceId id)
	{
	ResourceId rid = resources.size();

	if (id >= resources.size())
		{
		foundation.printLine("ResourceManager::genAssetMap():invalid resource id.");
		return rid;
		}
	Resource & r = resources[id];
	switch (r.type)
		{
		case ResourceType::eVOX_VOL:
			{
			ASSERT(r.resourceSpecificIndex < textureResources.size());
			if (!textureResources[r.resourceSpecificIndex].va)
				foundation.printLine("ResourceManager::genAssetMap():cannot use on atlas-less resource.");
			else
				{
				unsigned nFloats = textureResources[r.resourceSpecificIndex].va->getAssetMapSizeInFloats();
				if (nFloats)
					{
					char descString[32];
					sprintf(descString, "f.%d.cs_const", nFloats);	//kinda awkward :/ but I can't be bothered to make a non-string parsing based version of the below.
					rid = loadFromResourceDir(descString);	//caution! This could become async later! 
					size_t size;
					void* bytes;
					bytes = textureResources[r.resourceSpecificIndex].va->genAssetMap(size);
					writeResource(rid, bytes, size);
					}
				}
			}
		break;
		default:
			foundation.printLine("ResourceManager::genAssetMap():invalid resource type.");
		}
	return rid;
	}

void ResourceManager::loadCommands(Resource &r, ODBlock & odBlock)
	{
	ScriptPtr csq = commandManager.createScript(odBlock);

	r.resourceSpecificIndex = scriptResources.size();
	scriptResources.pushBack(ScriptResource(csq));
	}

void ResourceManager::loadShader(Resource &r, ODBlock & odBlock)
	{
	/*
	Shader
	{
	name { simpleVertexShader; }
	profile { vs_5_0;}
	source { "simpleVertexShader.hlsl"; }
	vertexAttributes { POSITION; COLOR; }		//only vertex shaders
	}
	*/
	const char * sourceFile = NULL;
	const char * profile = NULL;
	odBlock.getBlockString("source", &sourceFile);
	odBlock.getBlockString("profile", &profile);

	VertexAttributes attribs;

	ShaderType::Enum type;

	if (strcmp(profile, "vs_5_0") == 0)
		{
		type = ShaderType::eVERTEX;
		ODBlock * vertexAttribs = odBlock.getBlock("vertexAttributes");
		if (vertexAttribs)
			attribs.read(*vertexAttribs);
		}
	else if (strcmp(profile, "ps_5_0") == 0)
		type = ShaderType::ePIXEL;
	else
		{
		//TODO: add compute shader support here?
		foundation.printLine("ResourceManager::loadShader(): unknown shadertype in shader ODS: ", profile);
		return;
		}

	if (sourceFile && profile)
		{
		size_t length;
		String fullPath = fileManager.getResourceDir() + sourceFile;
		const void* buffer = fileManager.loadFile(fullPath, &length, false);
		ShaderPtr sptr = renderer.createShader(buffer, length, fullPath, type, true, NULL, &attribs);
		fileManager.unloadFile(buffer);



		if (r.resourceSpecificIndex != ~0u)
			{
			//this was already loaded previously, release the old instance, and replace with new
			ShaderResource & oldr = shaderResources[r.resourceSpecificIndex];
			renderer.releaseShader(oldr.ptr);
			oldr.ptr = sptr;
			//oldr.type = type;
			}
		else
			{
			//create new instance.
			r.resourceSpecificIndex = shaderResources.size();
			shaderResources.pushBack(ShaderResource(sptr));
			}
		}
	else
		foundation.printLine("ResourceManager::loadShader(): source or profile in shader ODS not found!");
	}


void ResourceManager::loadMesh(Resource &r, ODBlock & odBlock)
	{
	/*
	Mesh
	{
	name { triangle; }

	vertexAttributes { POSITION; COLOR; }	//TODO: implement this channel based approach to vertex mapping and assert that it matches the shader

	vertices								//vertices are interpreted according to vertexAttributes field.
	{
	0;		0.5;		0;				1; 0; 0; 1;
	0.45;	-0.5;		0;				0; 1; 0; 1;
	-0.45;	-0.5;		0;				0; 0; 1; 1;
	}



	indices									//indices are optional.  If there are no indices, vertices are drawn as unindexed triangles.
	{
	0;		1;		2;
	}
	}
	*/
	//VertexAttributes attribs;
	r.resourceSpecificIndex = meshResources.size();
	meshResources.pushBack(MeshResource());
	MeshResource & mr = meshResources.back();


	ODBlock * vertexAttribs = odBlock.getBlock("vertexAttributes");
	if (!vertexAttribs)
		{
		foundation.printLine("ResourceManager::loadMesh(): cannot find vertex attribs block in mesh.ods!");
		return;
		}
	mr.attributes.read(*vertexAttribs);
	MxU32 vertexSize = mr.attributes.sizeofVertex();

	ODBlock * vertexBlock = odBlock.getBlock("vertices");
	if (!vertexBlock)
		{
		foundation.printLine("ResourceManager::loadMesh(): cannot find vertex block in mesh.ods!");
		return;
		}
	MxU32 floatCount = vertexBlock->numSubBlocks();
	if ((floatCount*sizeof(float)) % vertexSize != 0)
		{
		foundation.printLine("ResourceManager::loadMesh(): floatCount*4 % vertexSize != 0 in mesh.ods!");
		return;
		}
	MxU32 vertexCount = (floatCount*sizeof(float)) / vertexSize;

	//load indices.
	ODBlock * indexBlock = odBlock.getBlock("indices");
	GeometryArray ia(NULL, 0, 0);
	if (indexBlock)
		{
		MxU32 indexCount = indexBlock->numSubBlocks();
		if (indexCount % 3 != 0)
			{
			foundation.printLine("ResourceManager::loadMesh(): indexCount % 3 != 0 in mesh.ods!");
			}
		else
			{
			Array<MxU32, Allocator> indices;
			indices.reserve(indexCount);
			indexBlock->reset();
			for (unsigned int n = 0; n < indexCount; n++)
				{
				MxU32 i;
				sscanf_s(indexBlock->nextTerminal(), "%d", &i);
				indices.pushBack(i);
				}

			mr.indexArray = renderer.createIndexArray(indices.begin(), sizeof(MxU32), indexCount);
			}
		}

	/*
	This loads all vertex attribs packed into a single buffer like it is in the source file.
	We decided we don't want to do that for more flexible shading.
	Array<float, Allocator> floats;

	floats.reserve(floatCount);
	vertexBlock->reset();
	for (unsigned n=0; n<floatCount; n++)
	{
	float f;
	sscanf_s(vertexBlock->nextTerminal(),"%f",&f);
	floats.pushBack(f);
	}
	GeometryArray va = renderer.createVertexArray(floats.begin(), vertexSize, vertexCount);
	*/

	//load into separate buffers instead:
	Array<float, Allocator> floatBuffers[maxStreamsPerMesh];
	ASSERT(mr.attributes.attribs.size() <= maxStreamsPerMesh);
	MxU32 totalSize = 0;
	for (unsigned a = 0; a < mr.attributes.attribs.size(); a++)
		{
		MxU32 bufferSize = vertexCount * mr.attributes.sizeofAttrib(a);
		floatBuffers[a].reserve(bufferSize);
		totalSize += bufferSize;
		}
	ASSERT(totalSize == floatCount * sizeof(float));
	vertexBlock->reset();
	for (unsigned v = 0; v < vertexCount; v++)
		{
		for (unsigned a = 0; a < mr.attributes.attribs.size(); a++)
			{
			ASSERT(mr.attributes.sizeofAttrib(a) % sizeof(float) == 0);	//TODO: the below code only supports float based attribs for now.
			for (unsigned f = 0; f < mr.attributes.sizeofAttrib(a) / sizeof(float); f++)
				{
				float x;
				sscanf_s(vertexBlock->nextTerminal(), "%f", &x);
				floatBuffers[a].pushBack(x);
				}
			}
		}
	mr.numVertexArrays = mr.attributes.attribs.size();
	for (unsigned a = 0; a < mr.numVertexArrays; a++)
		{
		mr.vertexArrays[a] = renderer.createVertexArray(floatBuffers[a].begin(), mr.attributes.sizeofAttrib(a), vertexCount);
		}
	}
//======================================================================
//resource user functions:
//I don't like the way the resource manager has to know how to call on the subsystems to use the resources.
//maybe we could have the caller do this?  Not sure.

void ResourceManager::executeCommands(ResourceId id)
	{
	ASSERT(id < resources.size());
	Resource & r = resources[id];
	if (r.type == ResourceType::eODSCRIPT_COMMANDS)
		{
		if (r.resourceSpecificIndex < scriptResources.size())
			{
			ScriptResource & res = scriptResources[r.resourceSpecificIndex];
			commandManager.runScript(res.ptr);
			}
		}
	else
          foundation.printLine("ResourceManager::executeCommands(): eODSCRIPT_COMMANDS resource expected!");
	}

void ResourceManager::clearResource(ResourceId id)
	{
	ASSERT(id < resources.size());
	Resource & r = resources[id];
	switch (r.type)
		{
		case ResourceType::eIMAGE:
		case ResourceType::eVOX_VOL:
		case ResourceType::eARRAY:
		case ResourceType::ePS_CONST:
		case ResourceType::eVS_CONST:
		case ResourceType::eCS_CONST:
		if (r.resourceSpecificIndex < textureResources.size())
			{
			TextureResource & res = textureResources[r.resourceSpecificIndex];
			renderer.clearTexture(res.ptr);
			}

		break;
		default:
        foundation.printLine("ResourceManager::clearResource(): texture type resource expected!");
		}

	}

void ResourceManager::bindResource(ResourceId id, unsigned slot, bool unbind)
	{
	if (id >= resources.size())
	{
		foundation.printLine("ResourceManager::bindResource(): invalid resource id!");
		return;
	}

	Resource & r = resources[id];
	switch (r.type)
		{
		case ResourceType::eODSCRIPT_SHADER:
		case ResourceType::eGENERIC_HLSL:
		if (r.resourceSpecificIndex < shaderResources.size())
			{
			ShaderResource & res = shaderResources[r.resourceSpecificIndex];
			renderer.bindShader(res.ptr);
			}
			break;

		case ResourceType::eIMAGE:
		case ResourceType::eVOX_VOL:
		case ResourceType::eARRAY:
		case ResourceType::ePS_CONST:
		case ResourceType::eVS_CONST:
		case ResourceType::eCS_CONST:
		if (r.resourceSpecificIndex < textureResources.size())
			{
			TextureResource & res = textureResources[r.resourceSpecificIndex];
			renderer.bindTexture(res.ptr, slot, unbind);
			}

		break;
		default:
        foundation.printLine("ResourceManager::bindShader(): shader resource expected!");

		}
	}

void ResourceManager::drawMesh(ResourceId id)
	{
	ASSERT(id < resources.size());
	Resource & r = resources[id];
	if (r.type == ResourceType::eODSCRIPT_MESH)
		{
		if (r.resourceSpecificIndex < meshResources.size())
			{
			MeshResource & res = meshResources[r.resourceSpecificIndex];
			renderer.draw(res.vertexArrays, res.numVertexArrays, res.indexArray);
			}
		}
	else
          foundation.printLine("ResourceManager::drawMesh(): eODSCRIPT_MESH resource expected!");
	}

ResourceType::Enum ResourceManager::getResourceType(ResourceId id)
	{
	ASSERT(id < resources.size());
	Resource & r = resources[id];
	return r.type;
	}

ResourceType::Enum ResourceManager::resourceTypeFromFileName(const String & fname)
	{
	for (unsigned i = 0; i < sizeof(extMap) / sizeof(ExtensionsMap); i++)
	if (fname.endsWith(extMap[i].extension, extMap[i].strlen))
		return extMap[i].type;
	return ResourceType::eVOID;	//TODO: support more types
	}



ResourceManager gResourceManager;
ResourceManager & resourceManager = gResourceManager;


