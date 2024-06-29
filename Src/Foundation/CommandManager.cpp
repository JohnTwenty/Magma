#include "App\stdAfx.h"
#include "Foundation\CommandManager.h"
#include "Foundation\StringManager.h"
#include "Foundation\ODBlock.h"
#include "Foundation\Foundation.h"

#define COMMAND_MAX_PARAMS 5	//TWO

struct CommandParameterType
	{
	enum Enum
		{
		eString, eInteger, eFloat, eCallback
		};

	};

struct CommandInvocation
	{
	CommandInvocation(unsigned commandIndex) : commandIndex(commandIndex) {}

	unsigned commandIndex;	//in CommandManager::commands

	union
		{
		unsigned stringIndex;
		int integer;
		float real;
		} parameters[COMMAND_MAX_PARAMS];	

	};


#define NUM_SIGNATURE_TYPES 8

struct CommandSignatureType
	{
	unsigned numParams;
	CommandParameterType::Enum signature[COMMAND_MAX_PARAMS];	//only first numParams members are valid.
	} commandSignatureTypes[NUM_SIGNATURE_TYPES];


struct Command
	{
	//this bit is ugly.  here we make C function calls based on dynamic type information, which is not supported in C
	//I am using a method that is portable and typesafe but one needs to edit this class to explicitly add every imaginable function signature explicitly.
	//As long as the usage of the class is simple and transparent I can live with that.
	//an alternative would be to use a library like http://www.dyncall.org/ 
	//currently each ctor initializes the commandsigtype for itself, which is redundant for a lot of functions with the same signature, but it's the simplest code right now
	Command(const char * name, void(*fptr) (void)) : name(name), fptrVoid(fptr), signatureType(0) 
	{ 
		commandSignatureTypes[signatureType].numParams = 0;  
	}
	Command(const char * name, void(*fptr) (const char *)) : name(name), fptrString(fptr), signatureType(1) 
	{ 
		commandSignatureTypes[signatureType].numParams = 1;  
		commandSignatureTypes[signatureType].signature[0] = CommandParameterType::eString; 
	}
	Command(const char * name, void(*fptr) (const char *, const char *)) : name(name), fptrStringString(fptr), signatureType(4)
		{ 
		commandSignatureTypes[signatureType].numParams = 2;  
		commandSignatureTypes[signatureType].signature[0] = CommandParameterType::eString; 
		commandSignatureTypes[signatureType].signature[1] = CommandParameterType::eString;
		}
	Command(const char * name, void(*fptr) (int)) : name(name), fptrInt(fptr), signatureType(3)
		{
		commandSignatureTypes[signatureType].numParams = 1;  
		commandSignatureTypes[signatureType].signature[0] = CommandParameterType::eInteger; 
		}
	Command(const char * name, void(*fptr) (int, int, int)) : name(name), fptrIntIntInt(fptr), signatureType(2)
		{ 
		commandSignatureTypes[signatureType].numParams = 3;  
		commandSignatureTypes[signatureType].signature[0] = CommandParameterType::eInteger; 
		commandSignatureTypes[signatureType].signature[1] = CommandParameterType::eInteger; 
		commandSignatureTypes[signatureType].signature[2] = CommandParameterType::eInteger; 
		}
	Command(const char * name, void(*fptr) (CommandCallback)) : name(name), fptrCallback(fptr), signatureType(5) 
		{ 
		commandSignatureTypes[signatureType].numParams = 1;  
		commandSignatureTypes[signatureType].signature[0] = CommandParameterType::eCallback; 
		}
	Command(const char * name, void(*fptr) (const char *, int)) : name(name), fptrStringInt(fptr), signatureType(6)
		{ 
		commandSignatureTypes[signatureType].numParams = 2;  
		commandSignatureTypes[signatureType].signature[0] = CommandParameterType::eString; 
		commandSignatureTypes[signatureType].signature[1] = CommandParameterType::eInteger;
		}
	Command(const char * name, void(*fptr) (const char *, const char *, const char *)) : name(name), fptrStringStringString(fptr), signatureType(7)
		{ 
		commandSignatureTypes[signatureType].numParams = 3;  
		commandSignatureTypes[signatureType].signature[0] = CommandParameterType::eString; 
		commandSignatureTypes[signatureType].signature[1] = CommandParameterType::eString;
		commandSignatureTypes[signatureType].signature[2] = CommandParameterType::eString;
		}

	//THREE: add more types of ctors here

	Command() : name(0), fptr(0) {}	//is this needed?

	void invoke()//simple version for void functions
		{
		if (signatureType == 0)
			fptrVoid();
		else
			foundation.printLine("CommandManager::invoke(): mismatched argument list for:", name);
		}

	void invoke (int a, int b, int c)
		{
		if (signatureType == 2)
			fptrIntIntInt(a, b, c);
		else
			foundation.printLine("CommandManager::invoke(): mismatched argument list for:", name);
	}

	void invoke(int a)
	{
		if (signatureType == 3)
			fptrInt(a);
		else
			foundation.printLine("CommandManager::invoke(): mismatched argument list for:", name);
	}

	void invoke(CommandInvocation & ci, ScriptPtr context)
		{ 
		switch (signatureType)
			{
			case 0:
				fptrVoid();
				break;

			case 1:
				fptrString(stringManager.lookupString(ci.parameters[0].stringIndex));
				break;
			case 2:
				fptrIntIntInt(ci.parameters[0].integer, ci.parameters[1].integer, ci.parameters[2].integer);
				break;
			case 3:
				fptrInt(ci.parameters[0].integer);
				break;
			case 4:
				fptrStringString(stringManager.lookupString(ci.parameters[0].stringIndex), stringManager.lookupString(ci.parameters[1].stringIndex));
				break;
			case 5:
				CommandCallback cb;
				cb.script = context;
				cb.name = ci.parameters[0].stringIndex;
				fptrCallback(cb);
				break;
			case 6:
				fptrStringInt(stringManager.lookupString(ci.parameters[0].stringIndex), ci.parameters[1].integer);
				break;
			case 7:
				fptrStringStringString(stringManager.lookupString(ci.parameters[0].stringIndex), stringManager.lookupString(ci.parameters[1].stringIndex), stringManager.lookupString(ci.parameters[2].stringIndex));
				break;
			default:
				ASSERT(0);//FOUR: if you hit this you need to implement any new signature types.
			}


		}

	const char * getName() { return name;  }
	unsigned getNumParams() { return commandSignatureTypes[signatureType].numParams; }
	CommandParameterType::Enum getParamType(unsigned i) { ASSERT(i < COMMAND_MAX_PARAMS); return  commandSignatureTypes[signatureType].signature[i]; }

	private:
	const char * name;	//overloading a name with multiple signatures is not supported.
	union
		{
		void * fptr;
		void(*fptrVoid) (void);
		void(*fptrString) (const char *);
		void(*fptrStringString) (const char *, const char *);
		void(*fptrStringStringString) (const char *, const char *, const char *);
		void(*fptrStringInt) (const char *, int);
		void(*fptrInt) (int);
		void(*fptrIntIntInt) (int, int, int);
		void(*fptrCallback) (CommandCallback);
		//FIVE: add more types here, adjusting NUM_SIGNATURE_TYPES as you go.
		};


	unsigned signatureType;

	};


struct CommandSequence		//also known as a script subroutine.
	{
	unsigned name;			//stored in stringManager
	Array <CommandInvocation, Allocator> calls;
	};

struct Script
	{
	Array <CommandSequence, Allocator> subs;	//subroutines.
	};



Array<Command, Allocator>		commands;	//the available commands.  TODO: replace with a hash for perf


CommandManager::CommandManager()
	{
	}

CommandManager::~CommandManager()
	{
	}

void CommandManager::shutDown()
	{
	commands.reset();		//have to release memory here explicitly because our memory system shuts down before global destructors run :(
	}



void CommandManager::addCommand(const char * name, void(*fptr) (void))
	{
	commands.pushBack(Command(name, fptr));
	}

void CommandManager::addCommand(const char * name, void(*fptr) (const char *))
	{
	commands.pushBack(Command(name, fptr));
	}

void CommandManager::addCommand(const char * name, void(*fptr) (const char *, const char *))
	{
	commands.pushBack(Command(name, fptr));
	}

void CommandManager::addCommand(const char * name, void(*fptr) (const char *, const char *, const char *))
	{
	commands.pushBack(Command(name, fptr));
	}

void CommandManager::addCommand(const char * name, void(*fptr) (int))
	{
	commands.pushBack(Command(name, fptr));
	}

void CommandManager::addCommand(const char * name, void(*fptr) (int, int, int))
	{
	commands.pushBack(Command(name, fptr));
	}

void CommandManager::addCommand(const char * name, void(*fptr) (CommandCallback))
	{
	commands.pushBack(Command(name, fptr));
	}

void CommandManager::addCommand(const char * name, void(*fptr) (const char *, int))
	{
	commands.pushBack(Command(name, fptr));
	}



void CommandManager::callCommand(unsigned commandIndex)	//only works for void commands for now.
	{
	ASSERT(commandIndex < commands.size());
	commands[commandIndex].invoke();	
	}

void CommandManager::callCommand(unsigned commandIndex, int a, int b, int c)
    {
	ASSERT(commandIndex < commands.size());
	commands[commandIndex].invoke(a, b, c);	
	}

void CommandManager::callCommand(unsigned commandIndex, int a)
{
	ASSERT(commandIndex < commands.size());
	commands[commandIndex].invoke(a);
}

unsigned CommandManager::findCommand(const char * commandName)	//returns the command index if found or ~0u otherwise
	{
	for (unsigned i = 0; i < commands.size(); i++)	//TODO: try to hash or something.  
		{
		if (!strcmp(commands[i].getName(), commandName))
			return i;
		}
	foundation.printLine("CommandManager::findCommand(): Command not found:", commandName);
	return ~0u;
	}

Command * CommandManager::findCommand(const char * commandName, unsigned & indexOut)
	{
	for (unsigned i = 0; i < commands.size(); i++)	//TODO: try to hash or something.  
		{
		if (!strcmp(commands[i].getName(), commandName))
			{
			indexOut = i;
			return &(commands[i]);
			}
		}
	foundation.printLine("CommandManager::findCommand(): Command not found:", commandName);
	return NULL;
	}

ScriptPtr CommandManager::createScript(ODBlock & odBlock)
	{
	unsigned numSubs = odBlock.numSubBlocks();


	Script * s = new Script;
	s->subs.reserve(numSubs);

	//so that we can have subs call eachother we need to parse in two passes
	//first we go through the sub definitions and we register them all in the sting manager and create CommandSequences for each.
	//then we load each command sequence.

	odBlock.reset();
	for (unsigned i = 0; i < numSubs; i++)
		{
		ODBlock * subBlock = odBlock.nextSubBlock();
		ASSERT(subBlock);
		s->subs.pushBack(CommandSequence());
		CommandSequence & cs = s->subs.back();
		cs.name = stringManager.addString(subBlock->ident());
		}

	odBlock.reset();
	for (unsigned i = 0; i < numSubs; i++)
		{
		ODBlock * subBlock = odBlock.nextSubBlock();
		ASSERT(subBlock);

		unsigned numCommands = subBlock->numSubBlocks();
		subBlock->reset();
		CommandSequence & cs = s->subs[i];

		for (unsigned c = 0; c < numCommands; c++)
			{
			ODBlock * commandBlock = subBlock->nextSubBlock();
			ASSERT(commandBlock);

			addCommandToSequence(cs, *commandBlock, s);
			}
		}


	return s;
	}

void CommandManager::addCommandToSequence(CommandSequence & cs, ODBlock & call, ScriptPtr context)
	{
	unsigned commandIndex; 
	Command * cptr = findCommand(call.ident(), commandIndex);
	if (cptr != NULL)
		{
		//check if enough arguments are provided and whether they can be converted to the types needed by the function

		call.reset();


		CommandInvocation cinv(static_cast<unsigned>(commandIndex));

		for (unsigned int n = 0; n<cptr->getNumParams(); n++)
			{
			if (call.moreTerminals())
				{
				CommandParameterType::Enum pType = cptr->getParamType(n);
				switch (pType)
					{
					case CommandParameterType::eString:
						cinv.parameters[n].stringIndex = stringManager.addString(call.nextTerminal());
						break;
					case CommandParameterType::eCallback:
						{
						CommandSequence * cs = findSubroutine(context, call.nextTerminal());
						cinv.parameters[n].stringIndex = cs ? cs->name : ~0u;
						}
						break;
					case CommandParameterType::eInteger:
						cinv.parameters[n].integer = call.nextTerminalAsInt();
						break;
					case CommandParameterType::eFloat:
						cinv.parameters[n].real = call.nextTerminalAsFloat(); 
						break;
					default:
						ASSERT(0);	//SEVEN: this should not happen

					}
				}
			else
				{
				//not enough params!!  
				foundation.print("CommandManager::addCommandToSequence: not enough params for command ");
				foundation.print(call.ident());
				foundation.print("!\n");
				//maybe we could add defaults?  seems error prone though.
				//return, throwing away the command invocation.
				return;
				}
			}
		if (call.moreTerminals())
			{
			//too many params!
			foundation.print("CommandManager::addCommandToSequence: too many params for command ");
			foundation.print(call.ident());
			foundation.print("!\n");
			//we make the call anyway, ignoring extra params.
			}
		cs.calls.pushBack(cinv);
		}
	else
		{
		//command not found!
		foundation.print("CommandManager::addCommandToSequence: command ");
		foundation.print(call.ident());
		foundation.print(" not found!\n");
		}

	}

CommandSequence* CommandManager::findSubroutine(ScriptPtr ptr, const char * subName)//this code binds the intra script function calls
	{
	ASSERT(ptr);
	if (ptr->subs.size())
		{
		if (subName)
			{
			for (CommandSequence * cs = ptr->subs.begin(); cs < ptr->subs.end(); cs++)
				{
				const char * csName = stringManager.lookupString(cs->name);
				ASSERT(subName);
				if (!strcmp(subName, csName))
					{
					//call this one!
					return cs;
					}
				}

			}
		}
	return 0;
	}

void CommandManager::releaseScript(ScriptPtr cs)
	{
	delete cs;
	}

void CommandManager::runScript(ScriptPtr ptr, const char * subName)
	{
	ASSERT(ptr);
	if (ptr->subs.size())
		{
		if (subName)
			{
			CommandSequence * cs = findSubroutine(ptr, subName);
			if (cs)
				runCommandSequence(*cs, ptr);
			/*
			for (CommandSequence * cs = ptr->subs.begin(); cs < ptr->subs.end(); cs++)
				{
				const char * csName = stringManager.lookupString(cs->name);
				ASSERT(subName);
				if (!strcmp(subName, csName))
					{
					//call this one!
					runCommandSequence(*cs, ptr);
					}

				}
			*/
			
			}
		else
			//run runs first sub by default, if there are several
			//by convention it should be called 'main' and eventually we could just look for it in the list
			runCommandSequence(ptr->subs.front(), ptr);
		}

	}

void CommandManager::runScript(CommandCallback cb)//faster version of runScript that doesn't have to do string compares.
	{
	ScriptPtr ptr = cb.script;
	ASSERT(ptr);
	if (ptr->subs.size())
		{
		for (CommandSequence * cs = ptr->subs.begin(); cs < ptr->subs.end(); cs++)
			{
			if (cs->name == cb.name)
				runCommandSequence(*cs, ptr);
			}
		}
	}


void CommandManager::runCommandSequence(CommandSequence & cs, ScriptPtr context)
	{
	for (CommandInvocation * ci = cs.calls.begin(); ci < cs.calls.end(); ci++)
		commands[ci->commandIndex].invoke(*ci, context);
	}


extern CommandManager & commandManager;


CommandManager gCommandManager;
CommandManager & commandManager = gCommandManager;