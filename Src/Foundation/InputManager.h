#pragma once



class InputManager
	{
	public:
	InputManager();
	~InputManager();

	void registerCommands();
	void shutDown();
	
	void pumpEvents();		//pump events from the OS to our app.

	};


extern InputManager & inputManager;

void cmdBindKey(const char * keyname, const char * command);
void cmdBindMouse(const char * command);	//command must take 3 ints: (mbDownBits, x, y)
//could also add:
//void cmdBindKeyToCallback(const char * keyname, CommandCallback);	//this would be to call a script function