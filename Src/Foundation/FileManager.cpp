#include "App\stdAfx.h"
#include "Foundation\FileManager.h"
#include "Foundation\String.h"

#pragma warning(push)
#pragma warning( disable : 4548) //expression before comma has no effect
#pragma warning( disable : 4668)//x  not defined as a preprocessor macro
#pragma warning( disable : 4365)//signed/unsigned mismatch
#pragma warning( disable : 4987)//nonstandard extension used
#include <SDL.h>
#pragma warning(pop)

#include <sys/stat.h>			//for last time file modified


String								resourceDir;


const void* FileManager::loadFile(const char* path, size_t* lengthOut, bool prependResourceDir)
{
	SDL_RWops* file = SDL_RWFromFile(prependResourceDir ? (resourceDir + path) : path, "rb");	//work in binary
	if (!file)
	{
		foundation.printLine("FileManager::loadFile():file not found:", path);
		*lengthOut = 0;
		return 0;
	}

	//try to early out if already loaded and file hasn't changed
	*lengthOut = file->size(file);

	void* buffer = memoryManager.allocate(*lengthOut);
	SDL_RWread(file, buffer, *lengthOut, 1);
	SDL_RWclose(file);

	return buffer;
}

void FileManager::unloadFile(const void* buffer)
{
	memoryManager.deallocate(const_cast<void *>(buffer));
}

bool FileManager::wasModifiedSince(const char* path, time_t lastTime, time_t* newTimeOut, bool prependResourceDir)
{
	//String fname = resourceDir + sourceFile;
	struct stat result;
	if (stat(prependResourceDir ? (resourceDir + path) : path, &result) == 0)
	{
		*newTimeOut = result.st_mtime;
		return *newTimeOut > lastTime;
	}
	else
		return true;
}

void FileManager::saveFile(const char* path, const void* buffer, size_t length, bool prependResourceDir)
{
	SDL_RWops* file = SDL_RWFromFile(prependResourceDir ? (resourceDir + path) : path, "wb");	//work in binary
	if (!file)
	{
		foundation.printLine("FileManager::saveFile():error writing file:", path);
	}
	else
	{
		SDL_RWwrite(file, buffer, length, 1);
		SDL_RWclose(file);
	}

}

void FileManager::setResourceDir(const char* d)
{
	resourceDir = d;
}

String & FileManager::getResourceDir()
{
return resourceDir;
}

FileManager gFileManager;
FileManager& fileManager = gFileManager;