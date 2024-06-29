#pragma once
#include "Renderer.h"
#include "Foundation\Types.h"

struct ID3D11Device2;
struct ID3D11DeviceContext2;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11DeviceChild;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11Debug;
struct ID3D11RasterizerState;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct ID3D11SamplerState;
struct ID3D11Resource;

//struct SDL_Surface;



struct ShaderType
{
	enum Enum
	{
	eVERTEX,
	ePIXEL,
	eCOMPUTE,
	eUNKNOWN
	};
};

struct ShaderPtr						//conceptually a ptr, even though it worked out to be two pointers.  
										//this should be opaque to the user!!
										//its passed by value so it should be small!
{
	ShaderPtr(ShaderType::Enum t, ID3D11DeviceChild* s, ID3D11InputLayout* i = NULL) : type(t), shader(s), inputLayout(i) {}
	ShaderPtr() : type(ShaderType::eUNKNOWN), shader(NULL), inputLayout(NULL) { }	//so we can have NULL handles.
private:
	ShaderType::Enum	type;			//type of shader
	ID3D11DeviceChild*	shader;			//vertex or pixel shader
	ID3D11InputLayout*	inputLayout;	//only used for vertex shaders

friend class RendererD3D11;
};

struct TextureType	//texture is a bit misnomer, its just a resource
{
	enum Enum
	{
	eShaderResource,
	eRenderTarget,		//a special kind of ShaderResource
	eComputeBuffer,
	eVSConstantBuffer,
	ePSConstantBuffer,
	eCSConstantBuffer,
	eUnknown
	};
};


struct TexturePtr
{

	TexturePtr(ID3D11Resource * t, ID3D11ShaderResourceView * r) : type(TextureType::eShaderResource), texture(t), shaderResourceView(r){}		//read only textures
	TexturePtr(ID3D11Resource * t, ID3D11UnorderedAccessView  * r) : type(TextureType::eComputeBuffer), texture(t), accessView(r) {}			//read write textures
	TexturePtr(ID3D11Resource* t, ID3D11RenderTargetView* r) : type(TextureType::eRenderTarget), texture(t), rtView(r) {}
	TexturePtr(ID3D11Buffer* b, TextureType::Enum  ty);
	TexturePtr() : type(TextureType::eUnknown), texture(NULL), shaderResourceView(NULL) {}
private:
	TextureType::Enum  type;
	ID3D11Resource * texture;			//e.g. ID3D11Texture2D or ID3D11Texture3D
	union
		{
		ID3D11ShaderResourceView	* shaderResourceView;	//this is the thing that can be bound to the shader.
		ID3D11UnorderedAccessView	* accessView;
		ID3D11RenderTargetView		* rtView;				//untested
		ID3D11Buffer				* constantBuffer; 
		};

friend class RendererD3D11;
};

struct GeometryArray					//its passed by value so it should be small!
{
	GeometryArray(ID3D11Buffer	*b, MxU32 elemSize, MxU32 nElems) : geometryBuffer(b), elementSize(elemSize), numElements(nElems) {}
	GeometryArray() : geometryBuffer(NULL), elementSize(0), numElements(0) {}	//so we can have NULL handles.

private:
	ID3D11Buffer*	geometryBuffer;		//vertex or index buffer
	MxU32			elementSize;		//vertex or index size
	MxU32			numElements;		//num vertices or indices

friend class RendererD3D11;
};


class RendererD3D11 :
	public Renderer
{
public:
	RendererD3D11(void);
	
	void createContext(unsigned w, unsigned h, bool renderWithCompute = false);
	void present();
	void compute(unsigned xw, unsigned yw, unsigned zw);
	void shutDown();

	ShaderPtr createShader(const void * code, size_t length, const char * name, ShaderType::Enum type, bool source, const char * cacheFName, VertexAttributes * va = NULL);	//returns ptr on success, which we don't hold on to, that's for the resource manager to own. source is true if uncompiled source data, otherwise binary compiled data. cacheFName is where to cache the compiled shader.
	void bindShader(ShaderPtr);
	void releaseShader(ShaderPtr);


	TexturePtr createSwapChain();
	TexturePtr createRenderTarget();
	TexturePtr createTexture(unsigned w, unsigned h, void * pixels = NULL, unsigned pitch = 0, PixelFormat::Enum pf = PixelFormat::e_Ru8_Gu8_Bu8_Au8);
	//TexturePtr createTexture(SDL_Surface &);
	TexturePtr createVolumeTexture(MxU32 width, MxU32 height, MxU32 depth, unsigned char * voxels);	//8 bpp!
	TexturePtr createTiledVolumeTexture(MxU32 width, MxU32 height, MxU32 depth);	//a large, potentially sparse volume texture backed by virtual memory.

	TexturePtr createStructuredBuffer(unsigned structSize, unsigned numElems, const void * data);
	TexturePtr createMappableBuffer(unsigned structSize, unsigned numElems, TextureType::Enum  type);

	void clearTexture(TexturePtr);
	void bindTexture(TexturePtr, unsigned slot = 0, bool unbind = false);
	void releaseTexture(TexturePtr);

	void * lockTexture(TexturePtr, unsigned & rowPitch, unsigned & depthPitch);	//can only be used on buffers created w usage D3D11_USAGE_DYNAMIC, which is slower than other usages.
	void unlockTexture(TexturePtr);
	void setBufferData(TexturePtr, unsigned structSize, unsigned numElems, const void * data);
	void writeToTiledVolumeTexture(TexturePtr, MxU32 x, MxU32 y, MxU32 z, MxU32 width, MxU32 height, MxU32 depth, unsigned char * voxels);//writes voxels to a region of the sparse volume, args are pixel coordinates.


	void setRenderState(const RenderState &);

	GeometryArray createVertexArray(const float * vertexData, MxU32 vertexSizeBytes, MxU32 vertexCount);
	GeometryArray createIndexArray(const MxU32 * indexData, MxU32 indexSizeBytes, MxU32 indexCount);
	void releaseGeometryArray(GeometryArray);
	void draw(GeometryArray * vertexArrays, MxU32 numVertexArrays, GeometryArray indexArray);		//we could add options later to draw a subset of the vertices (offset + count)


private:
	void allocateTiledVolumeTextureMemory(MxU32 width, MxU32 height, MxU32 depth);	//allocate actual memory for virtual memory based volume textures (createVolumeTiledResource) this is size in pixels. 

	bool renderWithCompute;									//todo: we should just remove the not render with compute version, its obsolete.
	ID3D11Device2 * device;
	ID3D11DeviceContext2* deviceContext;
	IDXGISwapChain * swapChain;
	ID3D11RasterizerState* rasterState;
	ID3D11SamplerState* sampleState;

#if defined(_DEBUG)
	ID3D11Debug * debug;
#endif
	};

