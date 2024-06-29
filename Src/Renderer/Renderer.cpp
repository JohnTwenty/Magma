#include "StdAfx.h"
#include "Foundation\Foundation.h"
#include "Renderer\Renderer.h"

#pragma warning(push)
#pragma warning( disable : 4548) //expression before comma has no effect
#pragma warning( disable : 4668)//x  not defined as a preprocessor macro
#pragma warning( disable : 4365)//signed/unsigned mismatch
#pragma warning( disable : 4987)//nonstandard extension used

#include <SDL.h>
#pragma warning(pop)

#include "Foundation\ODBlock.h"
#include "Foundation\CommandManager.h"



//pull in PhysX just for stupid matrix class
#pragma warning(push)
#undef free
#undef realloc
#pragma warning( disable : 4365) //C4365: 'argument' : conversion from 'unsigned int' to 'int'
#pragma warning( disable : 4061) //warning C4061: enumerator 'CompressionMask' in switch of enum
#include "foundation\PxMat44.h"
#include "foundation\PxFlags.h"
using namespace physx;
#pragma warning(pop)





struct AttributeMap
	{
	const char * name;
	VertexAttributes::Enum type;
	unsigned size;
	};

static const AttributeMap attributeMap[] =
	{
	//same order as in VertexAttributes::Enum !!
		{ "POSITION", VertexAttributes::ePosition3f, 3 * sizeof(float) },	//TODO: differentiate between 2, 3, and 4d positions.
		{ "NORMAL", VertexAttributes::eNormal3f, 3 * sizeof(float) },
		{ "COLOR", VertexAttributes::eColor4f, 4 * sizeof(float) },
		{ "TEX2", VertexAttributes::eTex2f, 2 * sizeof(float) },
	};


//vars:
SDL_Window *win;


//functions:
void cmdCreateWindow(int uWidth, int uHeight, int bFullscreen)
	{
	renderer.createWindow(static_cast<unsigned int>(uWidth), static_cast<unsigned int>(uHeight), bFullscreen ? true : false);
	}

void cmdCreateContext()
	{

	int width;
	int height;
	SDL_GetWindowSize(renderer.getWin(), &width, &height);

	renderer.createContext(width, height);
	}

void cmdCreateComputeContext()
	{
	int width;
	int height;
	SDL_GetWindowSize(renderer.getWin(), &width, &height);
	renderer.createContext(width, height, true);
	}

void cmdPresent()
	{
	renderer.present();
	}

void cmdCompute(int xw, int yw, int zw)
	{
	renderer.compute(xw, yw, zw);
	}

void cmdSetRenderState(int r)
	{
	//unfortunately this is horribly type unsafe but our scripting system is typeless...
	RenderStateFlags::Enum rsf;
	rsf = (RenderStateFlags::Enum )r;
	RenderState rs;
	rs.flags = rsf;
	renderer.setRenderState(rs);
	}

void VertexAttributes::read(ODBlock & block)
	{
	block.reset();
	while (block.moreTerminals())
		{
		const char * t = block.nextTerminal();
		for (unsigned i = 0; i < sizeof(attributeMap) / sizeof(AttributeMap); i++)
			{
			if (strcmp(attributeMap[i].name, t) == 0)
				{
				addAttrib(attributeMap[i].type);
				goto next;
				}
			}
		foundation.printLine("VertexAttributes::read(): Unknown vertex attribute found.");	// ignore it.
	next:;
		}
	}

unsigned VertexAttributes::sizeofVertex()	//return size of the vertex in bytes.
	{
		unsigned sum = 0;
		for (unsigned i = 0; i < attribs.size(); i++)
		{
			ASSERT(attribs[i] >= 1);
			sum += attributeMap[attribs[i]-1].size;
		}
		return sum;
	}

unsigned VertexAttributes::sizeofAttrib(unsigned i)
	{
	ASSERT(attribs[i] >= 1);
	return attributeMap[attribs[i] - 1].size;
	}



Renderer::Renderer()
	{
	win = 0;
	}

void Renderer::registerCommands()
	{
	commandManager.addCommand("createWindow", cmdCreateWindow);
	commandManager.addCommand("createContext", cmdCreateContext);
	commandManager.addCommand("createComputeContext", cmdCreateComputeContext);
	commandManager.addCommand("present", cmdPresent);
	commandManager.addCommand("compute", cmdCompute);
	commandManager.addCommand("setRenderState", cmdSetRenderState);

	}

// creates window.  If fullscreen is specified, width and height may be ignored.
void Renderer::createWindow(unsigned int width, unsigned int height, bool /*fullscreen*/)
	{

	if (win)
		return;	//error, we already have a window.  user can't call this more than once.

	//First we need to start up SDL, and make sure it went ok
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		{
		foundation.printLine("Renderer::createWindow(): SDL_Init() error:", SDL_GetError());
		foundation.fatal("SDL_Init() failed");
		return;
		}

	win = SDL_CreateWindow("Magma", 100, 100, (int)width, (int)height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);		//TODO: this already presupposes renderer type!!

	//Make sure creating our window went ok
	if (win == nullptr)
		{
		foundation.printLine("Renderer::createWindow(): SDL_CreateWindow() error:", SDL_GetError());
		foundation.fatal("SDL_CreateWindow failed");
		return;
		}

	}

void Renderer::getWindowSize(unsigned & width, unsigned & height)
	{
	SDL_GetWindowSize(win, (int *)&width, (int *)&height);
	}

SDL_Window * Renderer::getWin()
	{
	return win;
	}


//note: renderer singleton defined in renderer specialization class.

