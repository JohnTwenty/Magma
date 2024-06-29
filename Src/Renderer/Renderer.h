#pragma once

struct SDL_Window;
class ODBlock;

#include "Foundation\Array.h"
#include "Foundation\Allocator.h"
#include "Foundation\InlineAllocator.h"
#include "Foundation\Flags.h"


//In our engine, we will require that all vertex attributes
//be provided packed, and in separate buffers.  Furthermore,
//the ordering of vertex elements is always the same.
//While 3d APIs permit greater flexibility (eg. positions 
//and colors in one buffer and normals in another) supporting
//arbitrary layouts would significantly complicate the engine.
//Our simplifying assumption has the benefit that the vertex
//attributes of a vertex shader will completely define an
//input layout of a sequence of vertex buffers that any mesh
//can satisfy as long as it has the required elements available.
//The attribute ordering is that given by the below enum.
//If there are multiple attributes (e.g. texcoord sets) they are
//bound in separate vertex buffers, bud adjacently.
struct VertexAttributes
	{
	enum Enum
		{
		eVoid = 0,
		ePosition3f = 1,
		eNormal3f = 2,
		eColor4f = 3,
		eTex2f = 4
		};


	VertexAttributes() { attribs.reserve(4);  }	//reserve the size of the inline allocator
	VertexAttributes(const VertexAttributes & other)
		{
		//copy constructor
		attribs = other.attribs;
		}

	VertexAttributes& operator = (const VertexAttributes &other)
		{
		attribs = other.attribs;
		
		return *this;
		}

	void addAttrib(VertexAttributes::Enum e) { attribs.pushBack(static_cast<MxU8>(e)); }
	void read(ODBlock & block);
	unsigned sizeofVertex();	//return size of the vertex in bytes.
	unsigned sizeofAttrib(unsigned i);//size of attribs[i] at this index in bytes.

	Array<MxU8, InlineAllocator<4, Allocator>> attribs;
	};




struct RenderStateFlags
	{
	enum Enum
		{
		eWIRE = 0,
		eFILL = 1,
		eCULL_FRONT = 1 << 1,
		eCULL_BACK = 1 << 2,
		};
	};

FLAGS_OPERATORS(RenderStateFlags::Enum, unsigned char);

struct RenderState
	{
	MxFlags<RenderStateFlags::Enum> flags;
	};


struct PixelFormat
	{
	enum Enum
		{
		e_Ru8_Gu8_Bu8_Au8 = 0,
		e_Rf32_Gf32_Bf32_Af32 = 1,
		};
	};



class Renderer
	{
	public:
		Renderer(void);

		void registerCommands();

		// creates window.  If fullscreen is specified, width and height may be ignored.
		// can currently not be called more than once. (renderer only supports single window apps)
		void createWindow(unsigned int width, unsigned int height, bool fullscreen);

		//createContext(), clear(), etc are in theory pure virtual functions of this class but
		//given that at compile time it is known which renderer is being used, actually making the calls virtual is not necessary.

		void getWindowSize(unsigned &width, unsigned &height);

		SDL_Window *getWin();

	protected:

		RenderState currentState;	//should probably move to parent Renderer class.
	};

class RendererOGL;
class RendererD3D11;

//choose type of renderer to use here: Set one of these to 1
#define RENDERER_USE_OPENGL 0				
#define RENDERER_USE_D3D11  1


#if RENDERER_USE_OPENGL == 1
#include "RendererOGL.h"
#define RENDERER_CLASS RendererOGL	
#endif
#if RENDERER_USE_D3D11 == 1
#include "RendererD3D11.h"
#define RENDERER_CLASS RendererD3D11	
#endif

extern RENDERER_CLASS & renderer;

void cmdCreateWindow(int uWidth, int uHeight, int bFullscreen);
void cmdCreateContext();
void cmdCreateComputeContext();
void cmdPresent();
void cmdCompute(int xw, int yw, int zw);
void cmdSetRenderState(int RenderState);