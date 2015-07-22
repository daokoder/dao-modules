/*
// Dao Graphics Engine
// http://www.daovm.net
//
// Copyright (c) 2012-2014, Limin Fu
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
// SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef __DAO_PATH_H__
#define __DAO_PATH_H__

#include "dao_common.h"

#define DAOX_MAX_DASH    8
#define DAOX_PATH_UNIT   1024.0
#define DAOX_RESOLUTION  1024.0

typedef struct DaoxPathSegment    DaoxPathSegment;
typedef struct DaoxPathComponent  DaoxPathComponent;
typedef struct DaoxPath           DaoxPath;

typedef struct DaoxPathStyle      DaoxPathStyle;
typedef struct DaoxPathMesh       DaoxPathMesh;
typedef struct DaoxPathCache      DaoxPathCache;


enum DaoxPathCommandModes
{
	DAOX_PATH_CMD_ABS ,
	DAOX_PATH_CMD_REL
};
enum DaoxPathJunctions
{
	DAOX_JUNCTION_NONE ,
	DAOX_JUNCTION_FLAT ,
	DAOX_JUNCTION_SHARP ,
	DAOX_JUNCTION_ROUND
};
enum DaoxLineCaps
{
	DAOX_LINECAP_NONE ,
	DAOX_LINECAP_FLAT ,
	DAOX_LINECAP_SHARP ,
	DAOX_LINECAP_ROUND
};


typedef struct DaoxVectorD2    DaoxVectorD2;    /* 2D double vector type; */
typedef struct DaoxVectorD3    DaoxVectorD3;    /* 3D double vector type; */
typedef struct DaoxVectorD4    DaoxVectorD4;    /* 4D double vector type; */
typedef union  DaoxMatrixD3X3  DaoxMatrixD3X3;  /* 3x3 double matrix type; */
typedef union  DaoxMatrixD4X4  DaoxMatrixD4X4;  /* 4x4 double matrix type; */


struct DaoxVectorD2
{
	double  x, y;
};


struct DaoxVectorD3
{
	double  x, y, z;
};


struct DaoxVectorD4
{
	double  x, y, z, w;
};

DaoxVectorD4 DaoxVectorD4_XYZW( double x, double y, double z, double w );


union DaoxMatrixD3X3
{
	double  M[3][3];
	struct  {  DaoxVectorD3  V[3];  } V;
	struct  {
		double  A11, A12, A13;
		double  A21, A22, A23;
		double  A31, A32, A33;
	} A;
};

DaoxMatrixD3X3 DaoxMatrixD3X3_InitRows( DaoxVectorD3 V1, DaoxVectorD3 V2, DaoxVectorD3 V3 );
DaoxMatrixD3X3 DaoxMatrixD3X3_InitColumns( DaoxVectorD3 V1, DaoxVectorD3 V2, DaoxVectorD3 V3 );
double DaoxMatrixD3X3_Determinant( DaoxMatrixD3X3 *self );


union DaoxMatrixD4X4
{
	double  M[4][4];
	struct  {  DaoxVectorD4  V[4];  } V;
	struct  {
		double  A11, A12, A13, A14;
		double  A21, A22, A23, A24;
		double  A31, A32, A33, A34;
		double  A41, A42, A43, A44;
	} A;
};

DaoxMatrixD4X4 DaoxMatrixD4X4_InitRows( DaoxVectorD4 V1, DaoxVectorD4 V2, DaoxVectorD4 V3, DaoxVectorD4 V4 );
DaoxMatrixD4X4 DaoxMatrixD4X4_InitColumns( DaoxVectorD4 V1, DaoxVectorD4 V2, DaoxVectorD4 V3, DaoxVectorD4 V4 );
DaoxMatrixD4X4 DaoxMatrixD4X4_MulMatrix( DaoxMatrixD4X4 *self, DaoxMatrixD4X4 *other );
DaoxVectorD4 DaoxMatrixD4X4_MulVector( DaoxMatrixD4X4 *self, DaoxVectorD4 *vector );



struct DaoxPathSegment
{
	char  bezier;      /* 0: open; 1: linear; 2: quadratic; 3: cubic; */
	char  convexness;  /* 0: flat; 1: locally convex; -1: locally concave; */
	char  refined;
	char  subStart : 4;
	char  subEnd   : 4;

	DaoxVector2D  P1; /* start point; */
	DaoxVector2D  P2; /* end point; */
	DaoxVector2D  C1; /* first control point; */
	DaoxVector2D  C2; /* second control point; */

	DaoxPathSegment  *first;   /* first subdivided segment; */
	DaoxPathSegment  *second;  /* second subdivided segment; */
	DaoxPathSegment  *next;    /* next segment in the path; */
};

struct DaoxPathComponent
{
	DaoxPath           *path;
	DaoxPathSegment    *first;
	DaoxPathSegment    *last;
	DaoxPathSegment    *refinedFirst;
	DaoxPathSegment    *refinedLast;
	DaoxPathComponent  *next;
};

struct DaoxPath
{
	DAO_CSTRUCT_COMMON;

	uchar_t      mode;
	uchar_t      cached;
	uchar_t      hashed;
	uint_t       hash;
	float        length;
	DaoxOBBox2D  obbox;

	DaoxPathComponent  *first;
	DaoxPathComponent  *last;

	DaoxPathComponent  *freeComponents;
	DaoxPathSegment    *freeSegments;
};

extern DaoType* daox_type_path;

DaoxPathSegment* DaoxPathSegment_New();
void DaoxPathSegment_Delete( DaoxPathSegment *self );
double DaoxPathSegment_Length( DaoxPathSegment *self, float factor );

DaoxPath* DaoxPath_New();
void DaoxPath_Delete( DaoxPath *self );
void DaoxPath_Reset( DaoxPath *self );
void DaoxPath_Copy( DaoxPath *self, DaoxPath *other );

uint_t DaoxPath_Hash( DaoxPath *self );

DAO_CANVAS_DLL void DaoxPath_SetRelativeMode( DaoxPath *self, int relative );
DAO_CANVAS_DLL void DaoxPath_MoveTo( DaoxPath *self, float x, float y );
DAO_CANVAS_DLL void DaoxPath_Close( DaoxPath *self );
DAO_CANVAS_DLL void DaoxPath_LineTo( DaoxPath *self, float x, float y );
DAO_CANVAS_DLL void DaoxPath_QuadTo( DaoxPath *self, float cx, float cy, float x, float y );
DAO_CANVAS_DLL void DaoxPath_CubicTo( DaoxPath *self, float cx, float cy, float x, float y );
DAO_CANVAS_DLL void DaoxPath_CubicTo2( DaoxPath *self, float cx1, float cy1, float cx2, float cy2, float x2, float y2 );
DAO_CANVAS_DLL void DaoxPath_ArcTo( DaoxPath *self, float x, float y, float degrees );
DAO_CANVAS_DLL void DaoxPath_ArcTo2( DaoxPath *self, float x, float y, float degrees, float deg2 );
DAO_CANVAS_DLL void DaoxPath_ArcBy( DaoxPath *self, float cx, float cy, float degrees );

void DaoxPath_ImportPath( DaoxPath *self, DaoxPath *path, DaoxMatrix3D *transform );

void DaoxPath_Refine( DaoxPath *self, float maxlen, float maxdiff );

DaoxPathSegment* DaoxPath_LocateByDistance( DaoxPath *self, float dist, DaoxVector3D *pos );

void DaoxPathSegment_Divide( DaoxPathSegment *self, float at );





struct DaoxPathStyle
{
	uchar_t  fill;
	uchar_t  cap;
	uchar_t  dash;
	uchar_t  junction;
	float    width;
	float    dashes[DAOX_MAX_DASH];
};

void DaoxPathStyle_Init( DaoxPathStyle *self );
void DaoxPathStyle_SetDashes( DaoxPathStyle *self, int count, float lens[] );



struct DaoxPathMesh
{
	DAO_CSTRUCT_COMMON;
	
	uint_t   hash;

	DaoxPathStyle  strokeStyle;

	DaoxPath  *path;
	DaoxPath  *stroke;
};
extern DaoType *daox_type_path_mesh;

DaoxPathMesh* DaoxPathMesh_New();
void DaoxPathMesh_Delete( DaoxPathMesh *self );

void DaoxPathMesh_Reset( DaoxPathMesh *self, DaoxPath *path, DaoxPathStyle *style );
void DaoxMeshPath_ComputeStroke( DaoxPathMesh *self );



struct DaoxPathCache
{
	DAO_CSTRUCT_COMMON;

	DMap      *paths;
	DMap      *meshes;

	DaoxPath  *unitLine;
	DaoxPath  *unitRect;
	DaoxPath  *unitCircle1;
	DaoxPath  *unitCircle2;
	DaoxPath  *unitCircle3;

	int  pathCount, meshCount;
};
extern DaoType *daox_type_path_cache;

DaoxPathCache* DaoxPathCache_New();
void DaoxPathCache_Delete( DaoxPathCache *self );

DaoxPath* DaoxPathCache_FindPath( DaoxPathCache *self, DaoxPath *path );
DaoxPathMesh* DaoxPathCache_FindMesh( DaoxPathCache *self, DaoxPath *path, DaoxPathStyle *style );

#endif
