/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2012, Limin Fu
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

#ifndef __DAO_GEOMETRY_H__
#define __DAO_GEOMETRY_H__


#include "dao.h"


enum DaoxPathCommands
{
	DAOX_PATH_MOVE_TO ,
	DAOX_PATH_LINE_TO ,
	DAOX_PATH_ARCR_TO ,  /* counter clockwise */
	DAOX_PATH_ARCL_TO ,  /* clockwise */
	DAOX_PATH_QUAD_TO ,
	DAOX_PATH_CUBIC_TO ,
	DAOX_PATH_CLOSE ,
};
enum DaoxPathJunctions
{
	DAOX_JUNCTION_NONE ,
	DAOX_JUNCTION_SHARP ,
	DAOX_JUNCTION_FLAT ,
	DAOX_JUNCTION_ROUND
};


typedef struct DaoxPoint             DaoxPoint;
typedef struct DaoxLine              DaoxLine;
typedef struct DaoxQuad              DaoxQuad;
typedef struct DaoxPath              DaoxPath;
typedef struct DaoxSlice             DaoxSlice;
typedef struct DaoxTransform         DaoxTransform;

typedef struct DaoxByteArray         DaoxByteArray;
typedef struct DaoxSliceArray        DaoxSliceArray;
typedef struct DaoxPointArray        DaoxPointArray;
typedef struct DaoxQuadArray         DaoxQuadArray;
typedef struct DaoxPolygonArray      DaoxPolygonArray;

typedef struct DaoxPathSegment       DaoxPathSegment;
typedef struct DaoxBezierSegment     DaoxBezierSegment;


struct DaoxPoint
{
	float x;
	float y;
};
struct DaoxLine
{
	DaoxPoint  start;
	DaoxPoint  end;
};
struct DaoxQuad
{
	DaoxPoint  A;
	DaoxPoint  B;
	DaoxPoint  C;
	DaoxPoint  D;
};
struct DaoxTransform
{
	float  A, B, C, D;
};

struct DaoxPathSegment
{
	int    command;

	DaoxPoint  cp1;  /* the first control point; */
	DaoxPoint  cp2;  /* the second control point; */

	DaoxBezierSegment  *bezier;
};


struct DaoxBezierSegment
{
	unsigned int  count; /* count > 0: if this segment and its children are used; */

	DaoxPoint  P0;
	DaoxPoint  P1;
	DaoxPoint  P2;
	DaoxPoint  P3;

	DaoxBezierSegment  *first;
	DaoxBezierSegment  *second;
};


struct DaoxByteArray
{
	uchar_t  *bytes;

	int  count;
	int  capacity;
};

struct DaoxPointArray
{
	DaoxPoint  *points;

	int  count;
	int  capacity;
};

struct DaoxQuadArray
{
	DaoxQuad  *quads;

	int  count;
	int  capacity;
};

struct DaoxSlice
{
	int  offset;
	int  count;
};

struct DaoxSliceArray
{
	DaoxSlice  *slices;

	int  count;
	int  capacity;
};


struct DaoxPolygonArray
{
	DaoxPointArray  *points;
	DaoxSliceArray  *polygons;
};


struct DaoxPath
{
	DaoxPointArray  *points;
	DaoxByteArray   *commands;
};





typedef struct DaoxPathBuffer DaoxPathBuffer;

struct DaoxPathBuffer
{
	DaoxPointArray  *points;
	DaoxByteArray   *junctions;

	DaoxBezierSegment  *bezier;

	struct DaoxTriangulator  *triangulator;
};





#ifdef __cplusplus
extern "C"{
#endif


DAO_DLL DaoxByteArray* DaoxByteArray_New();
DAO_DLL void DaoxByteArray_Clear( DaoxByteArray *self );
DAO_DLL void DaoxByteArray_Delete( DaoxByteArray *self );
DAO_DLL void DaoxByteArray_Push( DaoxByteArray *self, uchar_t byte );
DAO_DLL void DaoxByteArray_Resize( DaoxByteArray *self, int count, uchar_t byte );


DAO_DLL DaoxPointArray* DaoxPointArray_New();
DAO_DLL void DaoxPointArray_Clear( DaoxPointArray *self );
DAO_DLL void DaoxPointArray_Delete( DaoxPointArray *self );
DAO_DLL void DaoxPointArray_PushXY( DaoxPointArray *self, float x, float y );
DAO_DLL void DaoxPointArray_Push( DaoxPointArray *self, DaoxPoint point );


DAO_DLL DaoxSliceArray* DaoxSliceArray_New();
DAO_DLL void DaoxSliceArray_Delete( DaoxSliceArray *self );
DAO_DLL void DaoxSliceArray_Push( DaoxSliceArray *self, int offset, int count );


DAO_DLL DaoxPolygonArray* DaoxPolygonArray_New();
DAO_DLL void DaoxPolygonArray_Delete( DaoxPolygonArray *self );
DAO_DLL void DaoxPolygonArray_Reset( DaoxPolygonArray *self );
DAO_DLL void DaoxPolygonArray_PushPolygon( DaoxPolygonArray *self );
DAO_DLL void DaoxPolygonArray_PushPointXY( DaoxPolygonArray *self, float x, float y );
DAO_DLL void DaoxPolygonArray_PushPoint( DaoxPolygonArray *self, DaoxPoint point );
DAO_DLL void DaoxPolygonArray_PushTriangle( DaoxPolygonArray *self, DaoxPoint A, DaoxPoint B, DaoxPoint C );
DAO_DLL void DaoxPolygonArray_PushRect( DaoxPolygonArray *self, DaoxPoint lb, DaoxPoint rt );
DAO_DLL void DaoxPolygonArray_PushQuad( DaoxPolygonArray *self, DaoxQuad quad );





DAO_DLL DaoxPath* DaoxPath_New();
DAO_DLL void DaoxPath_Delete( DaoxPath *self );
DAO_DLL void DaoxPath_Reset( DaoxPath *self );
DAO_DLL void DaoxPath_MoveTo( DaoxPath *self, float x, float y );
DAO_DLL void DaoxPath_LineTo( DaoxPath *self, float x, float y );
DAO_DLL void DaoxPath_ArcTo( DaoxPath *self, float x, float y, float degrees, int clockwise );
DAO_DLL void DaoxPath_QuadTo( DaoxPath *self, float x, float y, float cx, float cy );
DAO_DLL void DaoxPath_CubicTo( DaoxPath *self, float x, float y, float cx, float cy );
DAO_DLL void DaoxPath_CubicTo2( DaoxPath *self, float cx0, float cy0, float x, float y, float cx, float cy );
DAO_DLL void DaoxPath_Close( DaoxPath *self );





DAO_DLL DaoxBezierSegment* DaoxBezierSegment_New();
DAO_DLL void DaoxBezierSegment_Delete( DaoxBezierSegment *self );

DAO_DLL void DaoxBezierSegment_SetPoints( DaoxBezierSegment *self, DaoxPoint P0, DaoxPoint P1, DaoxPoint P2, DaoxPoint P3 );
DAO_DLL void DaoxBezierSegment_RefineQuadratic( DaoxBezierSegment *self, float threshold );
DAO_DLL void DaoxBezierSegment_RefineCubic( DaoxBezierSegment *self, float threshold );




/* Utility functions: */

DAO_DLL DaoxQuad DaoxQuad_FromRect( float left, float bottom, float right, float top );

DAO_DLL float DaoxDistance( DaoxPoint start, DaoxPoint end );
DAO_DLL float DaoxDistance2( DaoxPoint start, DaoxPoint end );
DAO_DLL DaoxQuad DaoxLine2Quad( DaoxPoint start, DaoxPoint end, float width );
DAO_DLL DaoxQuad DaoxLineJunctionMinor( DaoxPoint p1, DaoxPoint p2, DaoxPoint p3, float width );
DAO_DLL DaoxQuad DaoxQuadJunctionMinor( DaoxQuad *first, DaoxQuad *second );
DAO_DLL DaoxLine DaoxLineJunctionMajor( DaoxPoint p1, DaoxPoint p2, DaoxPoint p3, float width );
DAO_DLL DaoxLine DaoxQuadJunctionMajor( DaoxQuad *first, DaoxQuad *second, DaoxPoint c, float width );




DAO_DLL void DaoxPolygonArray_MakeLines( DaoxPolygonArray *self, DaoxPointArray *points, DaoxByteArray *junctions, int width, int junction, int close );

DAO_DLL void DaoxPolygonArray_TriangulatePolygon( DaoxPolygonArray *self, DaoxPointArray *points, struct DaoxTriangulator *triangulator );

DAO_DLL void DaoxPath_MakePolygons( DaoxPath *self, int width, int junction, DaoxPolygonArray *strokes, DaoxPolygonArray *fills, DaoxPathBuffer *buffer );


#ifdef __cplusplus
}
#endif


#endif
