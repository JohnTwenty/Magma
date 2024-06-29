#pragma once


class MemoryManager
{
public:
	MemoryManager(void);
	~MemoryManager();

	void * allocate(size_t size, const char* filename = nullptr, int line = 0);
	void deallocate(void *);


};


extern MemoryManager & memoryManager;