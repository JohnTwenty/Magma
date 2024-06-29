#pragma once

class String;

class FileManager
{
public:
	const void* loadFile(const char* path, size_t* lengthOut, bool prependResourceDir = true);
	void unloadFile(const void *);
	bool wasModifiedSince(const char* path, time_t lastTime, time_t* newTimeOut, bool prependResourceDir = true);

	void saveFile(const char * path, const void * buffer, size_t length, bool prependResourceDir = true);

	void setResourceDir(const char *);
	String & getResourceDir();
};


extern FileManager& fileManager;
