// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#pragma warning( disable : 4514)//unreferenced inline function has been removed.  
#pragma warning( disable : 4820)// padding added after data member
#pragma warning( disable : 5045) //Spectre nonsense


#include "App\targetver.h"

//MS crt memory tracking	-- TODO: replace this with a better custom tool!
#define _CRTDBG_MAP_ALLOC		 
#pragma warning(push)
#pragma warning( disable : 4548) //expression before comma has no effect
#include <stdlib.h>
#include <crtdbg.h>
#pragma warning(pop)



#include <assert.h>
#include <string.h>

#pragma warning(push)
#pragma warning( disable : 4710)//function not inlined
#include <stdio.h>
#pragma warning(pop)
#include <tchar.h>



// TODO: reference additional headers your program requires here
