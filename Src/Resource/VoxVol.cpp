#include "StdAfx.h"
#include "VoxVol.h"

#include <math.h>
#include "Foundation\Foundation.h"
#include "Foundation\MemoryManager.h"
#include "Foundation\FileManager.h"

#pragma warning(push)
#undef free
#undef realloc
#pragma warning( disable : 4365) //C4365: 'argument' : conversion from 'unsigned int' to 'int'
#pragma warning( disable : 4061) //warning C4061: enumerator 'CompressionMask' in switch of enum
#include "foundation/Px.h"
#include "foundation/PxVec2.h"
#include "foundation/PxMat44.h"
#include "foundation/PxTransform.h"
using namespace physx;
#pragma warning(pop)

VoxVol::VoxVol()
{
	numChunks = 0;
	maxChunkWidth = 0;
	maxChunkHeight = 0;
	maxChunkDepth = 0;
	lockedBuffer = nullptr;
}

VoxVol::~VoxVol()
{
}

VoxVol::Chunk::~Chunk()
{ 	
	if (voxels)
		memoryManager.deallocate(voxels);
}

float hash12(PxVec2 p)
{
	p *= 0.1031f;
	PxVec3 p3(p.x - floor(p.x), p.y - floor(p.y), 0.0f);
	p3.z = p3.x;
	PxVec3 q(p3.y + 33.33, p3.z + 33.33, p3.x + 33.33);
	float d = q.dot(p3);
    p3 += PxVec3(d);
	float k = (p3.x + p3.y) * p3.z;
    return k - floor(k);
}


float noise(PxVec2 x ) 
{
    PxVec2 p(floor(x.x), floor(x.y));
    PxVec2 f(x.x - p.x, x.y - p.y);
	f *= 2.0f;
	PxVec2 g(f.x * f.x * (3.0f - f.x), f.y * f.y * (3.0f - f.y));
	PxVec2 uv(p.x + g.x + 118.4f, p.y + g.y + 118.4f);
	uv *= (1.0f / 256.0f); 
	return hash12(uv);
}
MxU8 terrain(MxU32 x, MxU32 y, MxU32 z)	//y up
{
	PxVec2 p(x, z);
	 p *= 0.02f;

    float f  = 0.5; 
    f += 0.10f*noise( p );
	float h = 50.0f * f - 30.0f;
	float height = (h > -25.0) ? h : -25.0f;	//terrain height (mapTerrain)

	if (y <= height + 5.0f)
		return 1;
	else
		return 0;
}


MxU8 fountain(MxU32 x, MxU32 y, MxU32 z)
{
	/*
		if (x == 0 && y == 0 && z == 0)
				return 4;

		if (x == 2 && y == 0 && z == 0)
				return 8;

		if (x == 0 && y == 0 && z == 1)
				return 1;

		if (x == 0 && y == 0 && z == 2)
				return 1;
		*/
	if (y == 0 && x > 1 && x < 20 && z > 1 && z < 20)	//floor
		return 4;

	if (y < 8 && x == 4 && z == 4)
		return 4;

	if (y < 8 && x == 16 && z == 16)
		return 4;

	if (y < 8 && x == 4 && z == 16)
		return 4;

	if (y < 8 && x == 16 && z == 4)
		return 4;

	if (y == 8 && x >= 4 && x <= 16 && z == 4)
		return 4;

	if (y == 8 && x >= 4 && x <= 16 && z == 16)
		return 4;

	if (y == 8 && z >= 4 && z <= 16 && x == 4)
		return 4;

	if (y == 8 && z >= 4 && z <= 16 && x == 16)
		return 4;

	//well in center
	if (x == 9 && y < 3 && z == 10)
		return 4;
	if (x == 11 && y < 3 && z == 10)
		return 4;
	if (x == 10 && y < 3 && z == 9)
		return 4;
	if (x == 10 && y < 3 && z == 11)
		return 4;
	if (x == 9 && y < 3 && z == 9)
		return 4;
	if (x == 11 && y < 3 && z == 11)
		return 4;


	return 0;
}


MxU8 cave(MxU32 x, MxU32 y, MxU32 z)
{
	if (x == 8 && y == 6 && z == 2)
		return 10;

	if (y == 0 && x > 1 && x < 20 && z > 1 && z < 20)	//floor
		return 4;

	if (y > 0 && y < 5 && x > 6 && x < 15 && z > 6 && z < 15)	//floor
	{
		//tunnels
		if (x == 10 || z == 10)
			if (y < 3)
				return 0;
		return 4;
	}


	return 0;
}


MxU8 cornellBox(MxU32 x, MxU32 y, MxU32 z)
{
	int width = 12;
	int height = 12;
	int depth = 20;

	if (y == height && x > 5 && x < 8 && z > 14 && z < 17)	//light
		return 10;

	if ((y == 0 || y == height) && x > 1 && x < width && z >= 1 && z < depth)	//floor and ceiling
		return 4;


	if (y > 0 && y < height && x > 1 && x < width && (z == 0 || z == depth))		//back and front walls
		return 4;

	if (y > 0 && y < height && z >= 1 && z < depth && x == 1)		//red wall
		return 2;

	if (y > 0 && y < height && z >= 1 && z < depth && x == width)		//green wall
		return 8;

	if (y > 0 && y < 6 && x > 3 && x < 6 && z > 15 && z < 18)	//tall block
		return 4;

	if (y > 0 && y < 4 && x > 7 && x < 10 && z > 11 && z < 14)	//short block
		return 4;

	return 0;
}

//temp implicit map
MxU8 sillyPlanet(MxU32 x, MxU32 y, MxU32 z) //returns 1 near origin
{
	//	 mat = 1; // ground
	//	 mat = 7; // treetrunk
	//	 mat = 8; // leaves
	//	 mat = 10; // clouds    
	//	 mat = 2;	//dirt
	//   else     // stone (4)

	//    return (y < (4.0 * sin(x/10.0) + (3.0 * cos(z/10.0)))  ) ? 1 : 0;  

	PxVec3 center(16.0f, 6.0f, 16.0f);
	PxVec3 q(x, y, z);
	PxReal radius = 13.0f;

	PxReal radius2 = 15.0f;

	PxVec3 center2(16.0f, 0.0f, 16.0f);

	if (y == 0)		//clip away ground floor cause we have clipping issues with blocks that touch the edge.
		return 0;

	if (x == 16 && z == 16 && y == 20)	//y = 19 is on ground, 25 in air
		return 13;

	//PxReal dist2 = (center - q).magnitudeSquared();

	//PxReal dist22 = (center2 - q).magnitudeSquared();

	if ((center - q).magnitudeSquared() < radius * radius)
	{
		//inside sphere

		//is it the topmost cell of sphere?

		if ((center - q - PxVec3(0, 1, 0)).magnitudeSquared() < radius * radius)
		{
			//tunnel
			if (z == 15 && (y | 1) == 7 && x < 20)
				return 0;

			if (y < 7)
				return 4;	//stone
			else
				return 2;	//dirt
		}
		else
			return 1;	//grass			
	}
	else
	{
		if (x == 16 && z == 20 && y < 22)
			return 7;	//trunk

		if ((PxVec3(16, 22, 20) - q).magnitudeSquared() < 5)
			return 8; //leaves

		if (y == 26 && (((25 - x) * (25 - x) + (7 - z) * (7 - z)) < 25))
			return 10; //clouds
	}

	return 0;

}

void VoxVol::allocate(MxU32 w, MxU32 h, MxU32 d)
{
	//sanity check
	if (w > 1024)
		return;
	if (h > 1024)
		return;
	if (d > 1024)
		return;
	if (numChunks >= maxChunks)
		return;

	size_t size = w * h * d;
	chunks[numChunks].width = w;
	chunks[numChunks].height = h;
	chunks[numChunks].depth = d;
	chunks[numChunks].voxels = static_cast<MxU8*>(memoryManager.allocate(size));
	memset(chunks[numChunks].voxels, 0, size);
	numChunks++;

	if (w > maxChunkWidth)
		maxChunkWidth = w;
	if (h > maxChunkHeight)
		maxChunkHeight = h;
	if (d > maxChunkDepth)
		maxChunkDepth = d;
}


void VoxVol::load(const char* fullPath)
{
	if (strstr(fullPath, "magica"))
	{


		loadMagicka(fullPath);

		//HACK: hack in a light:
		//voxels[2 * width * height + 22 * width + 2] = 10;

	}
	else
	{
		allocate(32, 32, 32);

		//procedural worlds
		MxU8(*mapgen)(MxU32, MxU32, MxU32) = strstr(fullPath, "f.cornell") ? cornellBox : (strstr(fullPath, "f.planet") ? sillyPlanet : (strstr(fullPath, "f.cave") ? cave : (strstr(fullPath, "f.terrain") ? terrain : fountain)));	//I am so sorry

		//offsets
		unsigned x = 0, y = 0, z = 0;

		if (mapgen == terrain)
		{
			const char* p = fullPath + 9;//at least this offset until end of f.terrain.
			while (*p != 0 && *p != '(')
				p++;

			sscanf(p, "(%d %d %d)", &x, &y, &z);
		}




		for (MxU32 d = 0; d < chunks[0].depth; d++)
			for (MxU32 h = 0; h < chunks[0].height; h++)
				for (MxU32 w = 0; w < chunks[0].width; w++)
					chunks[0].voxels[d * chunks[0].width * chunks[0].height + h * chunks[0].width + w] = mapgen(w + x, h + y, d + z);

	
	}
}

MxU8* VoxVol::lockChunkForRead(unsigned i)
{
	ASSERT(i < numChunks);
	return chunks[i].voxels;
}


MxU8* VoxVol::lockForRead()	//only supported for single chunk volumes! Otherwise use writeToBuffer()
{
	if (numChunks == 1)
		return chunks[0].voxels;
	else
	{
		return NULL;
		/*
		//TODO: THis is stupid and wastes memory!
		//Why do we round up to maxChunkDIM along all dimensions? We should be able to pack them at least along one dimension and still fit them into a
		//rectangular subresource block.

		//need to create continuous block from all the frames, at least temporarily
		//it would be more natural to stack these along z axis so that memory is continuous for each frame
		//but unfortunately directx tiled subresource tiles are it seems 64-wide at minimum, so we need to stack along x axis to maximize usage of this coordinate.

		printf("VoxVol Chunk dimensions: %d %d %d\n", maxChunkWidth, maxChunkHeight, maxChunkDepth);
		printf("VoxVol Total dimensions: %d %d %d\n", maxChunkWidth * numChunks, maxChunkHeight, maxChunkDepth);
		size_t size = (maxChunkWidth * numChunks) * maxChunkHeight * maxChunkDepth;
		lockedBuffer = static_cast<MxU8*>(memoryManager.allocate(size));
		memset(lockedBuffer, 0, size);

		for (unsigned i = 0; i < numChunks; i++)
			for (MxU32 d = 0; d < chunks[i].depth; d++)
				for (MxU32 h = 0; h < chunks[i].height; h++)
					for (MxU32 w = 0; w < chunks[i].width; w++)
						//stack chunks along z axis: 
						//lockedBuffer[(maxChunkDepth * i + d) * maxChunkWidth * maxChunkHeight + h * maxChunkWidth + w] = chunks[i].voxels[d * chunks[i].width * chunks[i].height + h * chunks[i].width + w];

						//stack along x axis: 
						//return (z * (w * n) * yMax) + (y * w * n) + w * i + x;

						lockedBuffer[d * maxChunkWidth * numChunks * maxChunkHeight + (h * maxChunkWidth * numChunks) + maxChunkWidth * i + w] = 
						//(w*h*d) == 0 ? 0 : w % 10; 
						chunks[i].voxels[d * chunks[i].width * chunks[i].height + h * chunks[i].width + w];




		return lockedBuffer;
		*/
	}
}

void VoxVol::unlock()
{
	if (lockedBuffer)
		memoryManager.deallocate(lockedBuffer);

	lockedBuffer = nullptr;
}

void VoxVol::writeToBuffer(void* p, unsigned rowPitch, unsigned depthPitch)
{
	MxU8* buffer = static_cast<MxU8 *>(p);
	
	for (MxU32 d = 0; d < chunks[0].depth; d++)
	{
		for (MxU32 h = 0; h < chunks[0].height; h++)
		{
			for (MxU32 w = 0; w < chunks[0].width; w++)
				buffer[w] = chunks[0].voxels[d * chunks[0].width * chunks[0].height + h * chunks[0].width + w];
			buffer += rowPitch;
		}
		buffer += depthPitch;
	}

}

bool notText(const char*& b, const char* t)
{
	if (*b++ != *t++) return true;
	if (*b++ != *t++) return true;
	if (*b++ != *t++) return true;
	if (*b++ != *t++) return true;

	return false;
}

void readDictionary(const char* & bytes, size_t & bytesRemaining)
{
	unsigned num_pairs = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
	bytesRemaining -= 2 * 4;

	for (unsigned i = 0; i < num_pairs; i++)
	{
		unsigned key_size = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
		//read key_size bytes
		bytes += key_size;
		unsigned value_size = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
		//read value_size bytes
		bytes += value_size;
		bytesRemaining -= 2 * 4;
		bytesRemaining -= key_size;
		bytesRemaining -= value_size;
	}

}

void VoxVol::loadMagicka(const char* fullPath)
{
	//based in part on: https://github.com/jpaver/opengametools/blob/master/src/ogt_vox.h
	//and https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
	// MAKE_VOX_CHUNK_ID: used to construct a literal to describe a chunk in a .vox file.
#define MAKE_VOX_CHUNK_ID(c0,c1,c2,c3)     ( (c0<<0) | (c1<<8) | (c2<<16) | (c3<<24) )

	static const unsigned CHUNK_ID_VOX_ = MAKE_VOX_CHUNK_ID('V', 'O', 'X', ' ');
	static const unsigned CHUNK_ID_MAIN = MAKE_VOX_CHUNK_ID('M', 'A', 'I', 'N');
	static const unsigned CHUNK_ID_SIZE = MAKE_VOX_CHUNK_ID('S', 'I', 'Z', 'E');
	static const unsigned CHUNK_ID_XYZI = MAKE_VOX_CHUNK_ID('X', 'Y', 'Z', 'I');
	static const unsigned CHUNK_ID_RGBA = MAKE_VOX_CHUNK_ID('R', 'G', 'B', 'A');
	static const unsigned CHUNK_ID_nTRN = MAKE_VOX_CHUNK_ID('n', 'T', 'R', 'N');
	static const unsigned CHUNK_ID_nGRP = MAKE_VOX_CHUNK_ID('n', 'G', 'R', 'P');
	static const unsigned CHUNK_ID_nSHP = MAKE_VOX_CHUNK_ID('n', 'S', 'H', 'P');
	static const unsigned CHUNK_ID_IMAP = MAKE_VOX_CHUNK_ID('I', 'M', 'A', 'P');
	static const unsigned CHUNK_ID_LAYR = MAKE_VOX_CHUNK_ID('L', 'A', 'Y', 'R');
	static const unsigned CHUNK_ID_MATL = MAKE_VOX_CHUNK_ID('M', 'A', 'T', 'L');
	static const unsigned CHUNK_ID_MATT = MAKE_VOX_CHUNK_ID('M', 'A', 'T', 'T');
	static const unsigned CHUNK_ID_rOBJ = MAKE_VOX_CHUNK_ID('r', 'O', 'B', 'J');
	static const unsigned CHUNK_ID_rCAM = MAKE_VOX_CHUNK_ID('r', 'C', 'A', 'M');

    struct VoxPalette
    {
        unsigned color[256];      // rgba palette of colors. use the voxel indices to lookup color from the palette.
    };


	size_t fileSize;
	const void* memory = fileManager.loadFile(fullPath, &fileSize, false);

	if (!memory)
	{
		foundation.printLine("VoxVol::loadMagicka():file not found.");
		return;
	}

	const char* bytes = reinterpret_cast<const char*>(memory);

	if (notText(bytes, "VOX ")) return;
	unsigned version = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
	if (version != 150 && version != 200) return;	//150 is official version, found file with 106

	size_t bytesRemaining = fileSize;

	unsigned volIndex = 0;

	while (bytesRemaining >= 3 * 4 && bytesRemaining <= fileSize)	//size of a chunk header; beware of rollover of unsigned
	{
		unsigned chunk_id = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
		unsigned numBytesContent_main = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
		unsigned numBytesChildren_main = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
		bytesRemaining -= 3 * 4;


		switch (chunk_id)
		{
		case CHUNK_ID_MAIN:
			break;
		case CHUNK_ID_SIZE:
		{
			unsigned w = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			unsigned h = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			unsigned d = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;	//vertical direction
			bytesRemaining -= 3 * 4;

			allocate(w, h, d);
		}
			break;
		case CHUNK_ID_XYZI:
		{
			//only load one frame
			
			unsigned nVoxels = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			bytesRemaining -= 4;

			for (unsigned i = 0; i < nVoxels; i++)
			{
				unsigned char x = *bytes++;
				unsigned char y = *bytes++;
				unsigned char z = *bytes++;
				unsigned char c = *bytes++;

				(void)c;	//unused var -- 8 bit color index.
				if (numChunks)
					chunks[numChunks-1].voxels[z * chunks[numChunks-1].width * chunks[numChunks-1].height + y * chunks[numChunks-1].width + x] = 4;	//brown-ish minecraft tile index, see utils.hlsl/getMaterialColor
			}
			bytesRemaining -= nVoxels * 4;
			volIndex++;

		}
		break;
		case CHUNK_ID_nTRN:
		{
			unsigned node_id = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			readDictionary(bytes, bytesRemaining);

			unsigned child_node_id = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			unsigned reserved_id = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			unsigned layer_id = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			unsigned num_frames = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;

			bytesRemaining -= 4 * 4;

			for (unsigned i = 0; i < num_frames; i++)
			{
				readDictionary(bytes, bytesRemaining);
			}

		}
			break;
		case CHUNK_ID_nGRP:
		{
			unsigned node_id = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			bytesRemaining -= 1 * 4;
			readDictionary(bytes, bytesRemaining);
			unsigned num_child_nodes = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			bytesRemaining -= 1 * 4;
			for (unsigned i = 0; i < num_child_nodes; i++)
			{
				unsigned childId = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
				bytesRemaining -= 1 * 4;
			}
		}

			break;
		case CHUNK_ID_nSHP:
		{
			unsigned node_id = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			bytesRemaining -= 1 * 4;
			readDictionary(bytes, bytesRemaining);
			unsigned num_models = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			bytesRemaining -= 1 * 4;
			for (unsigned i = 0; i < num_models; i++)
			{
				unsigned model_index = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
				bytesRemaining -= 1 * 4;
				readDictionary(bytes, bytesRemaining);
			}
		}

			break;
		case CHUNK_ID_LAYR:
		{
			unsigned layer_id = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			bytesRemaining -= 1 * 4;
			readDictionary(bytes, bytesRemaining);
			unsigned reserved_id = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			bytesRemaining -= 1 * 4;
		}
			break;
		case CHUNK_ID_RGBA:
		{
			bytes += sizeof(VoxPalette);
			bytesRemaining -= sizeof(VoxPalette);

		}
			break;
		case CHUNK_ID_MATL:
		{
			unsigned material_id = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			bytesRemaining -= 1 * 4;
			readDictionary(bytes, bytesRemaining);

		}
			break;
		case CHUNK_ID_rCAM:
		{
			unsigned camera_id = *(reinterpret_cast<const unsigned*>(bytes)); bytes += 4;
			bytesRemaining -= 1 * 4;
			readDictionary(bytes, bytesRemaining);


		}
			break;
		case CHUNK_ID_rOBJ:
		default:
		{
			bytes += numBytesContent_main;
			bytesRemaining -= numBytesContent_main;
		}
			break;
		}

	}

	//todo, parse palette and materials

	fileManager.unloadFile(memory);



}

