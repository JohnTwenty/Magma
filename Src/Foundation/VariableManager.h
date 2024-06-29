#pragma once


//system that lets us assign and use variables in ODS scripts.
class VariableManager
	{
	public:
	VariableManager();
	~VariableManager();

	void shutDown();	//free all resources at exit


	void setVariable(const char * name, int value);
	void setVariable(const char * name, const char * value);

	int getVariableAsInt(const char * name);	//works like vars in Basic -- basically variables that have not been set default to zero.

	private:

	unsigned findVariable(const char * name);

	};


extern VariableManager & variableManager;