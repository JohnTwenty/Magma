#include "StdAfx.h"
#include "Foundation\MemoryManager.h"
#include <stdlib.h>




MemoryManager::MemoryManager(void)
{
}

MemoryManager::~MemoryManager()
{
}


void * MemoryManager::allocate(size_t size, const char* /*filename*/, int /*line*/)
{
	return malloc(size);
}

void MemoryManager::deallocate(void * p)
{
	::free(p);
}





MemoryManager gMemoryManager;
MemoryManager & memoryManager= gMemoryManager;