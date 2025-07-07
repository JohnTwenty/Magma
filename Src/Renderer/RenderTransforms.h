#pragma once

#pragma warning(push)
#undef free
#undef realloc
#pragma warning( disable : 4365) //C4365: 'argument' : conversion from 'unsigned int' to 'int'
#pragma warning( disable : 4061) //warning C4061: enumerator 'CompressionMask' in switch of enum
#include "foundation/PxMat44.h"
#include "foundation/PxTransform.h"
using namespace physx;
#pragma warning(pop)


class RenderTransforms
	{
	public:
	RenderTransforms() : nearPlaneWidth(2.0f), nearPlaneHeight(2.0f), zNear(2.0f), zFar(8.0f), cam2Scene(PxIdentity), actor2Scene(PxIdentity), mesh2Actor(PxIdentity) {}
	~RenderTransforms() {}

	void setProjection(float nearPlaneWidth, float nearPlaneHeight, float zNear, float zFar)
		{
		this->nearPlaneWidth = nearPlaneWidth;
		this->nearPlaneHeight = nearPlaneHeight;
		this->zNear = zNear;
		this->zFar = zFar;
		}
	//void setOrtho();	//todo

	void setCam2Scene(const PxTransform & t)		{ cam2Scene = t;  }
	void setActor2Scene(const PxTransform & t)		{ actor2Scene = t; }
	void setMesh2Actor(const PxTransform & t)		{ mesh2Actor = t; }

	const float * buildRenderMatrix();

	private:
	float nearPlaneWidth, nearPlaneHeight, zNear, zFar;
	PxTransform cam2Scene, actor2Scene, mesh2Actor;
	PxMat44 renderMatrix;
	};
