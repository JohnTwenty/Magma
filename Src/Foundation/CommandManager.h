#pragma once
#include "..\Foundation\Allocator.h"
#include "..\Foundation\Array.h"

class ODBlock;

struct Command;		//a command is a C function that is callable from script.
struct CommandSequence;

struct Script;		//a script is a bunch of interpreted code in an ODS file.
typedef Script * ScriptPtr;	//this is used as a resource handle

struct CommandCallback
	{
	ScriptPtr script;
	unsigned name;		//name of subroutine stored in stringManager
	//const char * name;
	};

//TODO: As much as I hate templates I think this needs to be recoded with them because the amount of places I need to add versions of code per signature to preserve type safety is getting ridiculous.

class CommandManager
	{
	public:
		CommandManager();
		~CommandManager();
		void shutDown();

		//commands:
		void addCommand(const char * name, void(*fptr) (void));
		void addCommand(const char * name, void(*fptr) (const char *));
		void addCommand(const char * name, void(*fptr) (const char *, const char *));
		void addCommand(const char * name, void(*fptr) (const char *, const char *, const char *));
		void addCommand(const char * name, void(*fptr) (int));
		void addCommand(const char * name, void(*fptr) (int, int, int));
		void addCommand(const char * name, void(*fptr) (CommandCallback));
		void addCommand(const char * name, void(*fptr) (const char *, int));
		//ONE: add more parameter signatures.


		unsigned findCommand(const char * commandName);	//returns the command index if found or ~0u otherwise
		void callCommand(unsigned commandIndex);		//TODO: add more types
		void callCommand(unsigned commandIndex, int, int, int);
		void callCommand(unsigned commandIndex, int);

		//Scripts:
	/*
	#command sequence

	Commands
		{
		main
			{
			command0;
			command0;
			command1;
			load { 4; }
			div { 2; }
			mad { 3; 5; }
			store { result; }
			}
		}
	*/

		ScriptPtr createScript(ODBlock & );	//result owned by the caller.
		void releaseScript(ScriptPtr);
		void runScript(ScriptPtr, const char * sub = 0);	//runs the first sub by default
		void runScript(CommandCallback);	

	private:
		Command * findCommand(const char * commandName, unsigned & indexOut);	//returns the ptr of the command in commands, or NULL if not found.  Also sets the index if found.
		void addCommandToSequence(CommandSequence & , ODBlock & call, ScriptPtr );
		void runCommandSequence(CommandSequence & , ScriptPtr );
		CommandSequence* findSubroutine(ScriptPtr, const char * name);//this code binds the intra script function calls; returns stringManager index.
	};

extern CommandManager & commandManager;