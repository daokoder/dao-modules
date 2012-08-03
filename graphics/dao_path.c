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
#include "dao_path.h"



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
void DaoxFloatArray_Push( DaoxFloatArray *self, float value )
{
	if( self->count >= self->capacity ){
		self->capacity += 0.2 * self->capacity + 1;
		self->values = (float*) dao_realloc( self->values, self->capacity * sizeof(float) );
	}
	self->values[ self->count ] = value;
	self->count += 1;
}
void DaoxFloatArray_QuickSort( float *values, int first, int last )
{
	float pivot, tmp;
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
	DaoxPointArray_Clear( & self->points );
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
		edge->points.count = 0;
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
			DaoxPointArray_Push( & edge->points, center );
			current = node;
			// TODO: extra points;
		}else if( command == DAOX_PATH_QUAD_TO ){
			DaoxPoint center = points[ K++ ];
			DaoxPathNode *node = DaoxPathGraph_NewNode( self, points[ K++ ] );
			if( current == NULL ) continue;
			edge = DaoxPathGraph_NewEdge( self, current, node, command );
			DaoxPointArray_Push( & edge->points, center );
			current = node;
		}else if( command == DAOX_PATH_CUBIC_TO ){
			DaoxPoint C1 = points[ K++ ];
			DaoxPoint C2 = points[ K++ ];
			DaoxPathNode *node = DaoxPathGraph_NewNode( self, points[ K++ ] );
			if( current == NULL ) continue;
			edge = DaoxPathGraph_NewEdge( self, current, node, command );
			DaoxPointArray_Push( & edge->points, C1 );
			DaoxPointArray_Push( & edge->points, C2 );
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
			//printf( "%p %p\n", node, edge );
			if( edge->second == current ){
				DaoxPath_Close( path );
				break;
			}
			switch( edge->command ){
			case DAOX_PATH_ARCR_TO :
				//	TODO:
				break;
			case DAOX_PATH_ARCL_TO :
				//	TODO:
				break;
			case DAOX_PATH_QUAD_TO :
				DaoxPointArray_Push( path->points, edge->points.points[0] );
				break;
			case DAOX_PATH_CUBIC_TO :
				DaoxPointArray_Push( path->points, edge->points.points[1] );
				DaoxPointArray_Push( path->points, edge->points.points[2] );
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
	float t;
	int i;

	if( edge->breaks.count == 0 ) return;
	t = edge->breaks.values[ edge->breaks.count-1 ];
	point.x = (1.0 - t) * edge->first->point.x + t * edge->second->point.x;
	point.y = (1.0 - t) * edge->first->point.y + t * edge->second->point.y;
	node = DaoxPathGraph_NewNode( self, point );
	edge2 = DaoxPathGraph_NewEdge( self, node, edge->second, edge->command );
	edge->second = node;
	node->in = edge;
	edge->breaks.count -= 1;
	if( edge->breaks.count == 0 ) return;
	for(i=0; i<edge->breaks.count; ++i) edge->breaks.values[i] /= t;
	DaoxPathGraph_BreakLine( self, edge );
}
void DaoxPathGraph_BreakArc( DaoxPathGraph *self, DaoxPathEdge *edge, int clockwise )
{
}
void DaoxPathGraph_BreakQuad( DaoxPathGraph *self, DaoxPathEdge *edge )
{
}
void DaoxPathGraph_BreakCubic( DaoxPathGraph *self, DaoxPathEdge *edge )
{
}
void DaoxPathGraph_BreakEdge( DaoxPathGraph *self, DaoxPathEdge *edge )
{
	DaoxFloatArray_Sort( & edge->breaks );
	// TODO:
	switch( edge->command ){
	case DAOX_PATH_LINE_TO : DaoxPathGraph_BreakLine( self, edge ); break;
	case DAOX_PATH_ARCL_TO : DaoxPathGraph_BreakArc( self, edge, 1 ); break;
	case DAOX_PATH_ARCR_TO : DaoxPathGraph_BreakArc( self, edge, 0 ); break;
	case DAOX_PATH_QUAD_TO : DaoxPathGraph_BreakQuad( self, edge ); break;
	case DAOX_PATH_CUBIC_TO : DaoxPathGraph_BreakCubic( self, edge ); break;
	}
}



void DaoxPathGraph_IntersectEdges( DaoxPathGraph *self )
{
	DaoxPathNode *node;
	DaoxPathSegment segment = {0};
	float left, right, bottom, top, width;
	daoint i, n;

	if( self->nodes->size == 0 ) return;

	node = (DaoxPathNode*) self->nodes->items.pVoid[0];
	left = right = node->point.x;
	bottom = top = node->point.y;

	DaoxQuadTree_Reset( self->quadtree );
	for(i=0; i<self->edges->size; ++i){
		DaoxPathEdge *edge = (DaoxPathEdge*) self->edges->items.pVoid[i];
		DaoxPathSegment_Init( & segment, edge );
		if( segment.left < left ) left = segment.left;
		if( segment.bottom < bottom ) bottom = segment.bottom;
		if( segment.right > right ) right = segment.right;
		if( segment.top > top ) top = segment.top;
	}
	left -= 1.0;
	right += 1.0;
	bottom -= 1.0;
	top += 1.0;
	width = right - left;
	if( width < (top - bottom) ) width = top - bottom;
	printf( "%15f %15f; %15f %15f\n", left, right, bottom, top );
	DaoxQuadTree_Set( self->quadtree, left, bottom, width );
	for(i=0; i<self->edges->size; ++i){
		DaoxPathEdge *edge = (DaoxPathEdge*) self->edges->items.pVoid[i];
		DaoxQuadTree_InsertEdge( self->quadtree, edge );
	}
	DaoxQuadNode_SearchIntersections( self->quadtree->root );
	for(i=0, n=self->edges->size; i<n; ++i){
		DaoxPathEdge *edge = (DaoxPathEdge*) self->edges->items.pVoid[i];
		printf( "i = %3i: %3i\n", (int)i, edge->breaks.count );
		DaoxFloatArray_Sort( & edge->breaks );
		DaoxPathGraph_BreakEdge( self, edge );
	}
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
void DaoxPathSegment_UpdateBoundingBox( DaoxPathSegment *self, DaoxPoint point )
{
	if( point.x < self->left ) self->left = point.x;
	if( point.x > self->right ) self->right = point.x;
	if( point.y < self->bottom ) self->bottom = point.y;
	if( point.y > self->top ) self->top = point.y;
}
void DaoxPathSegment_ResetBoundingBox( DaoxPathSegment *self )
{
	int i;
	self->left = self->right = self->P1.x;
	self->bottom = self->top = self->P1.y;
	DaoxPathSegment_UpdateBoundingBox( self, self->P2 );
#if 0
	for(i=0; i<self->points.count; ++i){
		DaoxPathSegment_UpdateBoundingBox( self, self->points.points[i] );
	}
#endif
}
void DaoxPathSegment_Init( DaoxPathSegment *self, DaoxPathEdge *edge )
{
	self->command = edge->command;
	self->P1 = edge->first->point;
	self->P2 = edge->second->point;
	self->edge = edge;
	self->start = 0.0;
	self->end = 1.0;
	// TODO: arc, bezier;
	DaoxPathSegment_ResetBoundingBox( self );
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

void DaoxQuadNode_Set( DaoxQuadNode *self, int depth, float left, float bottom, float width )
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
	float xm = self->left + 0.5 * self->width;
	float ym = self->bottom + 0.5 * self->width;
	float west = self->left, east = self->left + self->width;
	float south = self->bottom, north = self->bottom + self->width;
	int xcross = segment.left < xm && segment.right > xm;
	int ycross = segment.bottom < ym && segment.top > ym;

	if( segment.left < west || segment.right >= east ) return 0;
	if( segment.bottom < south || segment.top >= north ) return 0;

	self->count += 1;
	if( self->depth >= 9 || xcross || ycross ){
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
		float width = 0.5 * self->width;
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
	float t, len = DaoxDistance( start, end );
	int i, n = 1 + (int) (len / 8);

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
void DaoxQuadNode_InsertArc( DaoxQuadNode *self, DaoxPathEdge *edge, DaoxQuadTree *tree )
{
}
void DaoxQuadNode_InsertQuad( DaoxQuadNode *self, DaoxPathEdge *edge, DaoxQuadTree *tree )
{
}
void DaoxQuadNode_InsertCubic( DaoxQuadNode *self, DaoxPathEdge *edge, DaoxQuadTree *tree )
{
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
int DaoxPathSegment_LineIntersect( DaoxPathSegment *AB, DaoxPathSegment *CD, float *S, float *T )
{
	float S2, T2, BxAx, ByAy, CxAx, CyAy, DxCx, DyCy, K;
	if( AB->edge == CD->edge && abs(AB->index - CD->index) <= 1 ) return 0;
	if( AB->right < CD->left || AB->left > CD->right ) return 0;
	if( AB->top < CD->bottom || AB->bottom > CD->top ) return 0;
	BxAx = AB->P2.x - AB->P1.x;
	ByAy = AB->P2.y - AB->P1.y;
	CxAx = CD->P1.x - AB->P1.x;
	CyAy = CD->P1.y - AB->P1.y;
	DxCx = CD->P2.x - CD->P1.x;
	DyCy = CD->P2.y - CD->P1.y;
	K = BxAx * DyCy - ByAy * DxCx;
	if( K == 0.0 ) return 0;

	S2 = (CxAx * DyCy - CyAy * DxCx) / K;
	T2 = (CxAx * ByAy - CyAy * BxAx) / K;
	/*
	// Do NOT consider intersection at the end of the segments,
	// it will be considered at the beginning of the next segments.
	*/
	if( S2 < 0.0 || S2 >= 1.0 ) return 0;
	if( T2 < 0.0 || T2 >= 1.0 ) return 0;
	*S = (1.0 - S2) * AB->start + S2 * AB->end;
	*T = (1.0 - T2) * CD->start + T2 * CD->end;
	printf( "%10f %10f %10f:  %10f, %10f  %10f, %10f\n", K, *S, *T, AB->P2.x, AB->P2.y, CD->P1.x, CD->P1.y );
	return 1;
}
void DaoxQuadNode_SearchIntersections2( DaoxQuadNode *self, DaoxPathSegment *S2 )
{
	DaoxPathSegment *S1;
	float T1 = 0, T2 = 0;
	int i;

	if( self->count == 0 ) return;

	if( S2->right < self->left || S2->left > (self->left + self->width) ) return;
	if( S2->top < self->bottom || S2->bottom > (self->bottom + self->width) ) return;

	for(S1=self->segments; S1; S1=S1->next){
		if( DaoxPathSegment_LineIntersect( S1, S2, & T1, & T2 ) ){
			if( T1 > 0.001 && T1 < 0.999 ) DaoxFloatArray_Push( & S1->edge->breaks, T1 );
			if( T2 > 0.001 && T2 < 0.999 ) DaoxFloatArray_Push( & S2->edge->breaks, T2 );
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
	float T1 = 0, T2 = 0;
	int i, j;

	printf( "segments = %9p; %i\n", self->segments, self->count );
	if( self->count <= 1 ) return;

	nodes[0] = self->NW;
	nodes[1] = self->NE;
	nodes[2] = self->SW;
	nodes[3] = self->SE;

	for(S1=self->segments; S1; S1=S1->next){
		for(S2=self->segments; S2!=S1; S2=S2->next){
			printf( "S1 = %9p; S2 = %9p\n", S1, S2 );
			if( DaoxPathSegment_LineIntersect( S1, S2, & T1, & T2 ) ){
				if( T1 > 0.001 && T1 < 0.999 ) DaoxFloatArray_Push( & S1->edge->breaks, T1 );
				if( T2 > 0.001 && T2 < 0.999 ) DaoxFloatArray_Push( & S2->edge->breaks, T2 );
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
	return self;
}
void DaoxQuadTree_Delete( DaoxQuadTree *self )
{
	DaoxPathSegment *seg = self->segments;
	DaoxQuadTree_Reset( self );
	DaoxQuadNode_Delete( self->root );
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
void DaoxQuadTree_Set( DaoxQuadTree *self, float left, float bottom, float width )
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
