#pragma once

#include "Foundation\Types.h"


class VoxVol
	{
	static const unsigned maxChunks = 12;

	struct Chunk
	{
		Chunk() : width(0), height(0), depth(0), voxels(NULL) {}
		~Chunk();

	MxU32 width, height, depth;
	MxU8 * voxels;

	} chunks[maxChunks];

	unsigned numChunks; 
	MxU32 maxChunkWidth, maxChunkHeight, maxChunkDepth;
	MxU8* lockedBuffer;

	public:
	VoxVol();
	~VoxVol();


	void load(const char * fullPath);

	//returns a total volume size that would be needed to store ALL of the chunks in a single volume.
	MxU32 getWidth();
	MxU32 getHeight();
	MxU32 getDepth();


	MxU8 * lockForRead();	//only supported for single chunk volumes! Otherwise use writeToBuffer()
	void unlock();
	void writeToBuffer(void * p, unsigned rowPitch, unsigned depthPitch);//write all chunks as a single volume to a buffer.

	//per chunk access:
	unsigned getNumChunks();
	MxU8* lockChunkForRead(unsigned i);	//only supported for single chunk volumes! Otherwise use writeToBuffer()
	void unlockChunk() {}
	MxU32 getChunkWidth(unsigned i);
	MxU32 getChunkHeight(unsigned i);
	MxU32 getChunkDepth(unsigned i);


	private:
	void allocate(MxU32 width, MxU32 height, MxU32 depth);
	void loadMagicka(const char* fullPath);

	};

inline unsigned VoxVol::getNumChunks()
{
	return numChunks;
}

inline MxU32 VoxVol::getChunkWidth(unsigned i)
{
	return chunks[i].width;
}

inline MxU32 VoxVol::getChunkHeight(unsigned i)
{
	return chunks[i].height;
}

inline MxU32 VoxVol::getChunkDepth(unsigned i)
{
	return chunks[i].depth;
}


inline MxU32 VoxVol::getWidth()
{
return maxChunkWidth * numChunks;
}

inline MxU32 VoxVol::getHeight()
{
return maxChunkHeight;
}

inline MxU32 VoxVol::getDepth()
{
return maxChunkDepth;
}
