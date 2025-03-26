#include "App\stdAfx.h"
#include "Foundation\VariableManager.h"
#include "Foundation\Allocator.h"
#include "Foundation\Array.h"


const unsigned maxVarLength = 16;

struct Variable
	{
	Variable() { name[0] = 0; sValue = 0; }		//default value is zero because it sort of makes sense for all variable type casts.
	char name[maxVarLength];					//variables have max 15 letters. 0 terminated.  name[0] == 0 means slot is unused.

	union
		{
		int iValue;
		const char * sValue;
		//add more types of value interprets.
		};

	};

const unsigned hashTableSize = 128;

Array<Variable, Allocator>			variables(hashTableSize);


unsigned jenkins_one_at_a_time_hash(const char *string)
	{
	unsigned  hash = 0;
	while (*string)
		{
		hash += *string;
		hash += (hash << 10);
		hash ^= (hash >> 6);
		string++;
		}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
	}

VariableManager::VariableManager()
	{
	//variables.reserve(hashTableSize);
	}

VariableManager::~VariableManager()
	{

	}

void VariableManager::shutDown()
	{
	variables.reset();
	}

void VariableManager::setVariable(const char * name, int value)
	{
	unsigned index = findVariable(name);
	variables[index].iValue = value;
	}

void VariableManager::setVariable(const char * name, const char * value)
	{
	unsigned index = findVariable(name);
	variables[index].sValue = value;
	}

int VariableManager::getVariableAsInt(const char * name)
	{
	unsigned index = findVariable(name);
	int value = variables[index].iValue;
	return value;
	}

unsigned VariableManager::findVariable(const char * name)
	{
	unsigned hashIndex = jenkins_one_at_a_time_hash(name) % hashTableSize;
	const char * result = variables[hashIndex].name;
	while (strncmp(result, name, maxVarLength - 1))	//we truncate variables so we should only base comparison on the truncated portion. Minus one to correct for null terminator.
		{
		if (variables[hashIndex].name[0] == 0)
			{
			//this is an unused slot, create the variable here

			strncpy_s(variables[hashIndex].name, maxVarLength, name, maxVarLength - 1);
			variables[hashIndex].name[maxVarLength - 1] = 0;	//force terminating zero in case new var is too long
			return hashIndex;
			}
		hashIndex++;	//linear probing strategy
		if (hashIndex >= variables.size())
			{
			//ran past end and didn't find, append at end.
			variables.pushBack(Variable());
			return variables.size() - 1;
			}
		result = variables[hashIndex].name;
		}
	return hashIndex;
	}


VariableManager gVariableManager;
VariableManager & variableManager = gVariableManager;
