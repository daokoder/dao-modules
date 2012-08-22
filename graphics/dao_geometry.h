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

typedef struct DaoxBounds            DaoxBounds;

typedef struct DaoxByteArray         DaoxByteArray;
typedef struct DaoxSliceArray        DaoxSliceArray;
typedef struct DaoxPointArray        DaoxPointArray;
typedef struct DaoxQuadArray         DaoxQuadArray;
typedef struct DaoxPolygonArray      DaoxPolygonArray;
typedef struct DaoxFloatArray        DaoxFloatArray;

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
	float  Axx, Axy;
	float  Ayx, Ayy;
	float  Bx, By;
};


struct DaoxBounds
{
	float  left;
	float  right;
	float  bottom;
	float  top;
};



struct DaoxBezierSegment
{
	unsigned int  count; /* count > 0: if this segment and its children are used; */

	float  start;  /* parametric start location in the original segment; */
	float  end;    /* parametric   end location in the original segment; */

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


struct DaoxFloatArray
{
	float  *values;
	int     count;
	int     capacity;
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


DAO_DLL void DaoxTransform_RotateXAxisTo( DaoxTransform *self, float x, float y );
DAO_DLL void DaoxTransform_RotateYAxisTo( DaoxTransform *self, float x, float y );
DAO_DLL void DaoxTransform_SetScale( DaoxTransform *self, float x, float y );
DAO_DLL void DaoxTransform_Multiply( DaoxTransform *self, DaoxTransform other );
DAO_DLL DaoxPoint DaoxTransform_Transform( DaoxTransform *self, DaoxPoint point );
DAO_DLL DaoxPoint DaoxTransform_TransformXY( DaoxTransform *self, float x, float y );
DAO_DLL DaoxTransform DaoxTransform_Inverse( DaoxTransform *self );


DAO_DLL void DaoxBounds_AddMargin( DaoxBounds *self, float margin );
DAO_DLL void DaoxBounds_Init( DaoxBounds *self, DaoxPoint point );
DAO_DLL void DaoxBounds_InitXY( DaoxBounds *self, float x, float y );
DAO_DLL void DaoxBounds_Update( DaoxBounds *self, DaoxPoint point );
DAO_DLL void DaoxBounds_UpdateXY( DaoxBounds *self, float x, float y );
DAO_DLL DaoxBounds DaoxBounds_Transform( DaoxBounds *self, DaoxTransform *t );
DAO_DLL DaoxBounds DaoxBounds_FromTriangle( DaoxPoint A, DaoxPoint B, DaoxPoint C );
DAO_DLL int DaoxBounds_Contain( DaoxBounds *self, DaoxBounds other );
DAO_DLL int DaoxBounds_Intersect( DaoxBounds *self, DaoxBounds other );
DAO_DLL int DaoxBounds_CheckTriangle( DaoxBounds *self, DaoxPoint A, DaoxPoint B, DaoxPoint C );
DAO_DLL int DaoxBounds_CheckQuad( DaoxBounds *self, DaoxQuad quad );


DAO_DLL DaoxByteArray* DaoxByteArray_New();
DAO_DLL void DaoxByteArray_Clear( DaoxByteArray *self );
DAO_DLL void DaoxByteArray_Delete( DaoxByteArray *self );
DAO_DLL void DaoxByteArray_Push( DaoxByteArray *self, uchar_t byte );
DAO_DLL void DaoxByteArray_Resize( DaoxByteArray *self, int count, uchar_t byte );



DAO_DLL DaoxFloatArray* DaoxFloatArray_New();
DAO_DLL void DaoxFloatArray_Delete( DaoxFloatArray *self );
DAO_DLL void DaoxFloatArray_Reset( DaoxFloatArray *self );
DAO_DLL void DaoxFloatArray_Push( DaoxFloatArray *self, float value );



DAO_DLL DaoxPointArray* DaoxPointArray_New();
DAO_DLL void DaoxPointArray_Clear( DaoxPointArray *self );
DAO_DLL void DaoxPointArray_Delete( DaoxPointArray *self );
DAO_DLL void DaoxPointArray_Reset( DaoxPointArray *self );
DAO_DLL void DaoxPointArray_Resize( DaoxPointArray *self, int count );
DAO_DLL void DaoxPointArray_PushXY( DaoxPointArray *self, float x, float y );
DAO_DLL void DaoxPointArray_Push( DaoxPointArray *self, DaoxPoint point );
DAO_DLL void DaoxPointArray_PushPoints( DaoxPointArray *self, DaoxPointArray *points );


DAO_DLL DaoxSliceArray* DaoxSliceArray_New();
DAO_DLL void DaoxSliceArray_Delete( DaoxSliceArray *self );
DAO_DLL void DaoxSliceArray_Push( DaoxSliceArray *self, int offset, int count );


DAO_DLL DaoxPolygonArray* DaoxPolygonArray_New();
DAO_DLL void DaoxPolygonArray_Delete( DaoxPolygonArray *self );
DAO_DLL void DaoxPolygonArray_Reset( DaoxPolygonArray *self );
DAO_DLL void DaoxPolygonArray_PushPolygon( DaoxPolygonArray *self );
DAO_DLL void DaoxPolygonArray_PushPointXY( DaoxPolygonArray *self, float x, float y );
DAO_DLL void DaoxPolygonArray_PushPoint( DaoxPolygonArray *self, DaoxPoint point );
DAO_DLL void DaoxPolygonArray_PushPoints( DaoxPolygonArray *self, DaoxPointArray *points );
DAO_DLL void DaoxPolygonArray_PushTriangle( DaoxPolygonArray *self, DaoxPoint A, DaoxPoint B, DaoxPoint C );
DAO_DLL void DaoxPolygonArray_PushRect( DaoxPolygonArray *self, DaoxPoint lb, DaoxPoint rt );
DAO_DLL void DaoxPolygonArray_PushQuad( DaoxPolygonArray *self, DaoxQuad quad );





DAO_DLL DaoxSimplePath* DaoxSimplePath_New();
DAO_DLL void DaoxSimplePath_Delete( DaoxSimplePath *self );
DAO_DLL void DaoxSimplePath_Reset( DaoxSimplePath *self );
DAO_DLL void DaoxSimplePath_MoveTo( DaoxSimplePath *self, float x, float y );
DAO_DLL void DaoxSimplePath_LineTo( DaoxSimplePath *self, float x, float y );
DAO_DLL void DaoxSimplePath_ArcTo( DaoxSimplePath *self, float x, float y, float degrees, int clockwise );
DAO_DLL void DaoxSimplePath_QuadTo( DaoxSimplePath *self, float cx, float cy, float x, float y );
DAO_DLL void DaoxSimplePath_CubicTo( DaoxSimplePath *self, float cx, float cy, float x, float y );
DAO_DLL void DaoxSimplePath_CubicTo2( DaoxSimplePath *self, float cx1, float cy1, float cx2, float cy2, float x2, float y2 );
DAO_DLL void DaoxSimplePath_Close( DaoxSimplePath *self );





DAO_DLL DaoxBezierSegment* DaoxBezierSegment_New();
DAO_DLL void DaoxBezierSegment_Delete( DaoxBezierSegment *self );

DAO_DLL void DaoxBezierSegment_SetPoints( DaoxBezierSegment *self, DaoxPoint P0, DaoxPoint P1, DaoxPoint P2, DaoxPoint P3 );
DAO_DLL void DaoxBezierSegment_DivideQuadratic( DaoxBezierSegment *self, float at );
DAO_DLL void DaoxBezierSegment_DivideCubic( DaoxBezierSegment *self, float at );
DAO_DLL void DaoxBezierSegment_RefineQuadratic( DaoxBezierSegment *self, float maxlen );
DAO_DLL void DaoxBezierSegment_RefineCubic( DaoxBezierSegment *self, float maxlen );
DAO_DLL void DaoxBezierSegment_ExportEndPoints( DaoxBezierSegment *self, DaoxPointArray *points );
DAO_DLL DaoxBezierSegment* DaoxBezierSegment_GetSegment( DaoxBezierSegment *self, float parloc );




/* Utility functions: */

DAO_DLL DaoxQuad DaoxQuad_FromRect( float left, float bottom, float right, float top );

DAO_DLL float DaoxDistance( DaoxPoint start, DaoxPoint end );
DAO_DLL float DaoxDistance2( DaoxPoint start, DaoxPoint end );
DAO_DLL float DaoxTriangle_Area( DaoxPoint A, DaoxPoint B, DaoxPoint C );
DAO_DLL float DaoxTriangle_AngleCosine( DaoxPoint C, DaoxPoint A, DaoxPoint B );
DAO_DLL DaoxQuad DaoxLine2Quad( DaoxPoint start, DaoxPoint end, float width );
DAO_DLL DaoxQuad DaoxLineJunctionMinor( DaoxPoint p1, DaoxPoint p2, DaoxPoint p3, float width );
DAO_DLL DaoxQuad DaoxQuadJunctionMinor( DaoxQuad *first, DaoxQuad *second );
DAO_DLL DaoxLine DaoxLineJunctionMajor( DaoxPoint p1, DaoxPoint p2, DaoxPoint p3, float width );
DAO_DLL DaoxLine DaoxQuadJunctionMajor( DaoxQuad *first, DaoxQuad *second, DaoxPoint c, float width );
DAO_DLL int DaoxLine_Intersect( DaoxPoint A, DaoxPoint B, DaoxPoint C, DaoxPoint D, float *S, float *T );
DAO_DLL int DaoxLineQuad_Junction( DaoxQuad first, DaoxQuad second, DaoxPoint *tip );





#ifdef __cplusplus
}
#endif


#endif
