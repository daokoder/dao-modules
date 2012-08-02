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

/*
// Path conversion:
//
// Converting arbitrary paths to simple paths without crossing edges:
// Each path contour must have a direction (clockwise or counter-clockwise);
// Path regions are determined by winding number.
*/

#ifndef __DAO_PATH_H__
#define __DAO_PATH_H__

#include "dao_geometry.h"

typedef struct DaoxFloatArray  DaoxFloatArray;

typedef struct DaoxPathNode    DaoxPathNode;
typedef struct DaoxPathEdge    DaoxPathEdge;
typedef struct DaoxPathGraph   DaoxPathGraph;
typedef struct DaoxQuadTree    DaoxQuadTree;

typedef struct DaoxPathSegment       DaoxPathSegment;


struct DaoxFloatArray
{
	float  *values;
	int     count;
	int     capacity;
};
DaoxFloatArray* DaoxFloatArray_New();
void DaoxFloatArray_Delete( DaoxFloatArray *self );
void DaoxFloatArray_Reset( DaoxFloatArray *self );
void DaoxFloatArray_Push( DaoxFloatArray *self, float value );
void DaoxFloatArray_Sort( DaoxFloatArray *self );


struct DaoxPathNode
{
	DaoxPathEdge  *in;
	DaoxPathEdge  *out;

	DaoxPoint  point;

	short  contour;  /* contour index (0 for open contour); */
	short  exported;
};

struct DaoxPathEdge
{
	DaoxPathGraph *graph;
	DaoxPathNode  *first;
	DaoxPathNode  *second;

	DaoxPointArray  points;

	DaoxFloatArray  breaks;

	short  command;
};


struct DaoxPathGraph
{
	DArray  *nodes;
	DArray  *edges;

	DArray  *nodes2;
	DArray  *edges2;

	DaoxQuadTree  *quadtree;

	DaoxBezierSegment  *bezier;
};


DaoxPathGraph* DaoxPathGraph_New();
void DaoxPathGraph_Delete( DaoxPathGraph *self );

void DaoxPathGraph_Reset( DaoxPathGraph *self );

void DaoxPathGraph_Import( DaoxPathGraph *self, DaoxPath *path );
void DaoxPathGraph_Export( DaoxPathGraph *self, DaoxPath *path );

void DaoxPathGraph_IntersectEdges( DaoxPathGraph *self );



struct DaoxPathSegment
{
	int  command;
	int  index;   /* sub-segment index in the original segment; */

	DaoxPoint  P1;
	DaoxPoint  P2;
	DaoxPoint  C1;  /* the first control point; */
	DaoxPoint  C2;  /* the second control point; */

	float  left;
	float  bottom;
	float  right;
	float  top;

	float  start;  /* parametric start location in the original segment; */
	float  end;    /* parametric   end location in the original segment; */

	DaoxPathEdge  *edge;
};



struct DaoxQuadTree
{
	float  west;
	float  south;
	float  east;
	float  north;

	DaoxQuadTree  *NW;
	DaoxQuadTree  *NE;
	DaoxQuadTree  *SW;
	DaoxQuadTree  *SE;

	DaoxPathSegment  *segments;

	int  total;  /* total number of segments in this node and its children; */
	int  count;  /* number of segments in this node; */
	int  capacity;  /* capacity of the segments array; */
};

DaoxQuadTree* DaoxQuadTree_New();
void DaoxQuadTree_Delete( DaoxQuadTree *self );

void DaoxQuadTree_Reset( DaoxQuadTree *self );
void DaoxQuadTree_Set( DaoxQuadTree *self, float west, float south, float east, float north );
void DaoxQuadTree_InsertEdge( DaoxQuadTree *self, DaoxPathEdge *edge );
void DaoxQuadTree_SubdivideSegments( DaoxQuadTree *self );



#endif
