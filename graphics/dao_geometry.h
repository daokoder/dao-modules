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
typedef struct DaoxTransform         DaoxTransform;

typedef struct DaoxBounds            DaoxBounds;

typedef struct DaoxPointArray        DaoxPointArray;
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



struct DaoxPointArray
{
	DaoxPoint  *points;

	int  count;
	int  capacity;
};



struct DaoxFloatArray
{
	float  *values;
	int     count;
	int     capacity;
};






#ifdef __cplusplus
extern "C"{
#endif


DAO_DLL void DaoxTransform_Set( DaoxTransform *self, float *mat, int n );
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






/* Utility functions: */

DAO_DLL DaoxQuad DaoxQuad_FromRect( float left, float bottom, float right, float top );

DAO_DLL float DaoxDistance( DaoxPoint start, DaoxPoint end );
DAO_DLL float DaoxDistance2( DaoxPoint start, DaoxPoint end );
DAO_DLL float DaoxTriangle_Area( DaoxPoint A, DaoxPoint B, DaoxPoint C );
DAO_DLL float DaoxTriangle_AngleCosine( DaoxPoint C, DaoxPoint A, DaoxPoint B );
DAO_DLL int DaoxTriangle_Contain( DaoxPoint C, DaoxPoint A, DaoxPoint B, DaoxPoint P );
DAO_DLL DaoxQuad DaoxLine2Quad( DaoxPoint start, DaoxPoint end, float width );
DAO_DLL int DaoxLine_Intersect( DaoxPoint A, DaoxPoint B, DaoxPoint C, DaoxPoint D, float *S, float *T );
DAO_DLL int DaoxLineQuad_Junction( DaoxQuad first, DaoxQuad second, DaoxPoint *tip );





#ifdef __cplusplus
}
#endif


#endif
