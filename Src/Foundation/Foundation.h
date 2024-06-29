#pragma once
class Foundation
{
public:
	Foundation(void);
	~Foundation(void);
	void registerCommands();


	//the below is actually our error stream class which could be broken out....
	void print(const char * string);
	void printLine(const char * string=nullptr, const char * string2=nullptr);	//can add more optional params if needed.  function will print the string sequence concatenated, then a newline.
	void fatal(const char * string); 
	void assertViolation(const char* exp, const char* file, int line, bool& ignore);

};

extern Foundation & foundation;

#if !defined(_DEBUG)
#	define ASSERT(exp) ((void)0)
#else
#	define ASSERT(exp) { static bool ignore = false; ((void)(!!(exp) || (!ignore && (foundation.assertViolation(#exp, __FILE__, __LINE__, ignore), false))));  }
#endif
