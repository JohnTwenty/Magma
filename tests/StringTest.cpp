#include <cassert>
#include "../Src/Foundation/String.h"

void test_String_endsWith()
{
    String s("file.hlsl");
    assert(s.endsWith("hlsl", 4));
    assert(!s.endsWith("txt", 3));
}
