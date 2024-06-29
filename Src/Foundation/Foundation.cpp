#include "StdAfx.h"
#include "Foundation.h"
#include "Foundation\CommandManager.h"

#pragma warning(push)
#pragma warning( disable : 4668)//x  not defined as a preprocessor macro
#pragma warning( disable : 4365)//signed/unsigned mismatch
#pragma warning( disable : 4987)//nonstandard extension used
#pragma warning( disable : 4548) //expression before comma has no effect
#include <SDL.h>
#pragma warning(pop)

#ifdef WIN32
#include <windows.h>
#endif

//functions:

void cmdPrint(const char * string)
	{
	foundation.printLine(string);
	}



Foundation::Foundation(void)
	{
	}


Foundation::~Foundation(void)
	{
	}

void Foundation::registerCommands()
	{
	commandManager.addCommand("print", cmdPrint);

	}

void Foundation::print(const char * string)
	{
	printf(string);
#ifdef WIN32
//	OutputDebugString(string);
#endif
	}

void Foundation::printLine(const char * string, const char * string2)
	{
	if (string) printf(string);
	if (string2) printf(string2);
	printf("\n");
	}


void Foundation::fatal(const char * string)
	{
	print(string);
	SDL_Quit();
	exit(0);
	}

void Foundation::assertViolation(const char* exp, const char* file, int line, bool& ignore)
	{
	print("assert violation");
	printf(": %s at %s line %d\nIgnore,Continue,Break,Abort?\n", exp, file, line);
	char c;
	for (;;)
		{
		scanf_s("%c", &c);
		switch (c)
			{
			case 'i':
			case 'I':
				ignore = true;
				return;
			case 'c':
			case 'C':
				ignore = false;
				return;
			case 'b':
			case 'B':
				SDL_TriggerBreakpoint();
				return;
			case 'a':
			case 'A':
				exit(0);
				return;
			default:
				printf("i,c,b,a?\n");
			}
		}
	}


Foundation gFoundation;
Foundation & foundation = gFoundation;