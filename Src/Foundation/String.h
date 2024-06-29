#pragma once

#include "Types.h"
#include "Allocator.h"
#include "InlineAllocator.h"
#include "Array.h"



class String
{
public:

	String();
	~String();
	String( const String &text );
	String(const char*);
	void operator=(const char *);	//assign cstr
	String operator+(const char *) const;	//concatenate strings
	operator const char * (void) const; //cast to cstr
	bool endsWith(const char *, unsigned length) const;

private:
	static const int INLINE_SIZE = 32 - sizeof(Array<char, InlineAllocator<1, Allocator>>); //make total size of String be 32.
	Array<char, InlineAllocator<INLINE_SIZE, Allocator>> data;
};

INLINE String::String() 
{
}

INLINE String::String( const String &other ) : data(other.data)
{
}

INLINE String::String(const char* text)
{
*this = text;
}

INLINE String::~String()
{
}

INLINE String::operator const char * (void) const //cast to cstr
{
	return data.begin();
}