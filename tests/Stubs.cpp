#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include "../Src/Foundation/Foundation.h"
#include "../Src/Foundation/MemoryManager.h"

MemoryManager gMemoryManager;
MemoryManager &memoryManager = gMemoryManager;

MemoryManager::MemoryManager(void) {}
MemoryManager::~MemoryManager() {}
void * MemoryManager::allocate(size_t size, const char*, int) { return malloc(size); }
void MemoryManager::deallocate(void *p) { free(p); }

Foundation gFoundation;
Foundation &foundation = gFoundation;

Foundation::Foundation(void) {}
Foundation::~Foundation(void) {}
void Foundation::registerCommands() {}
void Foundation::print(const char *string) { if (string) fputs(string, stdout); }
void Foundation::printLine(const char *s1, const char *s2) { if (s1) fputs(s1, stdout); if (s2) fputs(s2, stdout); fputc('\n', stdout); }
void Foundation::printf(const char *format, ...) { va_list args; va_start(args, format); vprintf(format, args); va_end(args); }
void Foundation::fatal(const char *string) { if (string) fputs(string, stderr); abort(); }
void Foundation::assertViolation(const char* exp, const char* file, int line, bool& ignore) {
    fprintf(stderr, "Assertion failed: %s at %s:%d\n", exp, file, line);
    ignore = false;
}
