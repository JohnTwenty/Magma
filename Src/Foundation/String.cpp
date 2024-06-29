#include "stdafx.h"
#include <string.h>
#include "String.h"

#include "Foundation\Foundation.h"
#include "MemoryManager.h"




void String::operator=(const char * c)	//assign cstr
{
	
	if (c)
		data.assign(c, c+strlen(c)+1);	//also copy terminating 0
	else
		data.clear();
}

String String::operator+(const char * c) const	//append cstr
{
	String result(*this);
	if (c)
		{
		if (result.data.size() > 0)
			result.data.popBack();	//get rid of trailing 0 string terminator.
		result.data.append(c, strlen(c)+1);
		}
	return result;
}


//check if the string ends with the given length-sized pattern.
//useful for checking the extensions of file names.
bool String::endsWith(const char * pattern, unsigned length) const	//length does not include final 0.
{
	ASSERT(length);
	if (data.size() > length)
	{
		for (unsigned i = 0; i < length; i++)
			if (data.end()[i-length-1] != pattern[i])
				return false;

		return true;
	}
	else
		return false;
}

