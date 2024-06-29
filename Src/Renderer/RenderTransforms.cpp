#include "StdAfx.h"
#include "RenderTransforms.h"


const float * RenderTransforms::buildRenderMatrix()
	{
	float p = zFar / (zFar - zNear);
	float q = -p*zNear;

	PxMat44 projection(PxVec4((2 * zNear) / nearPlaneWidth, 0, 0, 0), PxVec4(0, (2 * zNear) / nearPlaneHeight, 0, 0), PxVec4(0, 0, p, 1), PxVec4(0, 0, q, 0));

	renderMatrix = projection * PxMat44(cam2Scene.getInverse()) * PxMat44(actor2Scene) * PxMat44(mesh2Actor);
	return renderMatrix.front();
	}