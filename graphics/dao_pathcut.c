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


#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "daoStdtype.h"
#include "dao_pathcut.h"


#define DAOX_MAX_TREE_DEPTH  6



DaoxFloatArray* DaoxFloatArray_New()
{
}
void DaoxFloatArray_Delete( DaoxFloatArray *self )
{
}
void DaoxFloatArray_Reset( DaoxFloatArray *self )
{
	self->count = 0;
}
void DaoxFloatArray_Push( DaoxFloatArray *self, double value )
{
	if( self->count >= self->capacity ){
		self->capacity += 0.2 * self->capacity + 1;
		self->values = (double*) dao_realloc( self->values, self->capacity * sizeof(double) );
	}
	self->values[ self->count ] = value;
	self->count += 1;
}
void DaoxFloatArray_QuickSort( double *values, int first, int last )
{
	double pivot, tmp;
	int lower = first+1, upper = last;

	if( first >= last ) return;
	tmp = values[first];
	values[first] = values[ (first+last)/2 ];
	values[ (first+last)/2 ] = tmp;
	pivot = values[ first ];

	while( lower <= upper ){
		while( lower < last && values[lower] < pivot ) lower ++;
		while( upper > first && pivot < values[upper] ) upper --;
		if( lower < upper ){
			tmp = values[lower];
			values[lower] = values[upper];
			values[upper] = tmp;
			upper --;
		}
		lower ++;
	}
	tmp = values[first];
	values[first] = values[upper];
	values[upper] = tmp;
	if( first+1 < upper ) DaoxFloatArray_QuickSort( values, first, upper-1 );
	if( upper+1 < last ) DaoxFloatArray_QuickSort( values, upper+1, last );
}
void DaoxFloatArray_Sort( DaoxFloatArray *self )
{
	if( self->count <= 1 ) return;
	DaoxFloatArray_QuickSort( self->values, 0, self->count - 1 );
}


DaoxPathNode* DaoxPathNode_New()
{
	DaoxPathNode *self = (DaoxPathNode*) dao_calloc( 1, sizeof(DaoxPathNode) );
	return self;
}
void DaoxPathNode_Delete( DaoxPathNode *self )
{
	dao_free( self );
}

DaoxPathEdge* DaoxPathEdge_New()
{
	DaoxPathEdge *self = (DaoxPathEdge*) dao_calloc( 1, sizeof(DaoxPathEdge) );
	return self;
}
void DaoxPathEdge_Delete( DaoxPathEdge *self )
{
	dao_free( self );
}

DaoxPathGraph* DaoxPathGraph_New()
{
	DaoxPathGraph *self = (DaoxPathGraph*) dao_calloc( 1, sizeof(DaoxPathGraph) );
	self->nodes = DArray_New(0);
	self->edges = DArray_New(0);
	self->nodes2 = DArray_New(0);
	self->edges2 = DArray_New(0);
	self->quadtree = DaoxQuadTree_New();
	self->bezier = DaoxBezierSegment_New();
	return self;
}
void DaoxPathGraph_Delete( DaoxPathGraph *self )
{
	daoint i;
	DArray *nodes = self->nodes2;
	DArray *edges = self->edges2;
	DaoxPathGraph_Reset( self );
	for(i=0; i<nodes->size; ++i) DaoxPathNode_Delete( (DaoxPathNode*) nodes->items.pVoid[i] );
	for(i=0; i<edges->size; ++i) DaoxPathEdge_Delete( (DaoxPathEdge*) edges->items.pVoid[i] );
	DArray_Delete( nodes );
	DArray_Delete( edges );
	DArray_Delete( self->nodes );
	DArray_Delete( self->edges );
	DaoxQuadTree_Delete( self->quadtree );
	DaoxBezierSegment_Delete( self->bezier );
	dao_free( self );
}

DaoxPathNode* DaoxPathGraph_NewNode( DaoxPathGraph *self, DaoxPoint point )
{
	DaoxPathNode* node = NULL;
	if( self->nodes2->size ){
		node = (DaoxPathNode*) DArray_Back( self->nodes2 );
		DArray_PopBack( self->nodes2 );
	}else{
		node = DaoxPathNode_New();
	}
	node->point = point;
	DArray_Append( self->nodes, node );
	return node;
}
DaoxPathEdge* DaoxPathGraph_NewEdge( DaoxPathGraph *self, DaoxPathNode *first, DaoxPathNode *second, short command )
{
	DaoxPathEdge* edge = NULL;
	if( self->edges2->size ){
		edge = (DaoxPathEdge*) DArray_Back( self->edges2 );
		DArray_PopBack( self->edges2 );
	}else{
		edge = DaoxPathEdge_New();
	}
	first->out = edge;
	second->in = edge;
	edge->first = first;
	edge->second = second;
	edge->command = command;
	edge->graph = self;
	DArray_Append( self->edges, edge );
	return edge;
}

void DaoxPathGraph_Reset( DaoxPathGraph *self )
{
	daoint i;
	for(i=0; i<self->nodes->size; ++i){
		DaoxPathNode *node = (DaoxPathNode*) self->nodes->items.pVoid[i];
		node->in = node->out = NULL;
		node->contour = node->exported = 0;
		DArray_Append( self->nodes2, node );
	}
	for(i=0; i<self->edges->size; ++i){
		DaoxPathEdge *edge = (DaoxPathEdge*) self->edges->items.pVoid[i];
		edge->first = edge->second = NULL;
		DaoxFloatArray_Reset( & edge->breaks );
		DArray_Append( self->edges2, edge );
	}
	DArray_Clear( self->nodes );
	DArray_Clear( self->edges );
}

void DaoxPathGraph_Import( DaoxPathGraph *self, DaoxPath *path )
{
	DaoxPoint *points = path->points->points;
	DaoxPathNode *first = NULL;
	DaoxPathNode *current = NULL;
	DaoxPathEdge *edge = NULL;
	int i, K = 0;

	if( path->commands->count == 0 ) return;

	for(i=0; i<path->commands->count; ++i){
		uchar_t command = path->commands->bytes[i];
		if( command == DAOX_PATH_MOVE_TO ){
			first = current = DaoxPathGraph_NewNode( self, points[ K++ ] );
		}else if( command == DAOX_PATH_LINE_TO ){
			DaoxPathNode *node = DaoxPathGraph_NewNode( self, points[ K++ ] );
			if( current == NULL ) continue;
			edge = DaoxPathGraph_NewEdge( self, current, node, command );
			current = node;
		}else if( command == DAOX_PATH_ARCR_TO || command == DAOX_PATH_ARCL_TO ){
			DaoxPoint center = points[ K++ ];
			DaoxPathNode *node = DaoxPathGraph_NewNode( self, points[ K++ ] );
			if( current == NULL ) continue;
			edge = DaoxPathGraph_NewEdge( self, current, node, command );
			edge->C1 = edge->C2 = center;
			current = node;
		}else if( command == DAOX_PATH_QUAD_TO ){
			DaoxPoint center = points[ K++ ];
			DaoxPathNode *node = DaoxPathGraph_NewNode( self, points[ K++ ] );
			if( current == NULL ) continue;
			edge = DaoxPathGraph_NewEdge( self, current, node, command );
			edge->C1 = edge->C2 = center;
			current = node;
		}else if( command == DAOX_PATH_CUBIC_TO ){
			DaoxPoint C1 = points[ K++ ];
			DaoxPoint C2 = points[ K++ ];
			DaoxPathNode *node = DaoxPathGraph_NewNode( self, points[ K++ ] );
			if( current == NULL ) continue;
			edge = DaoxPathGraph_NewEdge( self, current, node, command );
			edge->C1 = C1;
			edge->C2 = C2;
			current = node;
		}else if( command == DAOX_PATH_CLOSE ){
			if( first == NULL || current == NULL ) continue;
			edge = DaoxPathGraph_NewEdge( self, current, first, DAOX_PATH_LINE_TO );
			first = current = NULL;
		}
	}
	printf( "graph:  %3i  %3i\n", (int)self->nodes->size, (int)self->edges->size );
}
void DaoxPathGraph_Export( DaoxPathGraph *self, DaoxPath *path )
{
	DaoxPathNode *current = NULL;
	daoint i;

	DaoxPath_Reset( path );
	for(i=0; i<self->nodes->size; ++i){
		DaoxPathNode *node = (DaoxPathNode*) self->nodes->items.pVoid[i];
		node->exported = 0;
	}
	for(i=0; i<self->nodes->size; ++i){
		DaoxPathNode *node = (DaoxPathNode*) self->nodes->items.pVoid[i];
		if( node->exported ) continue;
		DaoxPath_MoveTo( path, node->point.x, node->point.y );
		current = node;
		do {
			DaoxPathEdge *edge = node->out;
			node->exported = 1;
			if( edge->second == current ){
				DaoxPath_Close( path );
				break;
			}
			switch( edge->command ){
			case DAOX_PATH_ARCR_TO :
			case DAOX_PATH_ARCL_TO :
			case DAOX_PATH_QUAD_TO :
				DaoxPointArray_Push( path->points, edge->C1 );
				break;
			case DAOX_PATH_CUBIC_TO :
				DaoxPointArray_Push( path->points, edge->C1 );
				DaoxPointArray_Push( path->points, edge->C2 );
				break;
			}
			DaoxPointArray_Push( path->points, edge->second->point );
			DaoxByteArray_Push( path->commands, edge->command );
			node = edge->second;
		} while( node && node != current );
	}
}

void DaoxPathSegment_Init( DaoxPathSegment *self, DaoxPathEdge *edge );
void DaoxQuadNode_SearchIntersections( DaoxQuadNode *self );


void DaoxPathGraph_BreakLine( DaoxPathGraph *self, DaoxPathEdge *edge )
{
	DaoxPathNode *node;
	DaoxPathEdge *edge2;
	DaoxPoint point;
	double t;
	int i;

	if( edge->breaks.count == 0 ) return;
	t = edge->breaks.values[ edge->breaks.count-1 ];
	if( t > 0.999999 ) goto NextBreak;

	point.x = (1.0 - t) * edge->first->point.x + t * edge->second->point.x;
	point.y = (1.0 - t) * edge->first->point.y + t * edge->second->point.y;
	node = DaoxPathGraph_NewNode( self, point );
	edge2 = DaoxPathGraph_NewEdge( self, node, edge->second, edge->command );
	edge->second = node;
	node->in = edge;

NextBreak:
	edge->breaks.count -= 1;
	if( edge->breaks.count == 0 ) return;
	for(i=0; i<edge->breaks.count; ++i) edge->breaks.values[i] /= t;
	DaoxPathGraph_BreakLine( self, edge );
}
void DaoxPathGraph_BreakArc( DaoxPathGraph *self, DaoxPathEdge *edge )
{
	DaoxPoint point;
	DaoxPathNode *node;
	DaoxPathEdge *edge2;
	double dx = edge->first->point.x - edge->C1.x;
	double dy = edge->first->point.y - edge->C1.y;
	double t, R, D, angle, area, sine, cosine;
	int i;

	if( edge->breaks.count == 0 ) return;
	t = edge->breaks.values[ edge->breaks.count-1 ];
	if( t > 0.999999 ) goto NextBreak;

	R = DaoxDistance2( edge->C1, edge->first->point );
	D = DaoxDistance2( edge->first->point, edge->second->point );
	angle = acos( (2.0*R - D) / (2.0*R) );

	area = DaoxTriangle_Area( edge->first->point, edge->second->point, edge->C1 );
	if( edge->command == DAOX_PATH_ARCR_TO ){
		if( area < 0.0 ) angle = 2.0 * M_PI - angle;
	}else{
		if( area < 0.0 ){
			angle = - angle;
		}else{
			angle = angle - 2.0 * M_PI;
		}
	}
	angle *= t;

	sine = sin( angle );
	cosine = cos( angle );

	point.x = edge->C1.x + dx * cosine - dy * sine;
	point.y = edge->C1.y + dx * sine + dy * cosine;

	printf( "intersecting at: %15f, %15f %15f\n", t, point.x, point.y );
	node = DaoxPathGraph_NewNode( self, point );
	edge2 = DaoxPathGraph_NewEdge( self, node, edge->second, edge->command );
	edge2->C1 = edge->C1;
	edge->second = node;
	node->in = edge;

NextBreak:
	edge->breaks.count -= 1;
	if( edge->breaks.count == 0 ) return;
	//printf( "intersecting at: %15f %15f\n", point.x, point.y );
	for(i=0; i<edge->breaks.count; ++i) edge->breaks.values[i] /= t;
	//printf( "intersecting at: %15f %15f\n", point.x, point.y );
	DaoxPathGraph_BreakArc( self, edge );
}
void DaoxPathGraph_BreakQuad( DaoxPathGraph *self, DaoxPathEdge *edge )
{
	DaoxPathNode *node;
	DaoxPathEdge *edge2;
	DaoxPoint P01, P12, C;
	double t;
	int i;

	if( edge->breaks.count == 0 ) return;
	t = edge->breaks.values[ edge->breaks.count-1 ];
	if( t > 0.999999 ) goto NextBreak;

	P01.x = (1.0 - t) * edge->first->point.x + t * edge->C1.x;
	P01.y = (1.0 - t) * edge->first->point.y + t * edge->C1.y;
	P12.x = (1.0 - t) * edge->C1.x + t * edge->second->point.x;
	P12.y = (1.0 - t) * edge->C1.y + t * edge->second->point.y;
	C.x = (1.0 - t) * P01.x + t * P12.x;
	C.y = (1.0 - t) * P01.y + t * P12.y;
	printf( "intersecting at: %15f %15f\n", C.x, C.y );
	node = DaoxPathGraph_NewNode( self, C );
	edge2 = DaoxPathGraph_NewEdge( self, node, edge->second, edge->command );
	edge2->C1 = P12;
	edge->C1 = P01;
	edge->second = node;
	node->in = edge;

NextBreak:
	edge->breaks.count -= 1;
	if( edge->breaks.count == 0 ) return;
	for(i=0; i<edge->breaks.count; ++i) edge->breaks.values[i] /= t;
	DaoxPathGraph_BreakQuad( self, edge );
}
void DaoxPathGraph_BreakCubic( DaoxPathGraph *self, DaoxPathEdge *edge )
{
	DaoxPathNode *node;
	DaoxPathEdge *edge2;
	DaoxPoint P01, P12, P23;
	DaoxPoint point, C1, C2;
	double t;
	int i;

	if( edge->breaks.count == 0 ) return;
	t = edge->breaks.values[ edge->breaks.count-1 ];
	if( t > 0.999999 ) goto NextBreak;

	P01.x = (1.0 - t) * edge->first->point.x + t * edge->C1.x;
	P01.y = (1.0 - t) * edge->first->point.y + t * edge->C1.y;
	P12.x = (1.0 - t) * edge->C1.x + t * edge->C2.x;
	P12.y = (1.0 - t) * edge->C1.y + t * edge->C2.y;
	P23.x = (1.0 - t) * edge->C2.x + t * edge->second->point.x;
	P23.y = (1.0 - t) * edge->C2.y + t * edge->second->point.y;
	C1.x = (1.0 - t) * P01.x + t * P12.x;
	C1.y = (1.0 - t) * P01.y + t * P12.y;
	C2.x = (1.0 - t) * P12.x + t * P23.x;
	C2.y = (1.0 - t) * P12.y + t * P23.y;
	point.x = (1.0 - t) * C1.x + t * C2.x;
	point.y = (1.0 - t) * C1.y + t * C2.y;
	printf( "intersecting at: %15f %15f\n", point.x, point.y );
	node = DaoxPathGraph_NewNode( self, point );
	edge2 = DaoxPathGraph_NewEdge( self, node, edge->second, edge->command );
	edge2->C1 = C2;
	edge2->C2 = P23;
	edge->C1 = P01;
	edge->C2 = C1;
	edge->second = node;
	node->in = edge;

NextBreak:
	edge->breaks.count -= 1;
	if( edge->breaks.count == 0 ) return;
	for(i=0; i<edge->breaks.count; ++i) edge->breaks.values[i] /= t;
	DaoxPathGraph_BreakCubic( self, edge );
}
void DaoxPathGraph_BreakEdge( DaoxPathGraph *self, DaoxPathEdge *edge )
{
	int i, k = 1;
	DaoxFloatArray_Sort( & edge->breaks );
	//printf( "breakings: %i\n", edge->breaks.count );
	if( edge->breaks.count ){
		for(i=1; i<edge->breaks.count; ++i){
			if( edge->breaks.values[i] != edge->breaks.values[k-1] ){
				edge->breaks.values[k++] = edge->breaks.values[i];
			}
		}
		edge->breaks.count = k;
	}
#if 0
#endif
	//printf( "breakings: %i\n", edge->breaks.count );
	for(i=0; i<edge->breaks.count; ++i) printf( "%3i: %15f\n", i, edge->breaks.values[i] );
	switch( edge->command ){
	case DAOX_PATH_LINE_TO : DaoxPathGraph_BreakLine( self, edge ); break;
	case DAOX_PATH_ARCL_TO : DaoxPathGraph_BreakArc( self, edge ); break;
	case DAOX_PATH_ARCR_TO : DaoxPathGraph_BreakArc( self, edge ); break;
	case DAOX_PATH_QUAD_TO : DaoxPathGraph_BreakQuad( self, edge ); break;
	case DAOX_PATH_CUBIC_TO : DaoxPathGraph_BreakCubic( self, edge ); break;
	}
}


void DaoxBoundingBox_Init( DaoxBoundingBox *self, DaoxPoint point );
void DaoxBoundingBox_Update( DaoxBoundingBox *self, DaoxPoint point );

int DaoxPointArray_SegmentArc( DaoxPointArray *self, DaoxPoint P0, DaoxPoint P1, DaoxPoint C, int D );

void DaoxPathGraph_IntersectEdges( DaoxPathGraph *self )
{
	DaoxPathNode *node;
	DaoxBoundingBox box;
	double left, right, bottom, top, width;
	daoint i, j, k, n;

	if( self->nodes->size == 0 ) return;

	node = (DaoxPathNode*) self->nodes->items.pVoid[0];
	DaoxBoundingBox_Init( & box, node->point );

	DaoxQuadTree_Reset( self->quadtree );
	for(i=0; i<self->edges->size; ++i){
		DaoxPathEdge *edge = (DaoxPathEdge*) self->edges->items.pVoid[i];
		DaoxPoint start = edge->first->point;
		DaoxPoint end = edge->second->point;
		DaoxBoundingBox_Update( & box, start );
		DaoxBoundingBox_Update( & box, end );
		switch( edge->command ){
		case DAOX_PATH_LINE_TO :
			break;
		case DAOX_PATH_ARCL_TO :
		case DAOX_PATH_ARCR_TO :
			k = edge->command == DAOX_PATH_ARCL_TO;
			self->quadtree->points->count = 0;
			DaoxPointArray_SegmentArc( self->quadtree->points, start, end, edge->C1, k );
			for(j=0; j<self->quadtree->points->count; ++j)
				DaoxBoundingBox_Update( & box, self->quadtree->points->points[j] );
			break;
		case DAOX_PATH_QUAD_TO :
			DaoxBoundingBox_Update( & box, edge->C1 );
			break;
		case DAOX_PATH_CUBIC_TO :
			DaoxBoundingBox_Update( & box, edge->C1 );
			DaoxBoundingBox_Update( & box, edge->C2 );
			break;
		}
	}
	left  = box.left - 1.0;
	right = box.right + 1.0;
	bottom = box.bottom - 1.0;
	top = box.top + 1.0;
	width = right - left;
	if( width < (top - bottom) ) width = top - bottom;
	printf( "bounding box: %15f %15f; %15f %15f\n", left, right, bottom, top );
	DaoxQuadTree_Set( self->quadtree, left, bottom, width );
	for(i=0; i<self->edges->size; ++i){
		DaoxPathEdge *edge = (DaoxPathEdge*) self->edges->items.pVoid[i];
		DaoxQuadTree_InsertEdge( self->quadtree, edge );
	}
	DaoxQuadNode_SearchIntersections( self->quadtree->root );
	for(i=0, n=self->edges->size; i<n; ++i){
		DaoxPathEdge *edge = (DaoxPathEdge*) self->edges->items.pVoid[i];
		//printf( "i = %3i: %3i\n", (int)i, edge->breaks.count );
		DaoxFloatArray_Sort( & edge->breaks );
		DaoxPathGraph_BreakEdge( self, edge );
	}
}




void DaoxBoundingBox_Init( DaoxBoundingBox *self, DaoxPoint point )
{
	self->left = self->right = point.x;
	self->bottom = self->top = point.y;
}
void DaoxBoundingBox_Update( DaoxBoundingBox *self, DaoxPoint point )
{
	if( point.x < self->left ) self->left = point.x;
	if( point.x > self->right ) self->right = point.x;
	if( point.y < self->bottom ) self->bottom = point.y;
	if( point.y > self->top ) self->top = point.y;
}


DaoxPathSegment* DaoxPathSegment_New()
{
	DaoxPathSegment *self = (DaoxPathSegment*) dao_calloc( 1, sizeof(DaoxPathSegment) );
	return self;
}
void DaoxPathSegment_Delete( DaoxPathSegment *self )
{
	dao_free( self );
}
void DaoxPathSegment_Init( DaoxPathSegment *self, DaoxPathEdge *edge )
{
	self->index = 0;
	self->command = edge->command;
	self->P1 = edge->first->point;
	self->P2 = edge->second->point;
	self->edge = edge;
	self->start = 0.0;
	self->end = 1.0;
}
void DaoxPathSegment_ResetBoundingBox( DaoxPathSegment *self )
{
	DaoxBoundingBox box;
	DaoxBoundingBox_Init( & box, self->P1 );
	DaoxBoundingBox_Update( & box, self->P2 );
	self->left = box.left;
	self->bottom = box.bottom;
	self->right = box.right;
	self->top = box.top;
}





DaoxQuadNode* DaoxQuadNode_New()
{
	DaoxQuadNode *self = (DaoxQuadNode*) dao_calloc( 1, sizeof(DaoxQuadNode) );
	return self;
}
void DaoxQuadNode_Delete( DaoxQuadNode *self )
{
	if( self->NW ){
		DaoxQuadNode_Delete( self->NW );
		DaoxQuadNode_Delete( self->NE );
		DaoxQuadNode_Delete( self->SW );
		DaoxQuadNode_Delete( self->SE );
	}
	dao_free( self );
}

void DaoxQuadNode_Set( DaoxQuadNode *self, int depth, double left, double bottom, double width )
{
	self->depth = depth;
	self->left = left;
	self->bottom = bottom;
	self->width = width;
}
void DaoxQuadNode_PushSegment( DaoxQuadNode *self, DaoxPathSegment segment, DaoxQuadTree *tree )
{
	DaoxPathSegment *S = DaoxQuadTree_NewPathSegment( tree );
	*S = segment;
	S->next = self->segments;
	self->segments = S;
}
int DaoxQuadNode_Insert( DaoxQuadNode *self, DaoxPathSegment segment, DaoxQuadTree *tree )
{
	double delta = 0.01 * self->width;
	double xm = self->left + 0.5 * self->width;
	double ym = self->bottom + 0.5 * self->width;
	double west = self->left, east = self->left + self->width;
	double south = self->bottom, north = self->bottom + self->width;
	int xcross, ycross;

	DaoxPathSegment_ResetBoundingBox( & segment );
	if( segment.left < west || segment.right >= east ) return 0;
	if( segment.bottom < south || segment.top >= north ) return 0;

	self->count += 1;

	//if( delta > 1 ) delta = 1;
	if( delta < 1E-3 ) delta = 1E-3;
	xcross = segment.left < xm+delta && segment.right > xm-delta;
	ycross = segment.bottom < ym+delta && segment.top > ym-delta;
	//printf( "DaoxQuadNode_Insert: %3i %3i %3i\n", self->depth, xcross, ycross );
	if( self->depth >= DAOX_MAX_TREE_DEPTH || xcross || ycross ){
		DaoxQuadNode_PushSegment( self, segment, tree );
		return 1;
	}
	if( self->NW == NULL ){
		self->NW = DaoxQuadNode_New();
		self->NE = DaoxQuadNode_New();
		self->SW = DaoxQuadNode_New();
		self->SE = DaoxQuadNode_New();
	}
	if( self->NW->depth == 0 ){
		/* No edges in the child nodes: */
		int depth = self->depth + 1;
		double width = 0.5 * self->width;
		DaoxQuadNode_Set( self->NW, depth, west,    ym, width );
		DaoxQuadNode_Set( self->NE, depth,   xm,    ym, width );
		DaoxQuadNode_Set( self->SW, depth, west, south, width );
		DaoxQuadNode_Set( self->SE, depth,   xm, south, width );
	}
	if( DaoxQuadNode_Insert( self->SW, segment, tree ) ) return 1;
	if( DaoxQuadNode_Insert( self->NW, segment, tree ) ) return 1;
	if( DaoxQuadNode_Insert( self->NE, segment, tree ) ) return 1;
	if( DaoxQuadNode_Insert( self->SE, segment, tree ) ) return 1;
	DaoxQuadNode_PushSegment( self, segment, tree );
	return 1;
}
void DaoxQuadNode_InsertLine( DaoxQuadNode *self, DaoxPathEdge *edge, DaoxQuadTree *tree )
{
	DaoxPathSegment segment;
	DaoxPoint start = edge->first->point;
	DaoxPoint end = edge->second->point;
	double d = tree->root->width / (1<<DAOX_MAX_TREE_DEPTH);
	double t, len = DaoxDistance( start, end );
	int i, n = 1 + (int) (len / d);

	DaoxPathSegment_Init( & segment, edge );
	segment.P2 = start;
	segment.end = 0.0;
	for(i=0; i<n; i++){
		t = (i + 1.0) / n;
		segment.index = i;
		segment.start = segment.end;
		segment.end = t;
		segment.P1 = segment.P2;
		segment.P2.x = (1.0 - t) * start.x + t * end.x;
		segment.P2.y = (1.0 - t) * start.y + t * end.y;
		DaoxQuadNode_Insert( self, segment, tree );
	}
}
int DaoxQuadNode_InsertArc2( DaoxQuadNode *self, DaoxPathSegment segment, DaoxQuadTree *tree )
{
	int index, mirror = 0;
	double area, max = tree->root->width / (1<<DAOX_MAX_TREE_DEPTH);
	double R = DaoxDistance2( segment.edge->C1, segment.P2 );
	double D12 = DaoxDistance2( segment.P1, segment.P2 );
	double cosine2 = (2.0*R - D12) / (2.0*R); /* cosine of the arc; */
	double cosine = sqrt( 1.0 + cosine2 );  /* cosine of a half arc; */
	double dx, dy, D;
	DaoxPathSegment S;
	DaoxPoint point;

	R = sqrt( R );
	D12 = sqrt( D12 );

	if( D12 < max && D12 < 0.05*R ){
		DaoxQuadNode_Insert( self, segment, tree );
		return segment.index + 1;
	}
	point.x = 0.5 * (segment.P1.x + segment.P2.x);
	point.y = 0.5 * (segment.P1.y + segment.P2.y);


	dx = segment.P1.x - point.x;
	dy = segment.P1.y - point.y;
	if( segment.command == DAOX_PATH_ARCR_TO ){
		dy = - dy;
	}else{
		dx = - dx;
	}
	point.x += dy;
	point.y += dx;
	D = DaoxDistance( segment.edge->C1, point );
	dx = R * (point.x - segment.edge->C1.x) / D;
	dy = R * (point.y - segment.edge->C1.y) / D;
	point.x = segment.edge->C1.x + dx;
	point.y = segment.edge->C1.y + dy;

	area = DaoxTriangle_Area( segment.P1, segment.P2, segment.edge->C1 );
	mirror |= segment.command == DAOX_PATH_ARCR_TO && area < 0.0;
	mirror |= segment.command == DAOX_PATH_ARCL_TO && area > 0.0;
	if( mirror ){
		point.x = 2.0 * segment.edge->C1.x - point.x;
		point.y = 2.0 * segment.edge->C1.y - point.y;
	}

	S = segment;
	S.P2 = point;
	S.end = 0.5 * (segment.start + segment.end);
	index = DaoxQuadNode_InsertArc2( self, S, tree );
	S = segment;
	S.P1 = point;
	S.index = index;
	S.start = 0.5 * (segment.start + segment.end);
	index = DaoxQuadNode_InsertArc2( self, S, tree );
	return index;
}
void DaoxQuadNode_InsertArc( DaoxQuadNode *self, DaoxPathEdge *edge, DaoxQuadTree *tree )
{
	DaoxPathSegment segment;
	DaoxPathSegment_Init( & segment, edge );
	DaoxQuadNode_InsertArc2( self, segment, tree );
}
int DaoxQuadNode_InsertBezier( DaoxQuadNode *self, DaoxBezierSegment *bezier, DaoxPathEdge *edge, DaoxQuadTree *tree, int index )
{
	DaoxPathSegment segment;
	if( bezier->count == 1 ){
		//printf( "index = %6i  %9p\n", index, edge );
		DaoxPathSegment_Init( & segment, edge );
		segment.index = index;
		segment.P1 = bezier->P0;
		segment.P2 = bezier->P3;
		segment.start = bezier->start;
		segment.end = bezier->end;
		DaoxQuadNode_Insert( self, segment, tree );
		index += 1;
	}else if( bezier->count ){
		index = DaoxQuadNode_InsertBezier( self, bezier->first, edge, tree, index );
		index = DaoxQuadNode_InsertBezier( self, bezier->second, edge, tree, index );
	}
	return index;
}
void DaoxQuadNode_InsertQuad( DaoxQuadNode *self, DaoxPathEdge *edge, DaoxQuadTree *tree )
{
	double d = tree->root->width / (1<<DAOX_MAX_TREE_DEPTH);
	DaoxBezierSegment *bezier = tree->bezier;
	DaoxPoint start = edge->first->point;
	DaoxPoint end = edge->second->point;

	DaoxBezierSegment_SetPoints( bezier, start, edge->C1, edge->C1, end );
	DaoxBezierSegment_RefineQuadratic( bezier, d );
	DaoxQuadNode_InsertBezier( self, bezier, edge, tree, 0 );
}
void DaoxQuadNode_InsertCubic( DaoxQuadNode *self, DaoxPathEdge *edge, DaoxQuadTree *tree )
{
	double d = tree->root->width / (1<<DAOX_MAX_TREE_DEPTH);
	DaoxBezierSegment *bezier = tree->bezier;
	DaoxPoint start = edge->first->point;
	DaoxPoint end = edge->second->point;

	DaoxBezierSegment_SetPoints( bezier, start, edge->C1, edge->C2, end );
	DaoxBezierSegment_RefineCubic( bezier, d );
	DaoxQuadNode_InsertBezier( self, bezier, edge, tree, 0 );
}
void DaoxQuadNode_InsertEdge( DaoxQuadNode *self, DaoxPathEdge *edge, DaoxQuadTree *tree )
{
	switch( edge->command ){
	case DAOX_PATH_LINE_TO :  DaoxQuadNode_InsertLine( self, edge, tree ); break;
	case DAOX_PATH_ARCL_TO :  DaoxQuadNode_InsertArc( self, edge, tree );  break;
	case DAOX_PATH_ARCR_TO :  DaoxQuadNode_InsertArc( self, edge, tree );  break;
	case DAOX_PATH_QUAD_TO :  DaoxQuadNode_InsertQuad( self, edge, tree ); break;
	case DAOX_PATH_CUBIC_TO : DaoxQuadNode_InsertCubic( self, edge, tree ); break;
	}
}
/*
// A,B intersect C,D with parametric location S,T:
//     (Cx-Ax)*(Dy-Cy) - (Cy-Ay)*(Dx-Cx)
// S = ---------------------------------
//     (Bx-Ax)*(Dy-Cy) - (By-Ay)*(Dx-Cx)
//
//     (Cx-Ax)*(By-Ay) - (Cy-Ay)*(Bx-Ax)
// T = ---------------------------------
//     (Bx-Ax)*(Dy-Cy) - (By-Ay)*(Dx-Cx)
*/
int DaoxPathSegment_LineIntersect( DaoxPathSegment *AB, DaoxPathSegment *CD, double *S, double *T )
{
	double S2, T2, BxAx, ByAy, CxAx, CyAy, DxCx, DyCy, K;
	//printf( "DaoxPathSegment_LineIntersect: %3i %3i\n", AB->index, CD->index );
	if( AB->edge == CD->edge ) return 0;
	if( AB->right < CD->left || AB->left > CD->right ) return 0;
	if( AB->top < CD->bottom || AB->bottom > CD->top ) return 0;
#if 0
	DaoxPoint p = {200.0,400};
	if( DaoxDistance( AB->P2, p ) < 200 ){
	printf( "1: %10f %10f %10f:  %10f, %10f  %10f, %10f\n", 0.0, 0.0, 0.0, AB->P1.x, AB->P1.y, AB->P2.x, AB->P2.y );
	printf( "1: %10f %10f %10f:  %10f, %10f  %10f, %10f\n", 0.0, 0.0, 0.0, CD->P1.x, CD->P1.y, CD->P2.x, CD->P2.y );
	}
#endif
	BxAx = AB->P2.x - AB->P1.x;
	ByAy = AB->P2.y - AB->P1.y;
	CxAx = CD->P1.x - AB->P1.x;
	CyAy = CD->P1.y - AB->P1.y;
	DxCx = CD->P2.x - CD->P1.x;
	DyCy = CD->P2.y - CD->P1.y;
	K = BxAx * DyCy - ByAy * DxCx;
	//printf( "DaoxPathSegment_LineIntersect: %3i %3i %15f\n", AB->index, CD->index, K );
	if( K == 0.0 ) return 0;

	S2 = (CxAx * DyCy - CyAy * DxCx) / K;
	T2 = (CxAx * ByAy - CyAy * BxAx) / K;
	/*
	// Do NOT consider intersection at the end of the segments,
	// it will be considered at the beginning of the next segments.
	*/
	if( S2 < 0 || S2 > 1.0 ) return 0;
	if( T2 < 0 || T2 > 1.0 ) return 0;
	*S = (1.0 - S2) * AB->start + S2 * AB->end;
	*T = (1.0 - T2) * CD->start + T2 * CD->end;
	if( *S <= 0 || *S >= 1.0 ) return 0;
	if( *T <= 0 || *T >= 1.0 ) return 0;
	//printf( "%10f %10f %10f:  %10f, %10f  %10f, %10f\n", K, *S, *T, AB->P2.x, AB->P2.y, CD->P1.x, CD->P1.y );
	//printf( "%10f, %10f  %10f, %10f\n", AB->edge->C1.x, AB->edge->C1.y, CD->edge->C1.x, CD->edge->C1.y );
	return 1;
}
void DaoxQuadNode_SearchIntersections2( DaoxQuadNode *self, DaoxPathSegment *S2 )
{
	DaoxPathSegment *S1;
	double T1 = 0, T2 = 0;
	int i;

	if( self->count == 0 ) return;

	if( S2->right < self->left || S2->left > (self->left + self->width) ) return;
	if( S2->top < self->bottom || S2->bottom > (self->bottom + self->width) ) return;

	for(S1=self->segments; S1; S1=S1->next){
		if( DaoxPathSegment_LineIntersect( S1, S2, & T1, & T2 ) ){
			if( T1 > 0.0 && T1 < 1.0 ) DaoxFloatArray_Push( & S1->edge->breaks, T1 );
			if( T2 > 0.0 && T2 < 1.0 ) DaoxFloatArray_Push( & S2->edge->breaks, T2 );
		}
	}
	if( self->NW && self->NW->count > 0 ) DaoxQuadNode_SearchIntersections( self->NW );
	if( self->NE && self->NE->count > 0 ) DaoxQuadNode_SearchIntersections( self->NE );
	if( self->SW && self->SW->count > 0 ) DaoxQuadNode_SearchIntersections( self->SW );
	if( self->SE && self->SE->count > 0 ) DaoxQuadNode_SearchIntersections( self->SE );
}
void DaoxQuadNode_SearchIntersections( DaoxQuadNode *self )
{
	DaoxPathSegment *S1, *S2;
	DaoxQuadNode *nodes[4];
	double T1 = 0, T2 = 0;
	int i, j;

	//printf( "depth = %3i;  segments = %19p; %5i\n", self->depth, self->segments, self->count );
	if( self->count <= 1 ) return;

	nodes[0] = self->NW;
	nodes[1] = self->NE;
	nodes[2] = self->SW;
	nodes[3] = self->SE;

	for(S1=self->segments; S1; S1=S1->next){
		for(S2=self->segments; S2!=S1; S2=S2->next){
			if( DaoxPathSegment_LineIntersect( S1, S2, & T1, & T2 ) ){
				if( T1 > 0.0 && T1 < 1.0 ) DaoxFloatArray_Push( & S1->edge->breaks, T1 );
				if( T2 > 0.0 && T2 < 1.0 ) DaoxFloatArray_Push( & S2->edge->breaks, T2 );
			}
		}
		for(j=0; j<4; ++j){
			if( nodes[j] && nodes[j]->count ) DaoxQuadNode_SearchIntersections2( nodes[j], S1 );
		}
	}
	for(j=0; j<4; ++j){
		if( nodes[j] && nodes[j]->count > 1 ) DaoxQuadNode_SearchIntersections( nodes[j] );
	}
}




DaoxQuadTree* DaoxQuadTree_New()
{
	DaoxQuadTree *self = (DaoxQuadTree*) dao_calloc( 1, sizeof(DaoxQuadTree) );
	self->root = DaoxQuadNode_New();
	self->root->depth = 1;
	self->points = DaoxPointArray_New();
	self->bezier = DaoxBezierSegment_New();
	return self;
}
void DaoxQuadTree_Delete( DaoxQuadTree *self )
{
	DaoxPathSegment *seg = self->segments;
	DaoxQuadTree_Reset( self );
	DaoxQuadNode_Delete( self->root );
	DaoxPointArray_Delete( self->points );
	DaoxBezierSegment_Delete( self->bezier );
	while( seg ){
		DaoxPathSegment *s = seg;
		seg = seg->next;
		DaoxPathSegment_Delete( s );
	}
	dao_free( self );
}
void DaoxQuadTree_ResetNode( DaoxQuadTree *self, DaoxQuadNode *node )
{
	node->depth = 0;
	if( node->NW ){
		DaoxQuadTree_ResetNode( self, node->NW );
		DaoxQuadTree_ResetNode( self, node->NE );
		DaoxQuadTree_ResetNode( self, node->SW );
		DaoxQuadTree_ResetNode( self, node->SE );
	}
	if( node->segments == NULL ) return;
	node->segments->next = self->segments;
	self->segments = node->segments;
	node->segments = NULL;
}
void DaoxQuadTree_Reset( DaoxQuadTree *self )
{
	DaoxQuadTree_ResetNode( self, self->root );
	self->root->depth = 1;
}
void DaoxQuadTree_Set( DaoxQuadTree *self, double left, double bottom, double width )
{
	DaoxQuadNode_Set( self->root, 1, left, bottom, width );
}
DaoxPathSegment* DaoxQuadTree_NewPathSegment( DaoxQuadTree *self )
{
	DaoxPathSegment *segment;
	if( self->segments ){
		segment = self->segments;
		self->segments = segment->next;
		segment->next = NULL;
		return segment;
	}
	return DaoxPathSegment_New();
}
void DaoxQuadTree_InsertEdge( DaoxQuadTree *self, DaoxPathEdge *edge )
{
	DaoxQuadNode_InsertEdge( self->root, edge, self );
}
