#pragma once


//string table class for efficient string management
//can only add strings, not remove


class StringManager
	{
	public:
	StringManager();
	~StringManager();
	
	void shutDown();	//free all resources at exit

	unsigned addString(const char *);	//returns index in table.  Always adds, doesn't check for duplicates!
	const char * lookupString(unsigned);

	};

extern StringManager & stringManager;