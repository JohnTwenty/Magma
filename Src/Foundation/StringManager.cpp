#include "StdAfx.h"
#include "Foundation\StringManager.h"
#include "Foundation\Foundation.h"
#include "Foundation\Allocator.h"
#include "Foundation\Array.h"

#include <stdlib.h>


const size_t defaultPageSize = 128 * 128;	//16KBytes

template <class T> const T& max(const T& a, const T& b)
	{
	return (a < b) ? b : a;
	}


struct StringPage
	{
	StringPage(size_t s)  : size(s), writeOffset(0)
		{ 
		memory = static_cast<char *>(memoryManager.allocate(size, __FILE__, __LINE__));
		}
	StringPage() : size(0), writeOffset(0), memory(0)	{}	//stupid array needs us to have a default ctor.
	void deallocate()
		{
		memoryManager.deallocate(memory);
		memory = 0;
		size = 0;
		writeOffset = 0;
		}
	//try to add the passed string to this Page, incl \0.  Returns char offset of the string on success or -1 on failure.
	int tryAddString(const char * str, size_t length)		//length as returned by strlen()
		{
		length++;					//include terminating zero in string length.
		char * end = memory + size;	//one beyond the end of memory
		char * writePtr = memory + writeOffset;
		char * writeEnd = writePtr + length;//if we were to write string, this address would be just after the terminating \0
		if (writeEnd <= end)
			{
			//ok
			ASSERT(writeEnd <= memory + size);
			memcpy(writePtr, str, length);

			size_t retval = writeOffset;
			writeOffset += length;
			return static_cast<int>(retval);
			}
		else
			return -1;
		}
	char * memory;
	size_t size;		//memory block size
	size_t writeOffset;		//offset at which we can start writing strings
	};


Array<StringPage, Allocator>	stringPages;


StringManager::StringManager()
	{
	stringPages.reserve(4);
	stringPages.pushBack(StringPage(defaultPageSize));
	}


StringManager::~StringManager()
	{

	}

void StringManager::shutDown()	//free all resources at exit
	{
	for (unsigned i = 0; i < stringPages.size(); i++)
		stringPages[i].deallocate();

	stringPages.reset();
	}



unsigned StringManager::addString(const char *str)	//returns index in table
	{
	ASSERT(str);


	size_t strLength = strlen(str);
	
	StringPage &sp = stringPages.back();	//we always have at least one stringPage so this will work
	int offset = sp.tryAddString(str, strLength);
	if (offset >= 0)
		{
		ASSERT(offset == offset & 0x00ffffff);	//need top byte for page index.
		return ((stringPages.size()-1)<<24)|offset;
		}
	else
		{
		//page is full, need to add a new one.
		ASSERT(strLength < defaultPageSize);	//if this isn't the case, its actually OK, but we should make sure the below does the right thing -- it should.
		size_t pageSize = max(strLength, defaultPageSize);
		stringPages.pushBack(StringPage(pageSize));
		StringPage &sp = stringPages.back();
		int offset = sp.tryAddString(str, strLength);
		ASSERT(offset != 1);	//this must have worked.
		ASSERT(offset == offset & 0x00ffffff);	//need top byte for page index.
		return ((stringPages.size() - 1) << 24) | offset;
		}
	}

const char * StringManager::lookupString(unsigned index)
	{
	size_t offset = index & 0x00ffffff;
	unsigned page = index & 0xff000000;

	ASSERT(page < stringPages.size());
	ASSERT(stringPages[page].memory[offset]);
	return stringPages[page].memory + offset;
	}




StringManager gStringManager;
StringManager & stringManager = gStringManager;
