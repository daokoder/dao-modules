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


enum DaoxSimplePathCommands
{
	DAOX_PATH_MOVE_TO ,
	DAOX_PATH_LINE_TO ,
	DAOX_PATH_ARCR_TO ,  /* counter clockwise (right-hand) */
	DAOX_PATH_ARCL_TO ,  /* clockwise */
	DAOX_PATH_QUAD_TO ,
	DAOX_PATH_CUBIC_TO ,
	DAOX_PATH_CLOSE ,
};
enum DaoxSimplePathJunctions
{
	DAOX_JUNCTION_NONE ,
	DAOX_JUNCTION_SHARP ,
	DAOX_JUNCTION_FLAT ,
	DAOX_JUNCTION_ROUND
};


typedef struct DaoxPoint             DaoxPoint;
typedef struct DaoxLine              DaoxLine;
typedef struct DaoxQuad              DaoxQuad;
typedef struct DaoxSimplePath              DaoxSimplePath;
typedef struct DaoxSlice             DaoxSlice;
typedef struct DaoxTransform         DaoxTransform;

typedef struct DaoxByteArray         DaoxByteArray;
typedef struct DaoxSliceArray        DaoxSliceArray;
typedef struct DaoxPointArray        DaoxPointArray;
typedef struct DaoxQuadArray         DaoxQuadArray;
typedef struct DaoxPolygonArray      DaoxPolygonArray;

typedef struct DaoxBezierSegment     DaoxBezierSegment;


struct DaoxPoint
{
	double x;
	double y;
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
	double  A, B, C, D;
};



struct DaoxBezierSegment
{
	unsigned int  count; /* count > 0: if this segment and its children are used; */

	double  start;  /* parametric start location in the original segment; */
	double  end;    /* parametric   end location in the original segment; */

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


struct DaoxSimplePath
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

DaoxPathBuffer* DaoxPathBuffer_New();
void DaoxPathBuffer_Delete( DaoxPathBuffer *self );




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
DAO_DLL void DaoxPointArray_PushXY( DaoxPointArray *self, double x, double y );
DAO_DLL void DaoxPointArray_Push( DaoxPointArray *self, DaoxPoint point );
DAO_DLL void DaoxPointArray_PushPoints( DaoxPointArray *self, DaoxPointArray *points );


DAO_DLL DaoxSliceArray* DaoxSliceArray_New();
DAO_DLL void DaoxSliceArray_Delete( DaoxSliceArray *self );
DAO_DLL void DaoxSliceArray_Push( DaoxSliceArray *self, int offset, int count );


DAO_DLL DaoxPolygonArray* DaoxPolygonArray_New();
DAO_DLL void DaoxPolygonArray_Delete( DaoxPolygonArray *self );
DAO_DLL void DaoxPolygonArray_Reset( DaoxPolygonArray *self );
DAO_DLL void DaoxPolygonArray_PushPolygon( DaoxPolygonArray *self );
DAO_DLL void DaoxPolygonArray_PushPointXY( DaoxPolygonArray *self, double x, double y );
DAO_DLL void DaoxPolygonArray_PushPoint( DaoxPolygonArray *self, DaoxPoint point );
DAO_DLL void DaoxPolygonArray_PushPoints( DaoxPolygonArray *self, DaoxPointArray *points );
DAO_DLL void DaoxPolygonArray_PushTriangle( DaoxPolygonArray *self, DaoxPoint A, DaoxPoint B, DaoxPoint C );
DAO_DLL void DaoxPolygonArray_PushRect( DaoxPolygonArray *self, DaoxPoint lb, DaoxPoint rt );
DAO_DLL void DaoxPolygonArray_PushQuad( DaoxPolygonArray *self, DaoxQuad quad );





DAO_DLL DaoxSimplePath* DaoxSimplePath_New();
DAO_DLL void DaoxSimplePath_Delete( DaoxSimplePath *self );
DAO_DLL void DaoxSimplePath_Reset( DaoxSimplePath *self );
DAO_DLL void DaoxSimplePath_MoveTo( DaoxSimplePath *self, double x, double y );
DAO_DLL void DaoxSimplePath_LineTo( DaoxSimplePath *self, double x, double y );
DAO_DLL void DaoxSimplePath_ArcTo( DaoxSimplePath *self, double x, double y, double degrees, int clockwise );
DAO_DLL void DaoxSimplePath_QuadTo( DaoxSimplePath *self, double cx, double cy, double x, double y );
DAO_DLL void DaoxSimplePath_CubicTo( DaoxSimplePath *self, double cx, double cy, double x, double y );
DAO_DLL void DaoxSimplePath_CubicTo2( DaoxSimplePath *self, double cx1, double cy1, double cx2, double cy2, double x2, double y2 );
DAO_DLL void DaoxSimplePath_Close( DaoxSimplePath *self );





DAO_DLL DaoxBezierSegment* DaoxBezierSegment_New();
DAO_DLL void DaoxBezierSegment_Delete( DaoxBezierSegment *self );

DAO_DLL void DaoxBezierSegment_SetPoints( DaoxBezierSegment *self, DaoxPoint P0, DaoxPoint P1, DaoxPoint P2, DaoxPoint P3 );
DAO_DLL void DaoxBezierSegment_DivideQuadratic( DaoxBezierSegment *self, double at );
DAO_DLL void DaoxBezierSegment_DivideCubic( DaoxBezierSegment *self, double at );
DAO_DLL void DaoxBezierSegment_RefineQuadratic( DaoxBezierSegment *self, double maxlen );
DAO_DLL void DaoxBezierSegment_RefineCubic( DaoxBezierSegment *self, double maxlen );
DAO_DLL void DaoxBezierSegment_ExportEndPoints( DaoxBezierSegment *self, DaoxPointArray *points );
DAO_DLL DaoxBezierSegment* DaoxBezierSegment_GetSegment( DaoxBezierSegment *self, double parloc );




/* Utility functions: */

DAO_DLL DaoxQuad DaoxQuad_FromRect( double left, double bottom, double right, double top );

DAO_DLL double DaoxDistance( DaoxPoint start, DaoxPoint end );
DAO_DLL double DaoxDistance2( DaoxPoint start, DaoxPoint end );
DAO_DLL double DaoxTriangle_Area( DaoxPoint A, DaoxPoint B, DaoxPoint C );
DAO_DLL DaoxQuad DaoxLine2Quad( DaoxPoint start, DaoxPoint end, double width );
DAO_DLL DaoxQuad DaoxLineJunctionMinor( DaoxPoint p1, DaoxPoint p2, DaoxPoint p3, double width );
DAO_DLL DaoxQuad DaoxQuadJunctionMinor( DaoxQuad *first, DaoxQuad *second );
DAO_DLL DaoxLine DaoxLineJunctionMajor( DaoxPoint p1, DaoxPoint p2, DaoxPoint p3, double width );
DAO_DLL DaoxLine DaoxQuadJunctionMajor( DaoxQuad *first, DaoxQuad *second, DaoxPoint c, double width );
DAO_DLL int DaoxLine_Intersect( DaoxPoint A, DaoxPoint B, DaoxPoint C, DaoxPoint D, double *S, double *T );
DAO_DLL int DaoxLineQuad_Junction( DaoxQuad first, DaoxQuad second, DaoxPoint *tip );




DAO_DLL void DaoxPolygonArray_MakeLines( DaoxPolygonArray *self, DaoxPointArray *points, DaoxByteArray *junctions, double width, int junction, int close );

DAO_DLL void DaoxPolygonArray_TriangulatePolygon( DaoxPolygonArray *self, DaoxPointArray *points, struct DaoxTriangulator *triangulator );

DAO_DLL void DaoxSimplePath_MakePolygons( DaoxSimplePath *self, double width, int junction, DaoxPolygonArray *strokes, DaoxPolygonArray *fills, DaoxPathBuffer *buffer );


#ifdef __cplusplus
}
#endif


#endif
