#include "App\stdAfx.h"
#include "VolumeAtlas.h"
#include "Foundation\Foundation.h"

#include <stdint.h>
#include "texture-atlas/src/texture_atlas.h"

struct Slab
{
	Slab(uint32_t id, uint32_t x, uint32_t y, uint32_t w, uint32_t h) :
		atlas2D_id(id),
		usedTillDepth(0),
		xCoord(x),
		yCoord(y),
		width(w),
		height(h) {}

	uint32_t atlas2D_id;
	uint32_t usedTillDepth;	//max depth of allocation.  So free between this value and the total depth of the VolumeAtlas.

	//This is just cache, we could read it using atlas_get_vtex_xywh_coords()
	uint32_t xCoord;
	uint32_t yCoord;
	uint32_t width;
	uint32_t height;

	bool isFitFor(unsigned w, unsigned h, unsigned tol)
	{
		return (w <= width && h <= height && (w + tol) >= width && (h + tol) >= height);
	}

};

struct AssetLocation	//TODO: we could shrink this but not sure about 16 bit support in HLSL ... its not a basic feature.
{
	AssetLocation(uint32_t x, uint32_t y, uint32_t z, uint32_t w, uint32_t h, uint32_t d) :
		xCoord(x),
		yCoord(y),
		zCoord(z),
		pack0(0),
		width(w),
		height(h),
		depth(d),
		pack1(0) {}
	uint32_t xCoord;
	uint32_t yCoord;
	uint32_t zCoord;
	uint32_t pack0;	//unused, for float4 packing
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t pack1;	//unused, for float4 packing
};



VolumeAtlas::VolumeAtlas(unsigned aw, unsigned ah, unsigned ad, unsigned tol)	//size of the atlas volume
{
	atlas2D = NULL;
	if (aw == ah)	//unfortunately the 2D atlas only supports squares.
	{
		if (atlas_create(&atlas2D, aw, 0) == 0)
			foundation.printLine("VolumeAtlas::VolumeAtlas(): 2D atlas creation failed.");
	}
	else
		foundation.printLine("VolumeAtlas::VolumeAtlas():non square atlas not supported.");

	atlasW = aw;
	atlasH = ah;
	atlasD = ad;
	tolerance = tol;
}

VolumeAtlas::~VolumeAtlas()
{
	if (atlas2D)
	{
		atlas_destroy(atlas2D);
		atlas2D = NULL;
	}
}

Slab* VolumeAtlas::findSlab(unsigned w, unsigned h, unsigned d, unsigned& x, unsigned& y, unsigned& z)
{
	for (Slab* s = slabs.begin(); s < slabs.end(); s++)
	{
		if (s->isFitFor(w, h, tolerance))
		{
			//try to find space in this pre-existing slab.
			if (allocInSlab(s, d, x, y, z))

			return s;
		}
	}
	return NULL;
}


Slab* VolumeAtlas::allocSlab(unsigned w, unsigned h)
{
	//use the atlas2D to find space for a slab:
	uint32_t id = 0;
	if (atlas2D == NULL || atlas_gen_texture(atlas2D, &id) == 0)
	{
		foundation.printLine("VolumeAtlas::findSpace(): 2D atlas creation failed.");	//can only happen on out of memory or we never created a 2D atlas.
		return NULL;
	}

	if (atlas_allocate_vtex_space(atlas2D, id, w, h) == 0)
	{
		foundation.printLine("VolumeAtlas::findSpace(): failed to find space in the 2D atlas.");
		return NULL;
	}

	unsigned short xywh[4];
	if (atlas_get_vtex_xywh_coords(atlas2D, id, 1, xywh) == 0)
	{
		foundation.printLine("VolumeAtlas::findSpace(): this should never happen.");
		return NULL;
	}

	return &slabs.pushBack(Slab(id, xywh[0], xywh[1], w, h));
}

bool VolumeAtlas::allocInSlab(Slab* s, unsigned d, unsigned& x, unsigned& y, unsigned& z)
{
	unsigned depthRemaining = atlasD - s->usedTillDepth;
	if (depthRemaining >= d)
	{
		//update the asset map
		x = s->xCoord;
		y = s->yCoord;
		z = s->usedTillDepth;


		s->usedTillDepth += d;
		ASSERT(s->usedTillDepth <= atlasD);

		assetMap.pushBack(AssetLocation(x, y, z, s->width, s->height, d));
		return true;
	}
	else
		return false;
}


bool VolumeAtlas::findSpace(unsigned w, unsigned h, unsigned d, unsigned& x, unsigned& y, unsigned& z)	//true on success.  Already allocates the space in the map.
{

	if (w > atlasW || h > atlasH || d > atlasD)
	{
		foundation.printLine("VolumeAtlas::findSpace(): cannot allocate space larger than atlas dims.");
		return false;
	}

	Slab * slab = findSlab(w, h, d, x, y, z);
	if (slab)
	{
		return true;
	}

	//if we get here either we did not find a slab or it is full so we need another.
	slab = allocSlab(w, h);

	if (slab == NULL)
	{
		foundation.printLine("VolumeAtlas::findSpace(): cannot allocate Slab.");
		return false;
	}

	allocInSlab(slab, d, x, y, z);	//if we just allocated the slab this must succeed because this is a fresh full depth slab.
	return true;
}

unsigned VolumeAtlas::getAssetMapSizeInFloats()
{
	return assetMap.size() * sizeof(AssetLocation) / 4;
}

void* VolumeAtlas::genAssetMap(size_t& sizeInBytes)
{
	sizeInBytes = assetMap.size() * sizeof(AssetLocation);
	return &(assetMap.front().xCoord);
}
