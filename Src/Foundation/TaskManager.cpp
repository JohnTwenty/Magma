#include "App\stdAfx.h"
#include "Foundation\TaskManager.h"
#include "Foundation\Foundation.h"
#include "Foundation\Array.h"
#include "Foundation\Allocator.h"
#include <SDL.h>

TaskManager gTaskManager;
TaskManager & taskManager = gTaskManager;

//This implementation is windows specific.  If we want to make this code portable this class needs multiple implementations.

const unsigned numThreadsToCreate = 3;	//this is arbitrary, should be CPU core count dependent.
//const unsigned timeOut = 800;			//we check to stopWorkerThreads every such miliseconds.

bool stopWorkerThreads = 0;				//if this becomes true, worker threads should finish.
unsigned numActiveThreads = 0;			//also protected by the below mutex.

Array<Task, Allocator> taskArray(16);	//ok, this is actually a stack because we're poor and don't have a queue handy.
SDL_mutex * taskArrayMutex = 0;
SDL_cond* tasksReadyCondition = 0;



int workerThreadMain(void *)			//parameter unused
	{
	for(;;)
		{
		if (SDL_LockMutex(taskArrayMutex))
			foundation.fatal("TaskManager::workerThreadMain(): SDL_LockMutex() failed!");

		//wait for work		
		//SDL_CondWaitTimeout(tasksReadyCondition, taskArrayMutex, timeOut);	//sleep until there is work, or until a timeout which is needed to periodically check stopWorkerThreads.
		SDL_CondWait(tasksReadyCondition, taskArrayMutex);	//timeout no longer needed

		//stop if requested
		if (stopWorkerThreads)
			{
			numActiveThreads--;					//as soon as this reaches 0 the app can shut down and try to destroy the currently locked mutex which is illegal plus would make the below call fail.
												//For this reason we have to grab the lock one last time on the main thread before quitting.
			SDL_UnlockMutex(taskArrayMutex);
			return 0;
			}


		Task task = 0;
		if (taskArray.size())
			task = taskArray.popBack();

		SDL_UnlockMutex(taskArrayMutex);

		if (task)
			task();
		}
	}

TaskManager::TaskManager()
	{
	//create a mutex to deny simultaneous access to the task queue
	taskArrayMutex = SDL_CreateMutex();
	tasksReadyCondition = SDL_CreateCond();
	if (taskArrayMutex == 0 || tasksReadyCondition == 0)
		foundation.fatal("TaskManager::TaskManager(): SDL_CreateMutex/Condition() failed!");



	//create a bunch of SDL threads each of which just runs our scheduler function
	//the scheduler function waits until tasks arrive, then pops them off a queue and runs them
	for (unsigned i = 0; i < numThreadsToCreate; i++)
		{
		SDL_Thread *thread = SDL_CreateThread(workerThreadMain, "workerThread", 0);

		if (thread == 0)
			foundation.printLine("TaskManager::TaskManager(): Error creating worker thread!");
		//no reason to hold onto thread yet.
		}
	numActiveThreads = numThreadsToCreate;

	}

TaskManager::~TaskManager()
	{
	//don't free dynamic memory here.
	}

void TaskManager::shutDown()
	{
	stopAllWorkers();

	if (taskArrayMutex)
		SDL_DestroyMutex(taskArrayMutex);
	if (tasksReadyCondition)
		SDL_DestroyCond(tasksReadyCondition);
	taskArrayMutex = 0;
	tasksReadyCondition = 0;

	taskArray.reset();
	}

void TaskManager::runTask(Task task)
	{
	if (SDL_LockMutex(taskArrayMutex))
		foundation.fatal("TaskManager::workerThreadMain(): SDL_LockMutex() failed!");

	taskArray.pushBack(task);
	SDL_CondSignal(tasksReadyCondition);
	SDL_UnlockMutex(taskArrayMutex);
	}

void TaskManager::stopAllWorkers()
	{
	stopWorkerThreads = 1;
	if (tasksReadyCondition)
		SDL_CondBroadcast(tasksReadyCondition);	//wake up all the sleeping threads to take notice of event.

	//cannot return until all worker threads have exited!!  Have to wait for all of them to spin down!
	while (numActiveThreads)
		{
		SDL_Delay(100);
		}
	//try to grab the lock one last time to make sure that all threads made it out of their lock sections.
	SDL_LockMutex(taskArrayMutex);
	SDL_UnlockMutex(taskArrayMutex);
	   
	}
