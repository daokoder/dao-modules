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


#ifndef __DAO_PATH_H__
#define __DAO_PATH_H__


#include "dao_geometry.h"
#include "dao_triangulator.h"


struct DaoxGraphicsData;

typedef struct DaoxPathSegment    DaoxPathSegment;
typedef struct DaoxPathComponent  DaoxPathComponent;
typedef struct DaoxPath           DaoxPath;


typedef struct DaoxIntArray  DaoxIntArray;


struct DaoxIntArray
{
	int  *values;
	int   count;
	int   capacity;
};
DaoxIntArray* DaoxIntArray_New();
void DaoxIntArray_Delete( DaoxIntArray *self );
void DaoxIntArray_Reset( DaoxIntArray *self );
void DaoxIntArray_Push( DaoxIntArray *self, int value );




struct DaoxPathSegment
{
	char  bezier;  /* 0: open; 1: linear; 2: quadratic; 3: cubic; */
	char  convexness;  /* 0: flat; 1: locally convex; -1: locally concave; */
	char  dash;    /* 0: gap; 1: dash; */

	int    count;

	double  length;  /* length; */
	double  delta;

	double  start;  /* parametric start location in the original segment; */
	double  end;    /* parametric   end location in the original segment; */

	DaoxPoint  P1; /* start point; */
	DaoxPoint  P2; /* end point; */
	DaoxPoint  C1; /* first control point; */
	DaoxPoint  C2; /* second control point; */

	DaoxPathSegment  *first;   /* first subdivided segment; */
	DaoxPathSegment  *second;  /* second subdivided segment; */
	DaoxPathSegment  *next;    /* next segment in the path; */

	DaoxPathComponent  *component;  /* host path component; */
};

struct DaoxPathComponent
{
	DaoxPath  *path;

	DaoxPathSegment  *first;
	DaoxPathSegment  *last;

	DaoxPathSegment  *cache;

	struct {
		DaoxPathSegment  *first;
		DaoxPathSegment  *last;
	} refined;

	DaoxPathComponent  *next;

	float  maxlen;
	float  maxdiff;
	double length;
};

struct DaoxPath
{
	DaoxPathComponent  *first;
	DaoxPathComponent  *last;

	DaoxPathComponent  *cache;

	DaoxPointArray  *points;
	DaoxIntArray    *triangles;

	double  length;
};

DaoxPathSegment* DaoxPathSegment_New( DaoxPathComponent *component );
void DaoxPathSegment_Delete( DaoxPathSegment *self );
void DaoxPathSegment_SetPoints( DaoxPathSegment *self, DaoxPoint P1, DaoxPoint P2, DaoxPoint C1, DaoxPoint C2 );

DaoxPath* DaoxPath_New();
void DaoxPath_Delete( DaoxPath *self );
void DaoxPath_Reset( DaoxPath *self );


DAO_DLL void DaoxPath_MoveTo( DaoxPath *self, double x, double y );
DAO_DLL void DaoxPath_LineTo( DaoxPath *self, double x, double y );
DAO_DLL void DaoxPath_ArcTo( DaoxPath *self, double x, double y, double degrees );
DAO_DLL void DaoxPath_ArcTo2( DaoxPath *self, double x, double y, double degrees, double deg2 );
DAO_DLL void DaoxPath_QuadTo( DaoxPath *self, double cx, double cy, double x, double y );
DAO_DLL void DaoxPath_CubicTo( DaoxPath *self, double cx, double cy, double x, double y );
DAO_DLL void DaoxPath_CubicTo2( DaoxPath *self, double cx1, double cy1, double cx2, double cy2, double x2, double y2 );
DAO_DLL void DaoxPath_Close( DaoxPath *self );

void DaoxPath_ImportPath( DaoxPath *self, DaoxPath *path, DaoxTransform *transform );

void DaoxPath_Preprocess( DaoxPath *self, DaoxPathBuffer *buffer );

void DaoxPath_Refine( DaoxPath *self, double maxlen, double maxdiff );

void DaoxPath_ExportGraphicsData( DaoxPath *self, struct DaoxGraphicsData *gdata );


#endif
