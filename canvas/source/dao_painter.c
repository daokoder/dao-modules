/*
// Dao Canvas Module
// http://www.daovm.net
//
// Copyright (c) 2015-2017, Limin Fu
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
#include "dao_painter.h"

DaoxPainter* DaoxPainter_New( DaoVmSpace *vmspace )
{
	DaoxPainter *self = (DaoxPainter*)dao_calloc(1,sizeof(DaoxPainter));
	DaoCstruct_Init( (DaoCstruct*) self, DaoVmSpace_GetType( vmspace, & daoPainterCore ) );
	self->gradient = DaoxGradient_New( vmspace, DAOX_GRADIENT_BASE );
	self->buffer = _DaoImage_New( _DaoImage_Type( vmspace ) );
	self->renderer = DaoxRenderer_New( self->buffer );
	self->rasterizer = DaoxRasterizer_New();
	self->rasterizer->filling_rule = DAOX_FILL_EVEN_ODD;
	self->target = self->buffer;
	DaoxRasterizer_SetGamma( self->rasterizer, 1.3);
	DaoGC_IncRC( (DaoValue*) self->gradient );
	DaoGC_IncRC( (DaoValue*) self->buffer );
	return self;
}
void DaoxPainter_Delete( DaoxPainter *self )
{
	DaoCstruct_Free( (DaoCstruct*) self );
	DaoxRasterizer_Delete( self->rasterizer );
	DaoxRenderer_Delete( self->renderer );
	DaoGC_DecRC( (DaoValue*) self->gradient );
	DaoGC_DecRC( (DaoValue*) self->buffer );
	dao_free( self );
}


float DaoxPainter_CanvasScale( DaoxPainter *self, DaoxCanvas *canvas )
{
	DaoxAABBox2D box = canvas->viewport;
	float xscale = fabs( box.right - box.left ) / (self->target->width + 1);
	float yscale = fabs( box.top - box.bottom ) / (self->target->height + 1);
	return 0.5 * (xscale + yscale);
}


void DaoxPainter_BufferPath( DaoxPainter *self, DaoxPath *path, DaoxMatrix3D *trans )
{
	DaoxPathComponent *com;
	DaoxPathSegment *seg;
	DaoxVector2D P1, P2;

	for(com=path->first; com; com=com->next){
		DaoxPathSegment *begin = com->refinedFirst ? com->refinedFirst : com->first;
		if( com->first->bezier == 0 ) continue;
		seg = begin;
		P1 = DaoxMatrix3D_Transform( trans, seg->P1 );
		DaoxRasterizer_MoveToD( self->rasterizer, P1.x, P1.y );
		do {
			P2 = DaoxMatrix3D_Transform( trans, seg->P2 );
			DaoxRasterizer_LineToD( self->rasterizer, P2.x, P2.y );
			seg = seg->next;
		} while( seg && seg != begin );
	}
}
static void DaoxGradient_Transform( DaoxGradient *self, DaoxMatrix3D *trans )
{
	self->points[0] = DaoxMatrix3D_Transform( trans, self->points[0] );
	self->points[1] = DaoxMatrix3D_Transform( trans, self->points[1] );
}

void DaoxPainter_PaintItemData( DaoxPainter *self, DaoxCanvas *canvas, DaoxCanvasNode *item, DaoxMatrix3D transform )
{
	DaoxBrush *brush = item->brush;
	DaoxPathMesh *mesh = item->mesh;
	DaoxPixBrush pixBrush = {daox_black_color, NULL, NULL};
	DaoxMatrix3D gradtrans = transform;
	float resolution = DaoxPainter_CanvasScale( self, canvas );
	float scale = item->scale;
	float stroke = brush->strokeStyle.width / (resolution + 1E-16);
	int i, fill = brush->fillColor.alpha > EPSILON || brush->fillGradient != NULL;

	transform.A11 *= scale;
	transform.A22 *= scale;
	transform.A12 *= scale;
	transform.A21 *= scale;
	if( fill ){
		pixBrush.color = brush->fillColor;
		pixBrush.gradient = NULL;
		if( brush->fillGradient ){
			pixBrush.gradient = self->gradient;
			DaoxGradient_Copy( self->gradient, brush->fillGradient );
			DaoxGradient_Transform( self->gradient, & gradtrans );
		}
		DaoxPainter_BufferPath( self, mesh->path, & transform );
		DaoxRasterizer_Render( self->rasterizer, self->renderer, & pixBrush );
	}

	if( stroke ){
		pixBrush.color = brush->strokeColor;
		pixBrush.gradient = NULL;
		if( brush->strokeGradient ){
			pixBrush.gradient = self->gradient;
			DaoxGradient_Copy( self->gradient, brush->strokeGradient );
			DaoxGradient_Transform( self->gradient, & gradtrans );
		}
		DaoxPainter_BufferPath( self, mesh->stroke, & transform );
		DaoxRasterizer_Render( self->rasterizer, self->renderer, & pixBrush );
	}
}

void DaoxPainter_PaintImageItem( DaoxPainter *self, DaoxCanvasNode *item, DaoxMatrix3D transform )
{

	if( item->brush->image == NULL ) return;

}

void DaoxPainter_PaintItem( DaoxPainter *self, DaoxCanvas *canvas, DaoxCanvasNode *item, DaoxMatrix3D transform )
{
	DaoxOBBox2D obbox;
	DaoxMatrix3D inverse;
	DaoxVector3D itempos = {0.0,0.0,0.0};
	DaoxMatrix3D transform2 = DaoxCanvasNode_GetLocalTransform( item );
	DaoVmSpace *vmspace = DaoType_GetVmSpace( self->ctype );
	float diameter;
	float scale = DaoxPainter_CanvasScale( self, canvas );
	float stroke = item->brush->strokeStyle.width / (scale + 1E-16);
	int n = item->children ? item->children->size : 0;
	int k = stroke >= 1.0;
	int m = stroke >= 1E-3;
	int i, triangles;

	DaoxCanvasNode_Update( item, canvas );

	DaoxMatrix3D_Multiply( & transform, transform2 );
	obbox = DaoxOBBox2D_Transform( & item->obbox, & transform );
	itempos.x = obbox.O.x;
	itempos.y = obbox.O.y;
	diameter = DaoxVector2D_Dist( obbox.X, obbox.Y );
	//printf( "DaoxPainter_PaintItem 1: %s %g %g\n", item->ctype->name->chars, diameter, distance );

#if 0
	if( diameter < 1E-5 ) goto HandleChildrenItems;
	if( DaoxOBBox2D_Intersect( & self->obbox, & obbox ) < 0 ) goto HandleChildrenItems;
#endif

	if( item->visible ){
		if( item->ctype == DaoVmSpace_GetType( vmspace, & daoCanvasImageCore ) ){
			DaoxPainter_PaintImageItem( self, item, transform );
		}else if( item->path ){
			DaoxPainter_PaintItemData( self, canvas, item, transform );
		}
	}

HandleChildrenItems:
	for(i=0; i<n; i++){
		DaoxCanvasNode *it = item->children->items.pCanvasNode[i];
		DaoxPainter_PaintItem( self, canvas, it, transform );
	}
}

void DaoxPainter_Paint( DaoxPainter *self, DaoxCanvas *canvas, DaoxAABBox2D viewport )
{
	DaoxColor bgcolor = canvas->background;
	float CX = 0.5*(viewport.left + viewport.right);
	float CY = 0.5*(viewport.top + viewport.bottom);
	float W = viewport.right - viewport.left;
	float H = viewport.top - viewport.bottom;
	int i, n = canvas->nodes->size;

	_DaoImage_Resize( self->target, W, H );

	if( bgcolor.alpha >= 1.0/255.0 ) DaoxRenderer_Clear( self->renderer, bgcolor );

	self->obbox = DaoxOBBox2D_InitRect( viewport.left, viewport.right, viewport.top, viewport.bottom );

	canvas->transform.B1 = - viewport.left;
	canvas->transform.B2 = - viewport.bottom;
	canvas->transform.A11 = 1.0;
	canvas->transform.A22 = 1.0;
	canvas->transform.A12 = canvas->transform.A21 = 0.0;
	for(i=0; i<n; i++){
		DaoxCanvasNode *it = canvas->nodes->items.pCanvasNode[i];
		DaoxPainter_PaintItem( self, canvas, it, canvas->transform );
	}
}


static DaoxColor Color_InitRGBA( uint_t r, uint_t g, uint_t b, uint_t a )
{
	DaoxColor color;
	color.red   = r / 255.0;
	color.green = g / 255.0;
	color.blue  = b / 255.0;
	color.alpha = a / 255.0;
	return color;
}

void DaoxPainter_PaintCanvasImage( DaoxPainter *self, DaoxCanvas *canvas, DaoxAABBox2D viewport, DaoImage *image, int width, int height )
{
	DaoxRenderer *renderer = self->renderer;
	DaoImage *target = self->target;

	image->depth = DAOX_IMAGE_BIT32;
	_DaoImage_Resize( image, width, height );

	self->target = image;
	self->renderer = DaoxRenderer_New( image );
	DaoxPainter_Paint( self, canvas, viewport );

	DaoxRenderer_Delete( self->renderer );
	self->renderer = renderer;
	self->target = target;
}
