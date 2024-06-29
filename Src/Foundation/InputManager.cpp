#include "App\stdAfx.h"
#include "Foundation\InputManager.h"
#include "Foundation\Allocator.h"
#include "Foundation\Array.h"
#include "Foundation\CommandManager.h"

#pragma warning(push)
#pragma warning( disable : 4668)//x  not defined as a preprocessor macro
#pragma warning( disable : 4365)//signed/unsigned mismatch
#include <SDL.h>
#pragma warning(pop)


struct InputBinding
	{
	InputBinding() : commandIndex(~0u) {}

	unsigned commandIndex;	//command to execute.  Has to be of type void f(int eventIndex, int details);

	};

unsigned mouseFuncCommandIndex = ~0u;			//mouse callback func


Array<InputBinding, Allocator> keyBindings(256);		//map from [SDL_Scancode] to an InputBinding.	256 because that is the range of important scan codes.


InputManager::InputManager()
	{
	}

InputManager::~InputManager()
	{
	}

void InputManager::registerCommands()
	{
	commandManager.addCommand("bindKey", cmdBindKey);
	commandManager.addCommand("bindMouse", cmdBindMouse);
	}

void InputManager::shutDown()
	{
	keyBindings.reset();
	}

void InputManager::pumpEvents()
	{
	SDL_Event event;
	while (SDL_PollEvent(&event)) 
		{
		/* handle your event here */
		switch (event.type)
			{
			case SDL_QUIT:		//this happens on window X or alt F4.
				commandManager.callCommand(commandManager.findCommand("quit"), 1);
			break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				{
				if (event.key.keysym.scancode < keyBindings.size() && event.key.repeat == 0)	//ignore key repeat events
					if (keyBindings[event.key.keysym.scancode].commandIndex != ~0u)	//else this key is not bound
						commandManager.callCommand(keyBindings[event.key.keysym.scancode].commandIndex, (event.type == SDL_KEYDOWN) ? 1 : 0);
				}
			break;
			case SDL_MOUSEMOTION:
				{
				int mbDownBits = (event.motion.state & SDL_BUTTON_LMASK) ? 1 : 0;
				mbDownBits |= (event.motion.state & SDL_BUTTON_MMASK) ? 2 : 0;
				mbDownBits |= (event.motion.state & SDL_BUTTON_RMASK) ? 4 : 0;
				if (mouseFuncCommandIndex != ~0u)	//if callback bound
					commandManager.callCommand(mouseFuncCommandIndex, mbDownBits, event.motion.x, event.motion.y);
				}
			break;
			}
		}
	}


void cmdBindKey(const char * keyname, const char * commandName)
	{
	//map keyname to an index

	//SDL_GetScancodeName
	SDL_Scancode sc = SDL_GetScancodeFromName(keyname);
	unsigned commandIndex = commandManager.findCommand(commandName);
	if (sc < keyBindings.size() && commandIndex != ~0u)
		keyBindings[sc].commandIndex = commandIndex;

	}

void cmdBindMouse(const char* commandName)
	{
	unsigned commandIndex = commandManager.findCommand(commandName);
	if (commandIndex != ~0u)
		mouseFuncCommandIndex = commandIndex;

	}



InputManager gInputManager;
InputManager & inputManager = gInputManager;
