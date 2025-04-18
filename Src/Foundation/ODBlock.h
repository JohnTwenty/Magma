#ifndef __ODBLOCK_H__
#define __ODBLOCK_H__

/*
ObjectDescription Scripts v 1
------------------------------
ODScript = Block
Statement = Block | Terminal
Block = indent '{' {Statement} '}'  
Terminal = ident ';'
ident = anything without braces or semicolons | '"' anything '"'

ObjectDescription Scripts v 2
------------------------------
ODScript = Statement
Statement = Block | Terminal
Block = indent [  '{ or (' {Statement} '} or )'  |   Terminal   ]
Terminal = ident [';'| ',']
ident = identWithoutWhitespace | '"' anything '"'
So, semicolons are now optional, commas can be used instead of semicolons, blocks can now have just an ident and nothing after,  and you can have empty braces.  Parentheses can be used instead of braces.
These changes are to support function style syntax:   f(a, b)
Less verbose datasets: pos { 1 2 3 } 
And value pair style:  gravity 9.8


Comments:
	# = line comment
	/ *  * / = multiline comment.  The / character cannot be used in identifiers.

idents may be enclosed in quotes, and should be unique to facilitate searching.

In a typical application, program would look for known Blocks, and read out its user set terminal(s).
Particular users define semantics:

SHIPFILE 
	{
	Shipname
		{
		Client
			{
			ShipModel
				{
				MeshFile 
					{
					Filename;
					lodlevels;
					}
				Texturefile 
					{
					Filename;
					}
				}
			CockpitModel
				{
				...
				}
			}
		Server
			{
			...
			}
		}
	}
*/
#include <stdio.h>
#include "Allocator.h"
#include "Array.h"
#include "Types.h"
class ODBlock; 
typedef Array<ODBlock *, Allocator> ODBlockList;


class ODBlock
/*-------------------------\
| Block = indent '{' {Statement} '}'  
| Terminals are simply empty blocks
|
|
\-------------------------*/
	{
	static const unsigned OD_MAXID = 128;	//max identifier length

	class ODSyntaxError 
		{
		public:
		enum Error { ODSE_UNEX_QUOTE, ODSE_UNEX_OBRACE, ODSE_UNEX_CBRACE, ODSE_UNEX_LITERAL,ODSE_UNEX_EOF,ODSE_ENC_UNKNOWN };
		private:
		Error err;
		public:
		ODSyntaxError(Error e) {err = e;};
		const char * asString();
		};
	enum State {WAIT_IDENT,IDENT,WAIT_BLOCK,BLOCK};
	char identifier[OD_MAXID];
	unsigned identSize;							//size of above array.
	bool bTerminal;
	ODBlockList subBlocks;	
	ODBlockList::Iterator termiter;				//iterator for reading terminals

	const char* parse(const char* buffer, unsigned size);
	public:
	ODBlock();									//create a new one
	~ODBlock();
	bool loadScript(const char* buffer, unsigned size);
	bool saveScript(FILE* writeP, bool bQuote);//saves this block to scipt file.  set bQuote == true if you want to machine parse output.

	//reading:
	const char * ident();
	inline unsigned numSubBlocks() {return subBlocks.size(); }//returns number of sub blocks
	bool isTerminal();							//resets to first statement returns false if its a terminal == no contained Blocks
	
	//writing:	
	void ident(const char *);						//identifier of the block
	void addStatement(ODBlock &);

	//queries:  return true in success
	ODBlock * getBlock(const char * identifier,bool bRecursiveSearch=false);	//returns block with given identifier, or NULL.

	//iterating:
	void reset();								//prepares to get first terminal or sub block of current block
	bool moreSubBlocks();						//returns true if more sub blocks (including terminals) are available
	ODBlock * nextSubBlock();					//returns a pointer to the next sub block.  
	bool moreTerminals();						//returns true if more terminals are available
	const char * nextTerminal();				//returns a pointer to the next immediate terminal child of current block's identifier string.  
	//casting alternatives:						only call these if moreTerminals() was true!!
	int nextTerminalAsInt();
	float nextTerminalAsFloat();
	bool nextTerminalAsBool();

	// hig level macro functs, return true on success: (call for obj containing:)
	bool getBlockInt(const char * ident, int* p = 0, unsigned count = 1);	//reads blocks of form:		ident{ 123;}
	bool getBlockU32(const char * ident, MxU32* p = 0, unsigned count = 1);		//reads blocks of form:		ident{ 123;}

	bool getBlockString(const char * ident, const char **);		//of form:				ident{abcdef;}
	bool getBlockStrings(const char * ident, const char **, unsigned  count);		//of form:				ident{abcdef; abcdef; ...}

	bool getBlockFloat(const char * ident, float * p = 0);		//of form:				ident{123.456;}
	bool getBlockFloats(const char * ident, float *, unsigned  count);//form:		ident{12.3; 12.3; 12.3; ... };

	bool addBlockFloats(const char * ident, float *, unsigned  count);
	bool addBlockInts(const char * ident, int *, unsigned  count);

	//errors
	static const char * lastError;
	};


#endif //__ODBLOCK_H__

