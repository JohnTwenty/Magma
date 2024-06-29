#pragma once


//A Task is a wrapper for a function entry point that can be executed on a thread
typedef void(*Task)(void);



//The TaskManager has a pool of threads and runs Tasks with them.
class TaskManager
	{
	public:
	TaskManager();
	~TaskManager();		//destructors for global singletons are called in undefined order after main thread already existed which is not great.  Call shutDown() manually instead.
	void shutDown();	//free all resources at exit

	void runTask(Task);	//tasks are not guaranteed to be launched in order.
	void stopAllWorkers();	//only returns once all work has stopped.


	};


extern TaskManager & taskManager;
/*
usage:

taskManager.runTask(fptr);//file update watcher!
taskManager.stopAllTasks();


*/