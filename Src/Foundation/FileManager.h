#pragma once

class String;

// Define the callback function type
typedef void (*FileCallback)(const char* filename);

class FileManager
{
public:
	const void* loadFile(const char* path, size_t* lengthOut, bool prependResourceDir = true);
	void unloadFile(const void *);
	bool wasModifiedSince(const char* path, time_t lastTime, time_t* newTimeOut, bool prependResourceDir = true);

	void saveFile(const char * path, const void * buffer, size_t length, bool prependResourceDir = true);

	void findMediaDirectory();
	void setResourceDir(const char *);
	String & getResourceDir();

	// New method to iterate through files matching a pattern
	bool iterateFiles(const char* pattern, FileCallback callback, bool prependResourceDir = true);
};


extern FileManager& fileManager;
