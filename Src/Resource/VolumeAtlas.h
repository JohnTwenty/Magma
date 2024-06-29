#pragma once

#include "Foundation\Allocator.h"
#include "Foundation\Array.h"


extern "C"
{
	typedef struct Atlas Atlas;
}

struct Slab;
struct AssetLocation;

//class to allocate subvolumes in an atlas volume.  Uses a 2D allocator in the wh direction to slice into full d slabs, which are then further sliced along d.
class VolumeAtlas
{
public:
	VolumeAtlas(unsigned aw, unsigned ah, unsigned ad, unsigned tolerance = 4);	//size of the atlas volume - currently w and h need to be the same.  Tolerance is a heuristic - allocation will use spaces bigger than required by tolerance.
	~VolumeAtlas();



	bool findSpace(unsigned w, unsigned h, unsigned d, unsigned & x, unsigned & y, unsigned & z);	//true on success.  Already allocates the space in the map.
	unsigned getAssetMapSizeInFloats();//get the size of the buffer in floats that genAssetMap() will fill when subsequently called.
	void* genAssetMap(size_t & sizeInBytes);	//returns a buffer (as a void *) that has the xyz_pad and whd_pad dwords of all allocated subtextures, in the order of allocation.  Don't call this if getAssetMapSizeInFloats() returns 0.
	//deallocation is not needed

private: 
	Slab* findSlab(unsigned w, unsigned h, unsigned d, unsigned& x, unsigned& y, unsigned& z);		//find a slab to store this size, taking into account the tolerance.
	Slab* allocSlab(unsigned w, unsigned h);
	bool allocInSlab(Slab *, unsigned d, unsigned & x, unsigned & y, unsigned & z);


	unsigned atlasW, atlasH, atlasD; 
	unsigned tolerance;
	Atlas* atlas2D;
	Array<Slab, Allocator> slabs;
	Array<AssetLocation, Allocator>			assetMap;


};
