/*
// Dao Graphics Engine
// http://www.daovm.net
//
// Copyright (c) 2012-2017, Limin Fu
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
#include "dao_canvas.h"




DaoxGradient* DaoxGradient_New( DaoVmSpace *vmspace, int type )
{
	DaoxGradient *self = (DaoxGradient*) dao_calloc(1,sizeof(DaoxGradient));
	DaoTypeCore *core = & daoGradientCore;
	switch( type ){
	case DAOX_GRADIENT_BASE : core = & daoGradientCore; break;
	case DAOX_GRADIENT_LINEAR : core = & daoLinearGradientCore; break;
	case DAOX_GRADIENT_RADIAL : core = & daoRadialGradientCore; break;
	}
	DaoCstruct_Init( (DaoCstruct*)self, DaoVmSpace_GetType( vmspace, core ) );
	self->stops = DArray_New( sizeof(float) );
	self->colors = DArray_New( sizeof(DaoxColor) );
	self->gradient = type;
	return self;
}
void DaoxGradient_Delete( DaoxGradient *self )
{
	DArray_Delete( self->stops );
	DArray_Delete( self->colors );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}
void DaoxGradient_Add( DaoxGradient *self, float stop, DaoxColor color )
{
	DaoxColor *C = (DaoxColor*) DArray_Push( self->colors );
	DArray_PushFloat( self->stops, stop );
	*C = color;
}
void DaoxGradient_Copy( DaoxGradient *self, DaoxGradient *other )
{
	self->gradient = other->gradient;
	self->radius = other->radius;
	self->points[0] = other->points[0];
	self->points[1] = other->points[1];
	DArray_Assign( self->stops, other->stops );
	DArray_Assign( self->colors, other->colors );
}

DaoxColor DaoxColor_Interpolate( DaoxColor C1, DaoxColor C2, float start, float mid, float end )
{
	DaoxColor color;
	float at = (mid - start) / (end - start + 1E-9);
	color.red = (1.0 - at) * C1.red + at * C2.red;
	color.green = (1.0 - at) * C1.green + at * C2.green;
	color.blue = (1.0 - at) * C1.blue + at * C2.blue;
	color.alpha = (1.0 - at) * C1.alpha + at * C2.alpha;
	return color;
}
DaoxColor DaoxGradient_InterpolateColor( DaoxGradient *self, float at )
{
	int i, n = self->stops->size;
	float start, end, *stops = self->stops->data.floats;
	DaoxColor *colors = self->colors->data.colors;
	DaoxColor color = {0.0,0.0,0.0,0.0};

	if( self->stops->size ) color = self->colors->data.colors[0];
	if( self->stops->size <= 1 ) return color;
	if( at < 0.0 ) at = 0.0;
	if( at > 1.0 ) at = 1.0;
	if( at < stops[0] || at > stops[n-1] ){
		start = stops[n-1];
		end = stops[0] + 1.0;
		if( at < stops[0] ) at += 1.0;
		return DaoxColor_Interpolate( colors[n-1], colors[0], start, at, end );
	}
	for(i=1; i<n; ++i){
		float start = stops[i-1];
		float end = stops[i];
		if( at >= start && at <= end ){
			color = DaoxColor_Interpolate( colors[i-1], colors[i], start, at, end );
			break;
		}
	}
	return color;
}
DaoxColor DaoxGradient_ComputeLinearColor( DaoxGradient *self, DaoxVector2D point )
{
	DaoxVector2D A = self->points[0];
	DaoxVector2D B = self->points[1];
	DaoxVector2D C = point;
	float BxAx = B.x - A.x;
	float ByAy = B.y - A.y;
	float CxAx = C.x - A.x;
	float CyAy = C.y - A.y;
	float t = (CxAx * BxAx + CyAy * ByAy) / (BxAx * BxAx + ByAy * ByAy);
	return DaoxGradient_InterpolateColor( self, t );
}
DaoxColor DaoxGradient_ComputeRadialColor( DaoxGradient *self, DaoxVector2D point )
{
	DaoxVector2D C = self->points[0];
	DaoxVector2D F = self->points[1];
	DaoxVector2D G = point;
	float R = self->radius;
	float GxFx = G.x - F.x;
	float GyFy = G.y - F.y;
	float FxCx = F.x - C.x;
	float FyCy = F.y - C.y;
	float a = GxFx * GxFx + GyFy * GyFy;
	float b = 2.0 * (GxFx * FxCx + GyFy * FyCy);
	float c = FxCx * FxCx + FyCy * FyCy - R * R;
	float t = (- b + sqrt(b * b - 4.0 * a * c)) / (2.0 * a);
	if( t < 1.0 ){
		t = 1.0;
	}else{
		t = 1.0 / t;
	}
	return DaoxGradient_InterpolateColor( self, t );
}
DaoxColor DaoxGradient_ComputeColor( DaoxGradient *self, DaoxVector2D point )
{
	DaoxColor color = {0.0,0.0,0.0,0.0};
	switch( self->gradient ){
	case DAOX_GRADIENT_LINEAR : return DaoxGradient_ComputeLinearColor( self, point );
	case DAOX_GRADIENT_RADIAL : return DaoxGradient_ComputeRadialColor( self, point );
	}
	return color;
}






DaoxBrush* DaoxBrush_New( DaoVmSpace *vmspace )
{
	DaoxBrush *self = (DaoxBrush*) dao_calloc(1,sizeof(DaoxBrush));
	DaoCstruct_Init( (DaoCstruct*)self, DaoVmSpace_GetType( vmspace, & daoBrushCore ) );
	DaoxPathStyle_Init( & self->strokeStyle );
	self->strokeColor.alpha = 1.0;
	self->fontSize = 12.0;
	return self;
}
void DaoxBrush_Delete( DaoxBrush *self )
{
	if( self->strokeGradient ) DaoGC_DecRC( (DaoValue*) self->strokeGradient );
	if( self->fillGradient ) DaoGC_DecRC( (DaoValue*) self->fillGradient );
	if( self->image ) DaoGC_DecRC( (DaoValue*) self->image );
	if( self->font ) DaoGC_DecRC( (DaoValue*) self->font );
	DaoCstruct_Free( (DaoCstruct*) self );
	dao_free( self );
}
void DaoxBrush_Copy( DaoxBrush *self, DaoxBrush *other )
{
	self->strokeStyle = other->strokeStyle;
	self->fontSize = other->fontSize;
	self->strokeColor = other->strokeColor;
	self->fillColor = other->fillColor;
	GC_Assign( & self->font, other->font );
	GC_Assign( & self->image, other->image );
	if( other->strokeGradient ){
		self->strokeGradient = DaoxGradient_New( DaoType_GetVmSpace( self->ctype ), 0 );
		DaoxGradient_Copy( self->strokeGradient, other->strokeGradient );
		GC_IncRC( self->strokeGradient );
	}
	if( other->fillGradient ){
		self->fillGradient = DaoxGradient_New( DaoType_GetVmSpace( self->ctype ), 0 );
		DaoxGradient_Copy( self->fillGradient, other->fillGradient );
		GC_IncRC( self->fillGradient );
	}
}
void DaoxBrush_SetStrokeWidth( DaoxBrush *self, float width )
{
	self->strokeStyle.width = width;
}
void DaoxBrush_SetStrokeColor( DaoxBrush *self, DaoxColor color )
{
	self->strokeColor = color;
}
void DaoxBrush_SetFillColor( DaoxBrush *self, DaoxColor color )
{
	self->fillColor = color;
}
void DaoxBrush_SetDashPattern( DaoxBrush *self, float pat[], int n )
{
	DaoxPathStyle_SetDashes( & self->strokeStyle, n, pat );
}
void DaoxBrush_SetFont( DaoxBrush *self, DaoxFont *font, float size )
{
	GC_Assign( & self->font, font );
	self->fontSize = size;
}




void DaoxCanvasNode_ResetTransform( DaoxCanvasNode *self )
{
	self->rotation = DaoxVector2D_XY( 1.0, 0.0 );
	self->translation = DaoxVector2D_XY( 0.0, 0.0 );
}
void DaoxCanvasNode_Init( DaoxCanvasNode *self, DaoType *type )
{
	memset( self, 0, sizeof(DaoxCanvasNode) );
	DaoCstruct_Init( (DaoCstruct*)self, type );
	self->scale = 1.0;
	self->visible = 1;
	self->changed = 1;
	self->moved = 1;
	DaoxCanvasNode_ResetTransform( self );
}
void DaoxCanvasNode_Free( DaoxCanvasNode *self )
{
	if( self->children ) DList_Delete( self->children );
	if( self->path ) DaoGC_DecRC( (DaoValue*) self->path );
	if( self->mesh ) DaoGC_DecRC( (DaoValue*) self->mesh );
	DaoGC_DecRC( (DaoValue*) self->parent );
	DaoGC_DecRC( (DaoValue*) self->brush );
	DaoCstruct_Free( (DaoCstruct*) self );
}



DaoxCanvasNode* DaoxCanvasNode_New( DaoType *type )
{
	DaoxCanvasNode *self = (DaoxCanvasNode*) dao_calloc( 1, sizeof(DaoxCanvasNode) );
	DaoxCanvasNode_Init( self, type );
	return self;
}
void DaoxCanvasNode_Delete( DaoxCanvasNode *self )
{
	DaoxCanvasNode_Free( self );
	dao_free( self );
}
void DaoxCanvasNode_MarkDataChanged( DaoxCanvasNode *self )
{
	self->changed = 1;
	self->moved = 1;
	if( self->parent ) DaoxCanvasNode_MarkStateChanged( self->parent );
}
void DaoxCanvasNode_MarkStateChanged( DaoxCanvasNode *self )
{
	self->moved = 1;
	if( self->parent ) DaoxCanvasNode_MarkStateChanged( self->parent );
}
DaoxMatrix3D DaoxCanvasNode_GetLocalTransform( DaoxCanvasNode *self )
{
	DaoxMatrix3D transform;
	transform.A11 =   self->rotation.x;
	transform.A12 = - self->rotation.y;
	transform.A21 =   self->rotation.y;
	transform.A22 =   self->rotation.x;
	transform.B1 = self->translation.x;
	transform.B2 = self->translation.y;
	return transform;
}
static void DArray_PushOBBoxVertexPoints2D( DArray *self, DaoxOBBox2D *box )
{
	DaoxVector2D dY = DaoxVector2D_Sub( & box->Y, & box->O );
	DaoxVector2D P = DaoxVector2D_Add( & box->X, & dY );

	DArray_Reserve( self, self->size + 4 );
	DArray_PushVector2D( self, & box->O );
	DArray_PushVector2D( self, & box->X );
	DArray_PushVector2D( self, & box->Y );
	DArray_PushVector2D( self, & P );
}
void DaoxCanvasNode_Update( DaoxCanvasNode *self, DaoxCanvas *canvas )
{
	DaoVmSpace *vmspace = DaoType_GetVmSpace( self->ctype );
	DArray *points;
	daoint i;

	if( self->moved == 0 && self->changed == 0 ) return;
	if( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasImageCore ) ) return;

	if( self->path != NULL && (self->mesh == NULL || self->changed) ){
		DaoxBrush *brush = self->brush;
		DaoxPathStyle style = self->brush->strokeStyle;
		DaoxPathMesh *mesh;

		style.fill = brush->fillColor.alpha > EPSILON || brush->fillGradient != NULL;
		style.width = style.width / (self->scale + EPSILON);
		mesh = DaoxPathCache_FindMesh( canvas->pathCache, self->path, & style );
		GC_Assign( & self->mesh, mesh );
		self->changed = 0;
	}
	self->moved = 0;

	points = DArray_New( sizeof(DaoxVector2D) );
	if( self->path && self->mesh ){
		DaoxOBBox2D obbox = DaoxOBBox2D_Scale( & self->mesh->path->obbox, self->scale );
		float strokeWidth = self->brush ? self->brush->strokeStyle.width : 0;
		if( strokeWidth > 1E-9 ){
			strokeWidth *= self->scale + EPSILON;
			obbox = DaoxOBBox2D_CopyWithMargin( & obbox, 0.5*strokeWidth );
		}
		DArray_PushOBBoxVertexPoints2D( points, & obbox );
	}
	if( self->children ){
		for(i=0; i<self->children->size; ++i){
			DaoxCanvasNode *it = self->children->items.pCanvasNode[i];
			DaoxCanvasNode_Update( it, canvas );
			DArray_PushOBBoxVertexPoints2D( points, & it->obbox );
		}
	}
	DaoxOBBox2D_ResetBox( & self->obbox, points->data.vectors2d, points->size );
	DArray_Delete( points );
}




DaoxCanvasLine* DaoxCanvasLine_New( DaoVmSpace *vmspace )
{
	return DaoxCanvasNode_New( DaoVmSpace_GetType( vmspace, & daoCanvasLineCore ) );
}

DaoxCanvasRect* DaoxCanvasRect_New( DaoVmSpace *vmspace )
{
	return DaoxCanvasNode_New( DaoVmSpace_GetType( vmspace, & daoCanvasRectCore ) );
}

DaoxCanvasCircle* DaoxCanvasCircle_New( DaoVmSpace *vmspace )
{
	return DaoxCanvasNode_New( DaoVmSpace_GetType( vmspace, & daoCanvasCircleCore ) );
}

DaoxCanvasEllipse* DaoxCanvasEllipse_New( DaoVmSpace *vmspace )
{
	return DaoxCanvasNode_New( DaoVmSpace_GetType( vmspace, & daoCanvasEllipseCore ) );
}

DaoxCanvasPath* DaoxCanvasPath_New( DaoVmSpace *vmspace )
{
	DaoxCanvasPath *self = (DaoxCanvasPath*)dao_calloc( 1, sizeof(DaoxCanvasPath));
	DaoxCanvasNode_Init( self,  DaoVmSpace_GetType( vmspace, & daoCanvasPathCore ) );
	return self;
}

DaoxCanvasImage* DaoxCanvasImage_New( DaoVmSpace *vmspace )
{
	DaoxCanvasImage *self = (DaoxCanvasImage*)dao_calloc( 1, sizeof(DaoxCanvasImage));
	DaoxCanvasNode_Init( self,  DaoVmSpace_GetType( vmspace, & daoCanvasImageCore ) );
	return self;
}

DaoxCanvasText* DaoxCanvasText_New( DaoVmSpace *vmspace )
{
	DaoxCanvasText *self = (DaoxCanvasText*) dao_calloc( 1, sizeof(DaoxCanvasText) );
	DaoxCanvasNode_Init( self, DaoVmSpace_GetType( vmspace, & daoCanvasTextCore ) );
	return self;
}




void DaoxCanvasLine_Set( DaoxCanvasLine *self, float x1, float y1, float x2, float y2 )
{
	DaoVmSpace *vmspace = DaoType_GetVmSpace( self->ctype );
	float dx = x2 - x1;
	float dy = y2 - y1;
	float r = sqrt( dx*dx + dy*dy );
	float cosine = dx / (r + EPSILON);
	float sine = dy / (r + EPSILON);

	assert( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasLineCore ) );
	self->scale = r / DAOX_PATH_UNIT;
	self->rotation.x = cosine;
	self->rotation.y = sine;
	self->translation.x = x1;
	self->translation.y = y1;
	self->changed = 1;
}

void DaoxCanvasRect_Set( DaoxCanvasRect *self, float x1, float y1, float x2, float y2 )
{
	DaoVmSpace *vmspace = DaoType_GetVmSpace( self->ctype );
	float w = fabs( x2 - x1 );
	float h = fabs( y2 - y1 );
	assert( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasRectCore ) );
	DaoxCanvasNode_ResetTransform( self );
	self->translation.x = x1;
	self->translation.y = y1;
	self->changed = 1;
	self->scale = w / DAOX_PATH_UNIT;
}

void DaoxCanvasCircle_Set( DaoxCanvasCircle *self, float x, float y, float r )
{
	DaoVmSpace *vmspace = DaoType_GetVmSpace( self->ctype );
	assert( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasCircleCore ) );
	self->scale = r / DAOX_PATH_UNIT;
	DaoxCanvasNode_ResetTransform( self );
	self->translation.x = x;
	self->translation.y = y;
	self->changed = 1;
}

void DaoxCanvasEllipse_Set( DaoxCanvasEllipse *self, float x, float y, float rx, float ry )
{
	DaoVmSpace *vmspace = DaoType_GetVmSpace( self->ctype );
	assert( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasEllipseCore ) );
	self->scale = rx / DAOX_PATH_UNIT;
	DaoxCanvasNode_ResetTransform( self );
	self->translation.x = x;
	self->translation.y = y;
	self->changed = 1;
}




#if 0
void DaoxCanvasPath_SetRelativeMode( DaoxCanvasPath *self, int relative )
{
	DaoxPath_SetRelativeMode( self->path, relative );
}
void DaoxCanvasPath_MoveTo( DaoxCanvasPath *self, float x, float y )
{
	assert( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasPathCore ) );
	DaoxPath_MoveTo( self->path, x, y );
}
void DaoxCanvasPath_LineTo( DaoxCanvasPath *self, float x, float y )
{
	assert( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasPathCore ) );
	DaoxPath_LineTo( self->path, x, y );
}
void DaoxCanvasPath_ArcTo( DaoxCanvasPath *self, float x, float y, float degrees )
{
	assert( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasPathCore ) );
	DaoxPath_ArcTo( self->path, x, y, degrees );
}
void DaoxCanvasPath_ArcBy( DaoxCanvasPath *self, float cx, float cy, float degrees )
{
	assert( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasPathCore ) );
	DaoxPath_ArcBy( self->path, cx, cy, degrees );
}
void DaoxCanvasPath_QuadTo( DaoxCanvasPath *self, float cx, float cy, float x, float y )
{
	assert( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasPathCore ) );
	DaoxPath_QuadTo( self->path, cx, cy, x, y );
}
void DaoxCanvasPath_CubicTo( DaoxCanvasPath *self, float cx, float cy, float x, float y )
{
	assert( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasPathCore ) );
	DaoxPath_CubicTo( self->path, cx, cy, x, y );
}
void DaoxCanvasPath_CubicTo2( DaoxCanvasLine *self, float cx0, float cy0, float cx, float cy, float x, float y )
{
	assert( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasPathCore ) );
	DaoxPath_CubicTo2( self->path, cx0, cy0, cx, cy, x, y );
}
void DaoxCanvasPath_Close( DaoxCanvasPath *self )
{
	assert( self->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasPathCore ) );
	DaoxPath_Close( self->path );
}

#endif







DaoxCanvas* DaoxCanvas_New( DaoVmSpace *vmspace )
{
	DaoxCanvas *self = (DaoxCanvas*) dao_calloc( 1, sizeof(DaoxCanvas) );
	DaoCstruct_Init( (DaoCstruct*)self, DaoVmSpace_GetType( vmspace, & daoCanvasCore ) );
	self->transform = DaoxMatrix3D_Identity();
	self->brushes = DList_New( DAO_DATA_VALUE );
	self->nodes = DList_New( DAO_DATA_VALUE );
	self->actives = DList_New(0); /* No GC, item GC handled by ::nodes; */

	self->auxPath = DaoxPath_New( vmspace );
	DaoGC_IncRC( (DaoValue*) self->auxPath );

	self->pathCache = DaoxPathCache_New( vmspace );
	DaoGC_IncRC( (DaoValue*) self->pathCache );

	return self;
}
void DaoxCanvas_Delete( DaoxCanvas *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	DList_Delete( self->nodes );
	DList_Delete( self->actives );
	DList_Delete( self->brushes );
	GC_DecRC( self->pathCache );
	GC_DecRC( self->auxPath );
	dao_free( self );
}


void DaoxCanvas_SetViewport( DaoxCanvas *self, float left, float right, float bottom, float top )
{
	self->viewport.left = left;
	self->viewport.right = right;
	self->viewport.bottom = bottom;
	self->viewport.top = top;
}
void DaoxCanvas_SetBackground( DaoxCanvas *self, DaoxColor color )
{
	self->background = color;
}
DaoxBrush* DaoxCanvas_GetCurrentBrush( DaoxCanvas *self )
{
	if( self->brushes->size == 0 ) return NULL;
	return self->brushes->items.pBrush[self->brushes->size-1];
}
DaoxBrush* DaoxCanvas_GetOrPushBrush( DaoxCanvas *self )
{
	if( self->brushes->size == 0 ) DaoxCanvas_PushBrush( self, 0 );
	return DaoxCanvas_GetCurrentBrush( self );
}
DaoxBrush* DaoxCanvas_PushBrush( DaoxCanvas *self, int index )
{
	DaoxBrush *prev = DaoxCanvas_GetCurrentBrush( self );
	DaoxBrush *brush = NULL;
	if( index >=0 && index < self->brushes->size ){
		brush = (DaoxBrush*) self->brushes->items.pVoid[index];
	}else{
		brush = DaoxBrush_New( DaoType_GetVmSpace( self->ctype ) );
		if( prev ) DaoxBrush_Copy( brush, prev );
	}
	DList_PushBack( self->brushes, brush );
	return brush;
}
void DaoxCanvas_PopBrush( DaoxCanvas *self )
{
	DList_PopBack( self->brushes );
}


void DaoxCanvas_UpdatePathMesh( DaoxCanvas *self, DaoxCanvasNode *node )
{
	DaoxPathMesh *mesh;
	DaoxBrush *brush = node->brush;
	DaoxPathStyle style = node->brush->strokeStyle;

	style.fill = brush->fillColor.alpha > EPSILON || brush->fillGradient != NULL;
	style.width = style.width / (node->scale + EPSILON);
	if( node->path == NULL ) return;

	mesh = DaoxPathCache_FindMesh( self->pathCache, node->path, & style );
	GC_Assign( & node->mesh, mesh );
}
void DaoxCanvas_AddNode( DaoxCanvas *self, DaoxCanvasNode *node )
{
	DaoxBrush *brush = node->brush;
	
	if( brush == NULL ){
		brush = DaoxCanvas_GetOrPushBrush( self );
		GC_Assign( & node->brush, brush );
	}
	DaoxCanvas_UpdatePathMesh( self, node );

	if( self->actives->size ){
		DaoxCanvasNode *activeNode = (DaoxCanvasNode*) DList_Back( self->actives );
		if( activeNode->children == NULL ) activeNode->children = DList_New( DAO_DATA_VALUE );
		GC_Assign( & node->parent, activeNode );
		DList_PushBack( activeNode->children, node );
		DaoxCanvasNode_MarkDataChanged( activeNode );
	}else{
		DList_PushBack( self->nodes, node );
	}
}

DaoxCanvasNode* DaoxCanvas_AddGroup( DaoxCanvas *self )
{
	DaoVmSpace *vmspace = DaoType_GetVmSpace( self->ctype );
	DaoxCanvasNode *node = DaoxCanvasNode_New( DaoVmSpace_GetType( vmspace, & daoCanvasNodeCore ) );
	DaoxCanvas_AddNode( self, node );
	DList_PushBack( self->actives, node );
	return node;
}

DaoxCanvasLine* DaoxCanvas_AddLine( DaoxCanvas *self, float x1, float y1, float x2, float y2 )
{
	DaoxCanvasLine *node = DaoxCanvasLine_New( DaoType_GetVmSpace( self->ctype ) );
	DaoxCanvasLine_Set( node, x1, y1, x2, y2 );
	DaoxCanvas_AddNode( self, node );
	DaoGC_IncRC( (DaoValue*) self->pathCache->unitLine );
	node->path = self->pathCache->unitLine;
	return node;
}

DaoxCanvasRect* DaoxCanvas_AddRect( DaoxCanvas *self, float x1, float y1, float x2, float y2, float rx, float ry )
{
	float w = fabs(x2 - x1);
	float h = fabs(y2 - y1);
	float r = h / (w + EPSILON);
	float rx2 = fabs(rx) / (w + EPSILON);
	float ry2 = fabs(ry) / (h + EPSILON);
	DaoxCanvasRect *node = DaoxCanvasRect_New( DaoType_GetVmSpace( self->ctype ) );
	DaoxPath *path = self->auxPath;

	DaoxPath_Reset( path );
	DaoxPath_SetRelativeMode( path, 1 );
	if( fabs( rx2 ) > 1E-3 && fabs( ry2 ) > 1E-3 ){
		r *= DAOX_PATH_UNIT;
		ry2 *= r;
		rx2 *= DAOX_PATH_UNIT;
		DaoxPath_MoveTo( path, 0.0, ry2 );
		DaoxPath_CubicTo2( path, 0.0, -0.55*ry2, -0.55*rx2, 0.0, rx2, -ry2 );
		DaoxPath_LineTo( path, DAOX_PATH_UNIT - 2.0*rx2, 0.0 );
		DaoxPath_CubicTo2( path, 0.55*rx2, 0.0, 0.0, -0.55*ry2, rx2, ry2 );
		DaoxPath_LineTo( path, 0.0, r - 2.0*ry2 );
		DaoxPath_CubicTo2( path, 0.0, 0.55*ry2, 0.55*rx2, 0.0, -rx2, ry2 );
		DaoxPath_LineTo( path, -(DAOX_PATH_UNIT - 2.0*rx2), 0.0 );
		DaoxPath_CubicTo2( path, -0.55*rx2, 0.0, 0.0, 0.55*ry2, -rx2, -ry2 );
	}else{
		DaoxPath_MoveTo( path, 0.0, 0.0 );
		DaoxPath_LineTo( path, DAOX_PATH_UNIT, 0.0 );
		DaoxPath_LineTo( path, 0.0, r*DAOX_PATH_UNIT );
		DaoxPath_LineTo( path, -DAOX_PATH_UNIT, 0.0 );
	}
	DaoxPath_Close( path );

	node->path = DaoxPathCache_FindPath( self->pathCache, path );
	DaoGC_IncRC( (DaoValue*) node->path );

	DaoxCanvasRect_Set( node, x1, y1, x2, y2 );
	DaoxCanvas_AddNode( self, node );
	return node;
}

DaoxCanvasCircle* DaoxCanvas_AddCircle( DaoxCanvas *self, float x, float y, float r )
{
	DaoxCanvasCircle *node = DaoxCanvasCircle_New( DaoType_GetVmSpace( self->ctype ) );
	DaoxPath *circle = self->pathCache->unitCircle3;

	if( r < 8.0 ){
		circle = self->pathCache->unitCircle1;
	}else if( r < 32.0 ){
		circle = self->pathCache->unitCircle2;
	}
	DaoGC_IncRC( (DaoValue*) circle );
	node->path = circle;

	DaoxCanvasCircle_Set( node, x, y, r );
	DaoxCanvas_AddNode( self, node );
	return node;
}

DaoxCanvasEllipse* DaoxCanvas_AddEllipse( DaoxCanvas *self, float x, float y, float rx, float ry )
{
	DaoxPath *path = self->auxPath;
	DaoxCanvasEllipse *node = DaoxCanvasEllipse_New( DaoType_GetVmSpace( self->ctype ) );
	DaoxMatrix3D mat = DaoxMatrix3D_Identity();

	mat.A22 = ry / (rx + EPSILON);
	DaoxPath_Reset( path );
	DaoxPath_ImportPath( path, self->pathCache->unitCircle3, & mat );

	node->path = DaoxPathCache_FindPath( self->pathCache, path );
	DaoGC_IncRC( (DaoValue*) node->path );

	DaoxCanvasEllipse_Set( node, x, y, rx, ry );
	DaoxCanvas_AddNode( self, node );
	return node;
}


DaoxCanvasPath* DaoxCanvas_AddPath( DaoxCanvas *self, DaoxPath *path )
{
	DaoVmSpace *vmspace = DaoType_GetVmSpace( self->ctype );
	DaoxCanvasPath *node = DaoxCanvasPath_New( vmspace );
	if( path == NULL ) path = DaoxPath_New( vmspace );
	DaoGC_IncRC( (DaoValue*) path );
	node->path = path;
	DaoxCanvas_AddNode( self, node );
	return node;
}
void DaoxCanvas_AddCharNodes( DaoxCanvas *self, DaoxCanvasText *textItem, DArray *text, float x, float y, float degrees )
{
	DaoxGlyph *glyph;
	DaoxBrush *brush;
	DaoxCanvasText *chnode;
	DaoxPath *textPath = textItem->path;
	DaoxFont *font = textItem->brush->font;
	DaoxVector2D rotation = {1.0, 0.0};
	DaoxVector2D charRotation = {1.0, 0.0};
	DaoxVector2D charTranslation = {0.0, 0.0};
	double width = textItem->brush->strokeStyle.width;
	double size = textItem->brush->fontSize;
	double scale = size / (float)font->fontHeight;
	double offset, advance, angle = degrees * M_PI / 180.0;
	daoint i;

	if( text->stride != 4 ) return;

	rotation.x = cos( angle );
	rotation.y = sin( angle );
	charRotation = rotation;

	DList_PushBack( self->actives, textItem );

	offset = x;
	for(i=0; i<text->size; ++i){
		DaoxAABBox2D bounds = {0.0,0.0,0.0,0.0};
		DaoxMatrix3D rotmat = {0.0,0.0,0.0,0.0,0.0,0.0};
		size_t ch = text->data.uints[i];
		glyph = DaoxFont_GetGlyph( font, ch );
		if( glyph == NULL ) break;

		rotmat.A11 = rotmat.A22 = rotation.x;
		rotmat.A12 = - rotation.y;
		rotmat.A21 = rotation.y;
		bounds.right = glyph->advanceWidth;
		bounds.top = font->lineSpace;
		bounds = DaoxAABBox2D_Transform( & bounds, & rotmat );
		advance = bounds.right - bounds.left;

		if( textItem->children == NULL ) textItem->children = DList_New( DAO_DATA_VALUE );

		chnode = DaoxCanvasPath_New( DaoType_GetVmSpace( self->ctype ) );
		chnode->path = DaoxPathCache_FindPath( self->pathCache, glyph->shape );
		chnode->scale = scale;
		DaoGC_IncRC( (DaoValue*) chnode->path );
		DaoxCanvas_AddNode( self, chnode );
		
		if( textPath ){
			float adv = 0.99 * (scale * advance + width);
			DaoxVector3D pos1 = {0.0,0.0,0.0};
			DaoxVector3D pos2 = {0.0,0.0,0.0};
			DaoxPathSegment *res1 = DaoxPath_LocateByDistance( textPath, offset, & pos1 );
			DaoxPathSegment *res2 = DaoxPath_LocateByDistance( textPath, offset+adv, & pos2 );

			if( pos1.z > -EPSILON ){
				float dx = pos2.x - pos1.x;
				float dy = pos2.y - pos1.y;
				float r = sqrt( dx*dx + dy*dy );
				float cos1 = dx / r;
				float sin1 = dy / r;
				float cos2 = rotation.x;
				float sin2 = rotation.y;
				charRotation.x = cos1 * cos2 - sin1 * sin2;
				charRotation.y = sin1 * cos2 + cos1 * sin2;
				charTranslation.x = pos1.x;
				charTranslation.y = pos1.y;
			}else{
				charTranslation.x += scale * advance + width;
			}
		}else{
			charTranslation.x = offset;
			charTranslation.y = y;
		}
		chnode->rotation = charRotation;
		chnode->translation = charTranslation;
		offset += scale * advance + width;
		chnode = NULL;
	}
	DList_PopBack( self->actives );
}
DaoxCanvasText* DaoxCanvas_AddText( DaoxCanvas *self, const char *text, float x, float y, float degrees )
{
	DaoxCanvasPath *node;
	DaoxBrush *brush;
	DString str = DString_WrapChars( text );
	DArray *codepoints;

	if( self->brushes->size == 0 ) return NULL;
	brush = DaoxCanvas_GetOrPushBrush( self );
	if( brush->font == NULL ) GC_Assign( & brush->font, DaoxFont_GetDefault() );

	node = DaoxCanvasText_New( DaoType_GetVmSpace( self->ctype ) );
	DaoxCanvas_AddNode( self, node );
	codepoints = DArray_New( sizeof(uint_t) );
	DString_DecodeUTF8( & str, codepoints );
	DaoxCanvas_AddCharNodes( self, node, codepoints, x, y, degrees );
	DArray_Delete( codepoints );
	return node;
}
DaoxCanvasText* DaoxCanvas_AddPathText( DaoxCanvas *self, const char *text, DaoxPath *path, float degrees )
{
	DaoxCanvasPath *node;
	DaoxBrush *brush;
	DString str = DString_WrapChars( text );
	DArray *codepoints;

	if( self->brushes->size == 0 ) return NULL;
	brush = DaoxCanvas_GetOrPushBrush( self );
	if( brush->font == NULL ) return NULL;

	node = DaoxCanvasText_New( DaoType_GetVmSpace( self->ctype ) );
	DaoxCanvas_AddNode( self, node );

	DaoxPath_Refine( path, 0.02*brush->fontSize, 0.02 );
	GC_Assign( & node->path, path );
	node->visible = 0;

	codepoints = DArray_New( sizeof(uint_t) );
	DString_DecodeUTF8( & str, codepoints );
	DaoxCanvas_AddCharNodes( self, node, codepoints, 0, 0, degrees );
	DArray_Delete( codepoints );
	return node;
}
DaoxCanvasImage* DaoxCanvas_AddImage( DaoxCanvas *self, DaoImage *image, float x, float y, float w )
{
	DaoxCanvasPath *node = DaoxCanvasImage_New( DaoType_GetVmSpace( self->ctype ) );
	DaoxBrush *brush = DaoxCanvas_GetOrPushBrush( self );

	GC_Assign( & brush->image, image );
	GC_Assign( & node->brush, brush );

	node->scale = w / image->width;
	node->translation.x = x;
	node->translation.y = y;
	node->obbox.O.x = node->obbox.O.y = 0;
	node->obbox.X.x = node->obbox.X.y = 0;
	node->obbox.Y.x = node->obbox.Y.y = 0;
	node->obbox.X.x = image->width;
	node->obbox.Y.y = image->height;

	DaoxCanvas_AddNode( self, node );
	return node;
}



