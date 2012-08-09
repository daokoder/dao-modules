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
#include <assert.h>
#include "daoValue.h"
#include "dao_triangulator.h"




DaoxVertex* DaoxVertex_New( daoint index )
{
	DaoxVertex *self = (DaoxVertex*) dao_calloc( 1, sizeof(DaoxVertex) );
	self->index = index;
	return self;
}
void DaoxVertex_Delete( DaoxVertex *self )
{
	dao_free( self );
}

DaoxTriangulator* DaoxTriangulator_New()
{
	DaoxTriangulator *self = (DaoxTriangulator*) dao_calloc( 1, sizeof(DaoxTriangulator) );
	self->points = DaoxPointArray_New();
	self->vertices = DArray_New(0);
	self->worklist = DArray_New(0);
	self->triangles = DArray_New(0);
	return self;
}
void DaoxTriangulator_Delete( DaoxTriangulator *self )
{
	DaoxVertex *vertex;
	DaoxTriangulator_Reset( self );
	vertex = self->caches;
	while( vertex ){
		DaoxVertex *v = vertex;
		vertex = vertex->next;
		DaoxVertex_Delete( v );
	}
	DaoxPointArray_Delete( self->points );
	DArray_Delete( self->vertices );
	DArray_Delete( self->worklist );
	DArray_Delete( self->triangles );
	dao_free( self );
}
void DaoxTriangulator_Reset( DaoxTriangulator *self )
{
	daoint i;
	for(i=0; i<self->vertices->size; ++i){
		DaoxVertex *vertex = (DaoxVertex*) self->vertices->items.pVoid[i];
		memset( vertex, 0, sizeof(DaoxVertex) );
		vertex->next = self->caches;
		self->caches = vertex;
	}
	self->points->count = 0;
	self->vertices->size = 0;
	self->worklist->size = 0;
	self->triangles->size = 0;
	self->start = NULL;
}

static DaoxVertex* DaoxTriangulator_GetVertex( DaoxTriangulator *self, daoint index )
{
	DaoxVertex *vertex = self->caches;
	if( vertex ){
		self->caches = vertex->next;
		vertex->index = index;
		vertex->next = NULL;
	}else{
		vertex = DaoxVertex_New( index );
	}
	return vertex;
}

void DaoxTriangulator_PushPoint( DaoxTriangulator *self, double x, double y )
{
	DaoxVertex *prev = NULL, *vertex = DaoxTriangulator_GetVertex( self, self->points->count );
	if( self->vertices->size ){
		prev = (DaoxVertex*)  self->vertices->items.pVoid[self->vertices->size-1];
		if( prev->next ){
			vertex->contour = prev->contour + 1;
		}else{
			vertex->contour = prev->contour;
			prev->next = vertex;
			vertex->prev = prev;
		}
	}
	DaoxPointArray_PushXY( self->points, x, y );
	DArray_PushBack( self->vertices, vertex );
	if( self->start == NULL ) self->start = vertex;
}
void DaoxTriangulator_PopPoint( DaoxTriangulator *self )
{
	DaoxVertex *vertex;
	if( self->vertices->size == 0 ) return;
	vertex = (DaoxVertex*) self->vertices->items.pVoid[self->vertices->size-1];
	if( vertex->prev ) vertex->prev->next = NULL;
	DaoxVertex_Delete( vertex );
	self->vertices->size -= 1;
	self->points->count -= 1;
}
int DaoxTriangulator_CloseContour( DaoxTriangulator *self )
{
	DaoxVertex *vertex;
	if( self->start == NULL || self->vertices->size == 0 ) return 0;
	vertex = (DaoxVertex*) self->vertices->items.pVoid[self->vertices->size-1];
	/* less than 3 vertices; */
	if( self->start->next == NULL || self->start->next->next == NULL ){
		while( vertex->next == NULL ){
			DaoxTriangulator_PopPoint( self );
			if( self->vertices->size == 0 ) break;
			vertex = (DaoxVertex*) self->vertices->items.pVoid[self->vertices->size-1];
		}
		return 0;
	}
	vertex->next = self->start;
	self->start->prev = vertex;
	self->start = NULL;
	return 1;
}

void DaoxTriangulator_MakeTriangle( DaoxTriangulator *self, DaoxVertex *A )
{
	DArray_PushBack( self->triangles, (void*)(daoint) A->prev->index );
	DArray_PushBack( self->triangles, (void*)(daoint) A->index );
	DArray_PushBack( self->triangles, (void*)(daoint) A->next->index );
}

void DaoxTriangulator_QuickSortVertices( DaoxTriangulator *self, DaoxVertex *vertices[], int first, int last )
{
	DaoxVertex *pivot, *tmp;
	DaoxPoint *points = self->points->points;
	daoint lower=first+1, upper=last;

	if( first >= last ) return;
	tmp = vertices[first];
	vertices[first] = vertices[ (first+last)/2 ];
	vertices[ (first+last)/2 ] = tmp;
	pivot = vertices[ first ];

	while( lower <= upper ){
		while( lower < last && points[vertices[lower]->index].x < points[pivot->index].x ) lower ++;
		while( upper > first && points[pivot->index].x < points[vertices[upper]->index].x ) upper --;
		if( lower < upper ){
			tmp = vertices[lower];
			vertices[lower] = vertices[upper];
			vertices[upper] = tmp;
			upper --;
		}
		lower ++;
	}
	tmp = vertices[first];
	vertices[first] = vertices[upper];
	vertices[upper] = tmp;
	if( first+1 < upper ) DaoxTriangulator_QuickSortVertices( self, vertices, first, upper-1 );
	if( upper+1 < last ) DaoxTriangulator_QuickSortVertices( self, vertices, upper+1, last );
}
void DaoxTriangulator_SortVertices( DaoxTriangulator *self )
{
	DaoxVertex **vertices = (DaoxVertex**) self->vertices->items.pVoid;
	int i, N = self->vertices->size;
	if( N <= 1 ) return;
	DaoxTriangulator_QuickSortVertices( self, vertices, 0, N-1 );
	for(i=0; i<N; ++i) vertices[i]->sorting = i;
}
void DaoxTriangulator_InitContourOrientation( DaoxTriangulator *self )
{
	DaoxVertex *vertex, *start;
	DaoxPoint A, B, C, *points = self->points->points;
	int i, dir, count, N = self->vertices->size;
	double area;

	/* sort vertices by the x coordinates: */
	DaoxTriangulator_SortVertices( self );

	for(i=0; i<N; ++i){
		start = self->vertices->items.pVoid[i];
		if( start->direction != 0 ) continue;
		count = 0;
		vertex = start;
		do {
			A = points[vertex->index];
			B = points[vertex->next->index];
			C = points[vertex->prev->index];
			area = DaoxTriangle_Area( A, B, C );
			if( area > 0.0 ) count += 1;
			if( area < 0.0 ) count -= 1;
			vertex = vertex->next;
		} while( vertex != start );

		dir = count < 0 ? DAOX_CLOCKWISE : DAOX_COUNTER_CW;
		vertex = start;
		do {
			vertex->direction = dir;
			vertex = vertex->next;
		} while( vertex != start );
	}
}
void DaoxTriangulator_Triangulate( DaoxTriangulator *self )
{
	DaoxVertex *V, *A, *B, *C, *inside;
	DaoxPoint PA, PB, PC, P, *points = self->points->points;
	int i, imin, imax, contours, K = 0, N = self->vertices->size;
	double dist, area, ymin, ymax, dmax;
	double AB, BC, CA, min_area = 1E-9;

	DaoxTriangulator_InitContourOrientation( self );

	V = (DaoxVertex*) self->vertices->items.pVoid[N-1];
	contours = V->contour;

	DArray_Assign( self->worklist, self->vertices );
	while( self->worklist->size && (++K) < 10*N ){
		A = (DaoxVertex*) self->worklist->items.pVoid[self->worklist->size-1];
		if( A->done ){
			DArray_PopBack( self->worklist );
			continue;
		}
		B = A->next;
		C = A->prev;
		PA = points[A->index];
		PB = points[B->index];
		PC = points[C->index];
		area = DaoxTriangle_Area( PA, PB, PC );
		if( B->next == C ){ /* already a triangle: */
			if( fabs( area ) > min_area ) DaoxTriangulator_MakeTriangle( self, A );
			A->next->prev = A->prev;
			A->prev->next = A->next;
			A->done = B->done = C->done = 1;
			DArray_PopBack( self->worklist );
			continue;
		}

		ymin = ymax = PA.y;
		if( PB.y < ymin ) ymin = PB.y; else if( PB.y > ymax ) ymax = PB.y;
		if( PC.y < ymin ) ymin = PC.y; else if( PC.y > ymax ) ymax = PC.y;
		imin = B->sorting < C->sorting ? B->sorting : C->sorting;
		imax = A->sorting;
		inside = NULL;
		dmax = - 1.0;
		/*
		// find the closest point to the triangle:
		//
		// Note: no need to distinguish duplicated vertices, because:
		// 1. if they are outside of the trianlge, they cause no problem;
		// 2. they cannot be the "closest" vertex to "A" or another vertex again,
		//    so no problem for joining inner contour or splitting outer contour.
		//
		// The reason that they cannot be the "closest" vertex is that,
		// only a concave vertex from the outer contour or a convex
		// vertex from an inner contour for a hole can be chosen as the
		// the "closest" vertex.
		//
		// And when its original vertex was choose as the "closest" vertex,
		// the associated "A" will always be processed until it is removed
		// from the polygon(s), which will leave the duplicated vertices
		// as convex vertices on the outer contour!
		*/
		for(i=imin+1; i<imax; ++i){
			V = (DaoxVertex*) self->vertices->items.pVoid[i];
			P = points[V->index];
			if( V->done ) continue;
			//printf( "%3i: %12f %12f %9p, %3i %3i %3i %3i\n", i, P.x, P.y, V, V->contour, A->contour, V->direction, A->direction );
			/* Ingore vertex from other contour with the same direction: */
			if( V->contour != A->contour && V->direction == A->direction ) continue;
			if( V->sorting == A->sorting || V->sorting == B->sorting || V->sorting == C->sorting )
				continue;
			if( P.y > ymax ) continue;
			if( P.y < ymin ) continue;
			//printf( "%3i: %12f %12f %9p, %3i %3i %3i %3i\n", i, P.x, P.y, V, V->contour, A->contour, V->direction, A->direction );
			BC = DaoxTriangle_Area( P, PB, PC );
			AB = DaoxTriangle_Area( P, PA, PB );
			CA = DaoxTriangle_Area( P, PC, PA );
			//if( A->direction == DAOX_CLOCKWISE ){
			if( area < 0.0 ){
				AB = -AB;
				BC = -BC;
				CA = -CA;
			}
			//printf( "%3i: %12f %12f  %12f\n", i, AB, BC, CA );
#if 0
			if( V->direction != A->direction && BC == 0.0 ) continue;
			if( BC > 0.0 && BC > dmax ){
				dmax = BC;
				inside = V;
			}
			continue;
#endif
			if( BC >= 0.0 && AB >= 0.0 && CA >= 0.0 ){
				/*
				// Consider vertex from a different contour with different direction,
				// only if is fully inside the triangle:
				*/
				//if( V->direction != A->direction && BC == 0.0 ) continue;
				//if( V->direction != A->direction && (BC == 0.0 || AB == 0.0 || CA == 0.0) ) continue;
				if( BC > dmax ){
					dmax = BC;
					inside = V;
				}
			}
		}
#if 0
		printf( "A:  %15f  %15f   %9p\n", PA.x, PA.y, A );
		printf( "B:  %15f  %15f   %9p\n", PB.x, PB.y, B );
		printf( "C:  %15f  %15f   %9p\n", PC.x, PC.y, C );
		if( inside ) printf( "I:  %15f  %15f\n", points[inside->index].x, points[inside->index].y );
		printf( "%p\n", inside );
		printf( "area: %15f\n", area );
#endif
		if( inside == NULL ){
			A->done = 1;
			//printf( "area: %15f\n", DaoxTriangle_Area( PA, PB, PC ) );
			if( fabs( area ) > min_area ) DaoxTriangulator_MakeTriangle( self, A );
			A->next->prev = A->prev;
			A->prev->next = A->next;
			DArray_PopBack( self->worklist );
			//if( self->triangles->size >= 3*78 ) break;
		}else{ /* point inside the triangle: */
			DaoxVertex *A2, *N2;
			int breaking = inside->contour == A->contour;
			if( inside->contour != A->contour ){ /* joining contour: */
				/* the "inside" vertex is from a hole: */
				DaoxVertex *V2 = inside;
				//printf( "joining\n" );
				/* update contour: */
				do {
					V2->contour = A->contour;
					V2->direction = A->direction;
					V2 = V2->next;
				} while( V2 != inside );
			}
			/*
			// connect "inside" to "A" with duplicated vertices, this will either:
			// 1. connect the inner contour with the outer contour;
			// 2. or break the outer contour.
			 */
			N2 = DaoxTriangulator_GetVertex( self, inside->index );
			A2 = DaoxTriangulator_GetVertex( self, A->index );
			N2->sorting = inside->sorting;
			A2->sorting = A->sorting;
			A2->contour = N2->contour = A->contour;
			A2->direction = N2->direction = A->direction;

			A2->prev = A->prev;  A->prev->next = A2;
			A2->next = N2;  N2->prev = A2;
			N2->next = inside->next;  inside->next->prev = N2;
			inside->next = A;  A->prev = inside;

			//printf( "dup:  %9p  %9p\n", A2, N2 );

			if( breaking ){
				contours += 1;
				//printf( "contours = %3i\n", contours );
				DaoxVertex *V2 = inside;
				/* update contour: */
				do {
					V2->contour = contours;
					V2 = V2->next;
				} while( V2 != inside );
			}

			DArray_PushBack( self->vertices, N2 );
			DArray_PushBack( self->vertices, A2 );
			DArray_PopBack( self->worklist );
			DArray_PushBack( self->worklist, N2 );
			DArray_PushBack( self->worklist, A2 );
			DArray_PushBack( self->worklist, A );
		}
	}
}

void DaoxTriangulator_ExportTriangles( DaoxTriangulator *self, DaoxPolygonArray *polygons )
{
	int i;
	for(i=0; i<self->triangles->size; i+=3){
		daoint *ids = self->triangles->items.pInt + i;
		DaoxPoint A = self->points->points[ids[0]];
		DaoxPoint B = self->points->points[ids[1]];
		DaoxPoint C = self->points->points[ids[2]];
		DaoxPolygonArray_PushTriangle( polygons, A, B, C );
	}
}





static DaoType* daox_type_triangulator = NULL;

static void TRIA_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxTriangulator *self = DaoxTriangulator_New();
	DaoProcess_PutCdata( proc, self, daox_type_triangulator );
}
static void TRIA_Push( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxTriangulator *self = (DaoxTriangulator*) DaoValue_TryCastCdata( p[0], daox_type_triangulator );
	DaoxTriangulator_PushPoint( self, p[1]->xDouble.value, p[2]->xDouble.value );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void TRIA_Close( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxTriangulator *self = (DaoxTriangulator*) DaoValue_TryCastCdata( p[0], daox_type_triangulator );
	DaoxTriangulator_CloseContour( self );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void TRIA_Triangulate( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxTriangulator *self = (DaoxTriangulator*) DaoValue_TryCastCdata( p[0], daox_type_triangulator );
	DaoxTriangulator_Triangulate( self );
	DaoProcess_PutInteger( proc, self->triangles->size / 3 );
}
static void TRIA_Get( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxTriangulator *self = (DaoxTriangulator*) DaoValue_TryCastCdata( p[0], daox_type_triangulator );
	DaoTuple *tuple = DaoProcess_PutTuple( proc, 3 );
	daoint *indeces = self->triangles->items.pInt;
	daoint index = p[1]->xInteger.value;
	if( index < 0 || 3*index >= self->triangles->size ) return;
	indeces += 3 * index;
	tuple->items[0]->xInteger.value = indeces[0];
	tuple->items[1]->xInteger.value = indeces[1];
	tuple->items[2]->xInteger.value = indeces[2];
}
static DaoFuncItem DaoxTriangulatorMeths[]=
{
	{ TRIA_New,     "Triangulator()" },
	{ TRIA_Push,    "PushPoint( self : Triangulator, x : double, y : double ) => Triangulator" },
	{ TRIA_Close,   "CloseContour( self : Triangulator ) => Triangulator" },
	{ TRIA_Triangulate,   "Triangulate( self : Triangulator ) => int" },
	{ TRIA_Get,           "[]( self : Triangulator, index : int ) => tuple<int,int,int>" },
	{ NULL, NULL }
};

DaoTypeBase DaoxTriangulator_Typer =
{
	"Triangulator", NULL, NULL, (DaoFuncItem*) DaoxTriangulatorMeths, { NULL }, { NULL },
	(FuncPtrDel)DaoxTriangulator_Delete, NULL
};


DAO_DLL int DaoTriangulator_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_type_triangulator = DaoNamespace_WrapType( ns, & DaoxTriangulator_Typer, 1 );
	return 0;
}
