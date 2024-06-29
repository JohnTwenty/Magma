#pragma once
#pragma warning(disable: 4464) //relative include path contains '..'

#include "..\Foundation\MemoryManager.h"


class Allocator
{
public:
	void* allocate(size_t size, const char* filename, int line) 
	{		
		return memoryManager.allocate(size, filename, line); 
	}
	void deallocate(void* ptr) 
	{ 
		memoryManager.deallocate(ptr);
	}
};

