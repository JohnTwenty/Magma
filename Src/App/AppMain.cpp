// AppMain.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <crtdbg.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#pragma message("_MSC_VER=" STR(_MSC_VER))

#pragma warning( disable : 4710)//function not inlined
#pragma warning(push)
#pragma warning( disable : 4668)//x  not defined as a preprocessor macro
#pragma warning( disable : 4365)//signed/unsigned mismatch
#pragma warning( disable : 4987)//nonstandard extension used
#pragma warning( disable : 4986)//exception specification does not match previous declaration
#pragma warning( disable : 4548) //expression before comma has no effect (malloc.h)

#include <iostream>
#include <SDL.h>

#if _MSC_VER > 1800
//this BS is needed for my old SDL to be compatible with VS2017
FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
	{
	return _iob;
	}
#endif

#pragma warning(pop)

#include "Foundation\Foundation.h"
#include "Foundation\FileManager.h"
#include "Foundation\CommandManager.h"
#include "Foundation\TaskManager.h"
#include "Foundation\StringManager.h"
#include "Foundation\VariableManager.h"
#include "Foundation\InputManager.h"
#include "Renderer\Renderer.h"
#include "Renderer\RenderTransforms.h"
#include "Resource\ResourceManager.h"


#pragma warning(push)
#undef free
#undef realloc
#pragma warning( disable : 4365) //C4365: 'argument' : conversion from 'unsigned int' to 'int'
#pragma warning( disable : 4061) //warning C4061: enumerator 'CompressionMask' in switch of enum
#include "PxPhysicsAPI.h"
using namespace physx;
#pragma warning(pop)

#pragma warning( disable : 4350) //template behavior change -- this has to stay off for the whole file

int crtAllocHook(int /*allocType*/, void* /*userData*/, size_t /*size*/, int
	/*blockType*/, long requestNumber, const unsigned char * /*filename*/, int	/*lineNumber*/)
	{
	static long rnb = -1;
	if (requestNumber == rnb)
		rnb = -1;

	return 1;
	}

void cmdSleep()
	{
	SDL_Delay(5000);
	}

void cmdSleepFor(int t)
{
	SDL_Delay((unsigned)t);
}

//this is just a test
void cmdAlertIfFileChanged()
	{
	foundation.printLine("TODO: Add async file change alerts!");
	}

CommandCallback renderCallback;

void cmdSetRenderCallback(CommandCallback cb)
	{
	renderCallback = cb;
	}

bool quitFlag = false;
unsigned gTime = 0;	//milliseconds since the SDL library initialized.
unsigned gLastMoveTime = 0;
PxTransform cam2world(PxIdentity);
PxReal camSpeed = 0.1f;
PxVec3 camVelBodySpace(PxZero);

void cmdAppLoop()
	{
	static unsigned lastTimeFPSWrite = 0;
	static unsigned gFrameCount = 0;
	for (;;)
		{
		//timing
		gTime = SDL_GetTicks();	

		if (gTime > (gLastMoveTime + 10))//update at 100 Hz
			{
			gLastMoveTime = gTime;
			gFrameCount++;

			//update fps counter
			if (gTime > (lastTimeFPSWrite + 1000))
				{
				lastTimeFPSWrite = gTime;
				char title[64];
				sprintf_s(title, "Magma - %d FPS", gFrameCount);
				SDL_SetWindowTitle(renderer.getWin(), title);
				gFrameCount = 0;
				}

			//update camera

			cam2world.p += cam2world.q.rotate(camVelBodySpace);

			//this should happen w custom key bind system
			inputManager.pumpEvents();

			if (quitFlag)
				return;

			//call render callback
			if (renderCallback.script)
				commandManager.runScript(renderCallback);
			else
				foundation.printLine("cmdAppLoop(): No Render callback has been set!");


			}


		}

	}

void cmdQuit(int down)
	{
	quitFlag = true;
	}

void cmdCamForward(int down)
	{
	camVelBodySpace[2] = down ? camSpeed : 0.0f;
	//cam2world.p += cam2world.q.getBasisVector2() * camSpeed;
	}

void cmdCamBack(int down)
	{
	camVelBodySpace[2] = down ? -camSpeed : 0.0f;
	//cam2world.p -= cam2world.q.getBasisVector2() * camSpeed;
	}

void cmdCamUp(int down)
	{
	camVelBodySpace[1] = down ? camSpeed : 0.0f;
	//cam2world.p += cam2world.q.getBasisVector1() * camSpeed;
	}

void cmdCamDown(int down)
	{
	camVelBodySpace[1] = down ? -camSpeed : 0.0f;
	//cam2world.p -= cam2world.q.getBasisVector1() * camSpeed;
	}

void cmdCamRight(int down)
	{
	camVelBodySpace[0] = down ? camSpeed : 0.0f;
	//cam2world.p += cam2world.q.getBasisVector0() * camSpeed;
	}

void cmdCamLeft(int down)
	{
	camVelBodySpace[0] = down ? -camSpeed : 0.0f;
	//cam2world.p -= cam2world.q.getBasisVector0() * camSpeed;
	}




PxQuat toQuat(float yaw, float pitch, float roll)
{
    // Abbreviations for the various angular functions
    float cy = cosf(yaw * 0.5f);
    float sy = sinf(yaw * 0.5f);
    float cp = cosf(pitch * 0.5f);
    float sp = sinf(pitch * 0.5f);
    float cr = cosf(roll * 0.5f);
    float sr = sinf(roll * 0.5f);

    PxQuat q;
    q.w = cy * cp * cr + sy * sp * sr;
    q.x = cy * cp * sr - sy * sp * cr;
    q.y = sy * cp * sr + cy * sp * cr;
    q.z = sy * cp * cr - cy * sp * sr;
    return q;
}


void cmdMouseFunc(int buttonMask, int x, int y)//shouldn't basic mouselook functionality be built into InputManager?
	{
	static int lastX, lastY;
	static bool isDragging = false; 
	const float mouseLookSpeedX =  0.005f;
	const float mouseLookSpeedY =  0.005f;	//invert y
	
	static float camRotX = 0.0f, camRotY = 0.0f;

	if (buttonMask & 1)	//left mouse drag to mouse look
		{
		if (!isDragging)
			{
			//start dragging
			isDragging = true;
			lastX = x;
			lastY = y;
			}
		else
			{
			int dx = x - lastX;
			int dy = y - lastY;

			lastX = x;
			lastY = y;

			camRotX += dx * mouseLookSpeedX;
			camRotY += dy * mouseLookSpeedY;

			cam2world.q = toQuat(0.0f, camRotX, camRotY);
			}		
		}
	else
		{
		isDragging = false;
		}

	}


void cmdSetTransformClipSpaceIdentity(const char * scriptVar)
	{
	PxMat44 id(PxIdentity);
	ResourceId rid = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));
	resourceManager.writeResource(rid, id.front(), sizeof(float) * 16);
	}

void cmdSetRenderTransform(const char * scriptVar)
	{
	RenderTransforms transforms;
	transforms.setProjection(2, 2, 2, 8);
	transforms.setActor2Scene(PxTransform(PxVec3(2.0f * sinf(gTime  / 1000.0f), 0.0f, 4.0f)));

	ResourceId rid = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));
	resourceManager.writeResource(rid, transforms.buildRenderMatrix(), sizeof(float) * 16);
	}


void cmdSetCommonPShaderConstants(const char * scriptVar)
	{
	//currently the window is not resizeable so we can just write the window size pixel shader constants:
	unsigned w, h;
	renderer.getWindowSize(w,h);
	float consts[] = { (float)w,	(float)h, ((float)gTime)*0.001f, 0.0f, cam2world.p.x, cam2world.p.y, cam2world.p.z, 0.0f, cam2world.q.x, cam2world.q.y, cam2world.q.z, cam2world.q.w };
	//renderer.setPixelShaderConstants(consts, 12);

	ResourceId rid = static_cast<unsigned>(variableManager.getVariableAsInt(scriptVar));
	resourceManager.writeResource(rid, consts, sizeof(float) * 12);
	}

void derivedAppRegisterCommands();	//defined in the specialized application 

int main(int /*argc*/, char** /*argv*/)
	{
	_CrtSetAllocHook(crtAllocHook);

	//switch cwd to media directory
	fileManager.findMediaDirectory();

	//register commands of all classes	TODO: the app should not have to do this.  Can't this just happen in module ctor?
	foundation.registerCommands();
	inputManager.registerCommands();
	resourceManager.registerCommands();
	renderer.registerCommands();

	taskManager.runTask(cmdAlertIfFileChanged);


	commandManager.addCommand("sleep", cmdSleep);
	commandManager.addCommand("sleepFor", cmdSleepFor);
	commandManager.addCommand("setRenderCallback", cmdSetRenderCallback);
	commandManager.addCommand("appLoop", cmdAppLoop);
	commandManager.addCommand("quit", cmdQuit);
	commandManager.addCommand("setCommonPShaderConstants", cmdSetCommonPShaderConstants);
	commandManager.addCommand("setTransformClipSpaceIdentity", cmdSetTransformClipSpaceIdentity);
	commandManager.addCommand("setRenderTransform", cmdSetRenderTransform);


	commandManager.addCommand("camForward", cmdCamForward);
	commandManager.addCommand("camLeft", cmdCamLeft);
	commandManager.addCommand("camRight", cmdCamRight);
	commandManager.addCommand("camBack", cmdCamBack);
	commandManager.addCommand("camUp", cmdCamUp);
	commandManager.addCommand("camDown", cmdCamDown);

	commandManager.addCommand("mouseFunc", cmdMouseFunc);

	derivedAppRegisterCommands();


	cmdRunCommands("AppMain.Commands.ods");


	taskManager.shutDown();
	resourceManager.shutDown();
	renderer.shutDown();
	inputManager.shutDown();
	commandManager.shutDown();
	stringManager.shutDown();
	variableManager.shutDown();

	SDL_Quit();
	//_CrtDumpMemoryLeaks();	//running this here is premature: globals will not get deallocated yet.
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF);
	return 0;
	}