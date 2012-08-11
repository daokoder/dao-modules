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
#include "dao_graphics.h"

DaoType *daox_type_graphics_scene = NULL;
DaoType *daox_type_graphics_item = NULL;
DaoType *daox_type_graphics_line = NULL;
DaoType *daox_type_graphics_rect = NULL;
DaoType *daox_type_graphics_circle = NULL;
DaoType *daox_type_graphics_ellipse = NULL;
DaoType *daox_type_graphics_polyline = NULL;
DaoType *daox_type_graphics_polygon = NULL;
DaoType *daox_type_graphics_path = NULL;
DaoType *daox_type_graphics_text = NULL;




DaoxGraphicsData* DaoxGraphicsData_New()
{
	DaoxGraphicsData *self = (DaoxGraphicsData*)dao_calloc(1,sizeof(DaoxGraphicsData));
	self->dashMasks = DaoxPolygonArray_New();
	self->strokePolygons = DaoxPolygonArray_New();
	self->fillPolygons = DaoxPolygonArray_New();
	return self;
}
void DaoxGraphicsData_Delete( DaoxGraphicsData *self )
{
	DaoxPolygonArray_Delete( self->dashMasks );
	DaoxPolygonArray_Delete( self->strokePolygons );
	DaoxPolygonArray_Delete( self->fillPolygons );
	dao_free( self );
}
void DaoxGraphicsData_Reset( DaoxGraphicsData *self )
{
	DaoxPolygonArray_Reset( self->dashMasks );
	DaoxPolygonArray_Reset( self->strokePolygons );
	DaoxPolygonArray_Reset( self->fillPolygons );
}
void DaoxGraphicsData_Init( DaoxGraphicsData *self, DaoxGraphicsItem *item )
{
	self->junction = item->junction;
	self->strokeWidth = item->strokeWidth;
	self->transform[0] = 1.0;
	self->transform[1] = 0.0;
	self->transform[2] = 0.0;
	self->transform[3] = 1.0;
	self->transform[4] = 0.0;
	self->transform[5] = 0.0;
	self->maxlen = 10;
	self->maxdiff = 0.001;
}




DaoxGraphicsItem* DaoxGraphicsItem_New( int shape )
{
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) dao_calloc( 1, sizeof(DaoxGraphicsItem) );
	DaoType *ctype = NULL;
	switch( shape ){
	case DAOX_GS_LINE     : ctype = daox_type_graphics_line;     break;
	case DAOX_GS_RECT     : ctype = daox_type_graphics_rect;     break;
	case DAOX_GS_CIRCLE   : ctype = daox_type_graphics_circle;   break;
	case DAOX_GS_ELLIPSE  : ctype = daox_type_graphics_ellipse;  break;
	case DAOX_GS_POLYLINE : ctype = daox_type_graphics_polyline; break;
	case DAOX_GS_POLYGON  : ctype = daox_type_graphics_polygon;  break;
	case DAOX_GS_PATH     : ctype = daox_type_graphics_path;     break;
	case DAOX_GS_TEXT     : ctype = daox_type_graphics_text;     break;
	}
	assert( ctype != NULL );
	DaoCdata_InitCommon( (DaoCdata*)self, ctype );
	self->shape = shape;
	self->junction = DAOX_JUNCTION_SHARP;
	self->transform.A = 1;
	self->transform.C = 1;
	self->strokeWidth = 1;
	self->strokeColor.alpha = 1;
	self->path = DaoxSimplePath_New();
	self->gdata = DaoxGraphicsData_New();
	if( shape >= DAOX_GS_POLYGON ) self->newpath = DaoxPath_New();
	return self;
}
void DaoxGraphicsItem_Delete( DaoxGraphicsItem *self )
{
	if( self->children ) DArray_Delete( self->children );
	if( self->newpath  ) DaoxPath_Delete( self->newpath );
	DaoxGraphicsData_Delete( self->gdata );
	DaoxSimplePath_Delete( self->path );
	DaoCdata_FreeCommon( (DaoCdata*) self );
	dao_free( self );
}


void DaoxGraphicsItem_SetStrokeWidth( DaoxGraphicsItem *self, double w )
{
	self->strokeWidth = w;
}

void DaoxGraphicsItem_SetStrokeColor( DaoxGraphicsItem *self, double r, double g, double b, double a )
{
	self->strokeColor.red = r;
	self->strokeColor.green = g;
	self->strokeColor.blue = b;
	self->strokeColor.alpha = a;
}

void DaoxGraphicsItem_SetFillColor( DaoxGraphicsItem *self, double r, double g, double b, double a )
{
	self->fillColor.red = r;
	self->fillColor.green = g;
	self->fillColor.blue = b;
	self->fillColor.alpha = a;
}


void DaoxGraphicsLine_Set( DaoxGraphicsLine *self, double x1, double y1, double x2, double y2 )
{
	assert( self->ctype == daox_type_graphics_line );
	self->position.x = x1;
	self->position.y = y1;
	self->scale.x = x2 - x1;
	self->scale.y = y2 - y1;
}

void DaoxGraphicsRect_Set( DaoxGraphicsRect *self, double x1, double y1, double x2, double y2 )
{
	assert( self->ctype == daox_type_graphics_rect );
	self->position.x = x1;
	self->position.y = y1;
	self->scale.x = x2 - x1;
	self->scale.y = y2 - y1;
}

void DaoxGraphicsCircle_Set( DaoxGraphicsCircle *self, double x, double y, double r )
{
	assert( self->ctype == daox_type_graphics_circle );
	self->position.x = x;
	self->position.y = y;
	self->scale.x = r;
	self->scale.y = r;
}

void DaoxGraphicsEllipse_Set( DaoxGraphicsEllipse *self, double x, double y, double rx, double ry )
{
	assert( self->ctype == daox_type_graphics_ellipse );
	self->position.x = x;
	self->position.y = y;
	self->scale.x = rx;
	self->scale.y = ry;
}



void DaoxGraphicsPolyLine_Add( DaoxGraphicsPolyLine *self, double x1, double y1, double x2, double y2 )
{
	assert( self->ctype == daox_type_graphics_polyline );
	DaoxPointArray_PushXY( self->path->points, x1, y1 );
	DaoxPointArray_PushXY( self->path->points, x2, y2 );
}

void DaoxGraphicsPolygon_Add( DaoxGraphicsPolygon *self, double x, double y )
{
	assert( self->ctype == daox_type_graphics_polygon );
	DaoxPointArray_PushXY( self->path->points, x, y );
}

void DaoxGraphicsPath_MoveTo( DaoxGraphicsPath *self, double x, double y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_MoveTo( self->newpath, x, y );
}
void DaoxGraphicsPath_LineTo( DaoxGraphicsPath *self, double x, double y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_LineTo( self->newpath, x, y );
}
void DaoxGraphicsPath_ArcTo( DaoxGraphicsPath *self, double x, double y, double degrees )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_ArcTo( self->newpath, x, y, degrees );
}
void DaoxGraphicsPath_QuadTo( DaoxGraphicsPath *self, double cx, double cy, double x, double y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_QuadTo( self->newpath, cx, cy, x, y );
}
void DaoxGraphicsPath_CubicTo( DaoxGraphicsPath *self, double cx, double cy, double x, double y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_CubicTo( self->newpath, cx, cy, x, y );
}
void DaoxGraphicsPath_CubicTo2( DaoxGraphicsLine *self, double cx0, double cy0, double cx, double cy, double x, double y )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_CubicTo2( self->newpath, cx0, cy0, cx, cy, x, y );
}
void DaoxGraphicsPath_Close( DaoxGraphicsPath *self )
{
	assert( self->ctype == daox_type_graphics_path );
	DaoxPath_Close( self->newpath );
}





static void DaoxGraphicsiItem_ResetPolygons( DaoxGraphicsItem *self )
{
	DaoxGraphicsData_Reset( self->gdata );
}
void DaoxGraphicsLine_UpdatePolygons( DaoxGraphicsLine *self, DaoxGraphicsScene *scene )
{
	DaoxPoint end;
	DaoxQuad quad;

	end.x = self->position.x + self->scale.x;
	end.y = self->position.y + self->scale.y;
	quad = DaoxLine2Quad( self->position, end, self->strokeWidth );

	DaoxGraphicsiItem_ResetPolygons( self );
	DaoxPolygonArray_PushQuad( self->gdata->strokePolygons, quad );
}
void DaoxGraphicsRect_UpdatePolygons( DaoxGraphicsRect *self, DaoxGraphicsScene *scene )
{
	DaoxQuad centerQuad, bottomQuad, topQuad, leftQuad, rightQuad;
	double left = self->position.x;
	double bottom = self->position.y;
	double right = self->position.x + self->scale.x;
	double top = self->position.y + self->scale.y;
	double tmp, W2 = 0.5 * self->strokeWidth;

	if( left > right ) tmp = left, left = right, right = tmp;
	if( bottom > top ) tmp = bottom, bottom = top, top = tmp;

	centerQuad = DaoxQuad_FromRect( left + W2, bottom + W2, right - W2, top - W2 );
	bottomQuad = DaoxQuad_FromRect( left - W2, bottom - W2, right + W2, bottom + W2 );
	topQuad    = DaoxQuad_FromRect( left - W2, top - W2, right + W2, top + W2 );
	leftQuad   = DaoxQuad_FromRect( left - W2, bottom + W2, left + W2, top - W2 );
	rightQuad  = DaoxQuad_FromRect( right - W2, bottom + W2, right + W2, top - W2 );

	DaoxGraphicsiItem_ResetPolygons( self );

	DaoxPolygonArray_PushRect( self->gdata->strokePolygons, bottomQuad.A, bottomQuad.C );
	DaoxPolygonArray_PushRect( self->gdata->strokePolygons, topQuad.A, topQuad.C );
	DaoxPolygonArray_PushRect( self->gdata->strokePolygons, leftQuad.A, leftQuad.C );
	DaoxPolygonArray_PushRect( self->gdata->strokePolygons, rightQuad.A, rightQuad.C );

	if( self->fillColor.alpha < 1E-9 ) return;
	DaoxPolygonArray_PushRect( self->gdata->fillPolygons, centerQuad.A, centerQuad.C );
}
void DaoxGraphicsEllipse_UpdatePolygons( DaoxGraphicsEllipse *self, DaoxGraphicsScene *scene )
{
	double CX = self->position.x;
	double CY = self->position.y;
	double RX = self->scale.x;
	double RY = self->scale.y;
	double W = self->strokeWidth;

	self->gdata->junction = DAOX_JUNCTION_FLAT;
	self->gdata->strokeWidth = W;
	self->gdata->transform[1] = 0.0;
	self->gdata->transform[2] = 0.0;
	self->gdata->transform[4] = CX;
	self->gdata->transform[5] = CY;
	self->gdata->maxlen = 3.5 * log(RX + RY + W + 1.0) / log(2.0);
	self->gdata->maxdiff = 0.5 / (RX + RY + W + 1.0);

	if( fabs( RX - RY ) < 1E-16 ){
		if( RX <= 10 ){
			double scale = RX / 10.0;
			self->gdata->transform[0] = scale;
			self->gdata->transform[3] = scale;
			self->gdata->maxlen = 1.0;
			DaoxPath_ExportGraphicsData( scene->circleSmall, self->gdata );
		}else{
			double scale = RX / 100.0;
			self->gdata->transform[0] = scale;
			self->gdata->transform[3] = scale;
			DaoxPath_ExportGraphicsData( scene->circleLarge, self->gdata );
		}
		return;
	}
	if( RX > RY ){
		if( RX < 3.0 * RY ){
			self->gdata->transform[0] = RX / 200.0;
			self->gdata->transform[3] = RY / 100.0;
			DaoxPath_ExportGraphicsData( scene->ellipseWide, self->gdata );
		}else{
			self->gdata->transform[0] = RX / 400.0;
			self->gdata->transform[3] = RY / 100.0;
			DaoxPath_ExportGraphicsData( scene->ellipseNarrow, self->gdata );
		}
	}else{
		self->gdata->transform[0] = 0.0;
		self->gdata->transform[3] = 0.0;
		if( RY < 3.0 * RX ){
			self->gdata->transform[1] = RY / 200.0;
			self->gdata->transform[2] = RX / 100.0;
			DaoxPath_ExportGraphicsData( scene->ellipseWide, self->gdata );
		}else{
			self->gdata->transform[1] = RY / 400.0;
			self->gdata->transform[2] = RX / 100.0;
			DaoxPath_ExportGraphicsData( scene->ellipseNarrow, self->gdata );
		}
	}

}
void DaoxGraphicsCircle_UpdatePolygons( DaoxGraphicsCircle *self, DaoxGraphicsScene *scene )
{
	DaoxGraphicsEllipse_UpdatePolygons( self, scene );
}
void DaoxGraphicsPolyLine_UpdatePolygons( DaoxGraphicsPolyLine *self, DaoxGraphicsScene *scene )
{
	int i;

	DaoxGraphicsiItem_ResetPolygons( self );

	for(i=0; i<self->path->points->count; i+=2){
		DaoxPoint start = self->path->points->points[i];
		DaoxPoint end = self->path->points->points[i+1];
		DaoxQuad quad = DaoxLine2Quad( start, end, self->strokeWidth );
		DaoxPolygonArray_PushQuad( self->gdata->strokePolygons, quad );
	}
}
void DaoxGraphicsPolygon_UpdatePolygons( DaoxGraphicsPolygon *self, DaoxGraphicsScene *scene )
{
	DaoxGraphicsiItem_ResetPolygons( self );
	DaoxPolygonArray_MakeLines( self->gdata->strokePolygons, self->path->points, NULL, self->strokeWidth, self->junction, 1 );
	printf( "@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	//if( self->fillColor.alpha < 1E-9 ) return;
	printf( "@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
	DaoxPolygonArray_TriangulatePolygon( self->gdata->fillPolygons, self->path->points, scene->buffer->triangulator );
}
void DaoxGraphicsPath_UpdatePolygons( DaoxGraphicsPath *self, DaoxGraphicsScene *scene )
{
	DaoxPolygonArray *fills = self->fillColor.alpha < 1E-9 ? NULL : self->gdata->fillPolygons;
	DaoxGraphicsiItem_ResetPolygons( self );
#if 0
	DaoxPathGraph_Reset( scene->graph );
	DaoxPathGraph_Import( scene->graph, self->path );
	DaoxPathGraph_IntersectEdges( scene->graph );
	DaoxPathGraph_Export( scene->graph, self->path );
#endif
	//DaoxSimplePath_MakePolygons( self->path, self->strokeWidth, self->junction, self->gdata->strokePolygons, fills, scene->buffer );
	if( self->newpath->first->refined.first == NULL ) DaoxPath_Preprocess( self->newpath, scene->buffer );
	DaoxGraphicsData_Init( self->gdata, self );
	DaoxPath_ExportGraphicsData( self->newpath, self->gdata );
	printf( "<<<<<<<<<<<<<<<<<<<<<<<<< polygons: %i\n", self->gdata->fillPolygons->polygons->count );
}
void DaoxGraphicsText_UpdatePolygons( DaoxGraphicsText *self, DaoxGraphicsScene *scene )
{
	//DaoxGraphicsPath_UpdatePolygons( self, scene );
}

void DaoxGraphicsItem_UpdatePolygons( DaoxGraphicsItem *self, DaoxGraphicsScene *scene )
{
	if( self->gdata->strokePolygons->points->count || self->gdata->fillPolygons->points->count ) return;
	switch( self->shape ){
	case DAOX_GS_LINE     : DaoxGraphicsLine_UpdatePolygons( self, scene );     break;
	case DAOX_GS_RECT     : DaoxGraphicsRect_UpdatePolygons( self, scene );     break;
	case DAOX_GS_CIRCLE   : DaoxGraphicsCircle_UpdatePolygons( self, scene );   break;
	case DAOX_GS_ELLIPSE  : DaoxGraphicsEllipse_UpdatePolygons( self, scene );  break;
	case DAOX_GS_POLYLINE : DaoxGraphicsPolyLine_UpdatePolygons( self, scene ); break;
	case DAOX_GS_POLYGON  : DaoxGraphicsPolygon_UpdatePolygons( self, scene );  break;
	case DAOX_GS_PATH     : DaoxGraphicsPath_UpdatePolygons( self, scene );     break;
	case DAOX_GS_TEXT     : DaoxGraphicsText_UpdatePolygons( self, scene );     break;
	}
}







DaoxGraphicsScene* DaoxGraphicsScene_New()
{
	double X2[6] = { 2.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
	double X4[6] = { 4.0, 0.0, 0.0, 1.0, 0.0, 0.0 };

	DaoxGraphicsScene *self = (DaoxGraphicsScene*) dao_calloc( 1, sizeof(DaoxGraphicsScene) );
	DaoCdata_InitCommon( (DaoCdata*) self, daox_type_graphics_scene );
	self->items = DArray_New(D_VALUE);
	self->buffer = DaoxPathBuffer_New();
	self->graph = DaoxPathGraph_New();
	self->circleSmall = DaoxPath_New();
	self->circleLarge = DaoxPath_New();
	self->ellipseWide = DaoxPath_New();
	self->ellipseNarrow = DaoxPath_New();

	/* less accurate approximation for small circle: */
	DaoxPath_MoveTo( self->circleSmall, -10.0, 0 );
	DaoxPath_ArcTo2( self->circleSmall,  20.0, 0, 180, 180 );
	DaoxPath_ArcTo2( self->circleSmall, -20.0, 0, 180, 180 );
	DaoxPath_Close( self->circleSmall );

	DaoxPath_MoveTo( self->circleLarge, -100, 0 );
	DaoxPath_ArcTo( self->circleLarge,  200.0, 0, 180 );
	DaoxPath_ArcTo( self->circleLarge, -200.0, 0, 180 );
	DaoxPath_Close( self->circleLarge );

	DaoxPath_ImportPath( self->ellipseWide, self->circleLarge, X2 );
	DaoxPath_ImportPath( self->ellipseNarrow, self->circleLarge, X4 );

	DaoxPath_Preprocess( self->circleSmall, self->buffer );
	DaoxPath_Preprocess( self->circleLarge, self->buffer );
	DaoxPath_Preprocess( self->ellipseWide, self->buffer );
	DaoxPath_Preprocess( self->ellipseNarrow, self->buffer );
	return self;
}
void DaoxGraphicsScene_Delete( DaoxGraphicsScene *self )
{
	DaoCdata_FreeCommon( (DaoCdata*) self );
	DArray_Delete( self->items );
	DaoxPathBuffer_Delete( self->buffer );
	DaoxPathGraph_Delete( self->graph );
	DaoxPath_Delete( self->circleSmall );
	DaoxPath_Delete( self->circleLarge );
	DaoxPath_Delete( self->ellipseWide );
	DaoxPath_Delete( self->ellipseNarrow );
	dao_free( self );
}

void DaoxGraphicsScene_SetFont( DaoxGraphicsScene *self, DaoxFont *font, double size )
{
	DaoGC_ShiftRC( (DaoValue*) font, (DaoValue*) self->font );
	self->font = font;
}

DaoxGraphicsLine* DaoxGraphicsScene_AddLine( DaoxGraphicsScene *self, double x1, double y1, double x2, double y2 )
{
	DaoxGraphicsLine *item = DaoxGraphicsItem_New( DAOX_GS_LINE );
	DaoxGraphicsLine_Set( item, x1, y1, x2, y2 );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsRect* DaoxGraphicsScene_AddRect( DaoxGraphicsScene *self, double x1, double y1, double x2, double y2 )
{
	DaoxGraphicsRect *item = DaoxGraphicsItem_New( DAOX_GS_RECT );
	DaoxGraphicsRect_Set( item, x1, y1, x2, y2 );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsCircle* DaoxGraphicsScene_AddCircle( DaoxGraphicsScene *self, double x, double y, double r )
{
	DaoxGraphicsCircle *item = DaoxGraphicsItem_New( DAOX_GS_CIRCLE );
	DaoxGraphicsCircle_Set( item, x, y, r );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsEllipse* DaoxGraphicsScene_AddEllipse( DaoxGraphicsScene *self, double x, double y, double rx, double ry )
{
	DaoxGraphicsEllipse *item = DaoxGraphicsItem_New( DAOX_GS_ELLIPSE );
	DaoxGraphicsEllipse_Set( item, x, y, rx, ry );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsPolyLine* DaoxGraphicsScene_AddPolyLine( DaoxGraphicsScene *self )
{
	DaoxGraphicsPolyLine *item = DaoxGraphicsItem_New( DAOX_GS_POLYLINE );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsPolygon* DaoxGraphicsScene_AddPolygon( DaoxGraphicsScene *self )
{
	DaoxGraphicsPolygon *item = DaoxGraphicsItem_New( DAOX_GS_POLYGON );
	DArray_PushBack( self->items, item );
	return item;
}

DaoxGraphicsPath* DaoxGraphicsScene_AddPath( DaoxGraphicsScene *self )
{
	DaoxGraphicsPath *item = DaoxGraphicsItem_New( DAOX_GS_PATH );
	DArray_PushBack( self->items, item );
	return item;
}
DaoxGraphicsText* DaoxGraphicsScene_AddText( DaoxGraphicsScene *self, const wchar_t *text, double x, double y )
{
	int i, j, jt = DAOX_JUNCTION_FLAT;
	double size, scale, maxlen, maxdiff;
	DaoxGlyph *glyph;
	DaoxGraphicsPath *item;
	
	if( self->font == NULL ) return NULL;

	size = 100.0;
	scale = size / (double)self->font->fontHeight;
	maxlen = 8.0 * self->font->fontHeight / size; 
	maxdiff = 1.0 / size;
	//maxlen = 1000;
	//maxdiff = 1.0;

	item = DaoxGraphicsItem_New( DAOX_GS_TEXT );
	DArray_PushBack( self->items, item );

	item->gdata->strokeWidth = 0;
	item->gdata->junction = DAOX_JUNCTION_FLAT;
	item->gdata->maxlen = maxlen;
	item->gdata->maxdiff = maxdiff;
	item->gdata->transform[0] = scale;
	item->gdata->transform[1] = 0.0;
	item->gdata->transform[2] = 0.0;
	item->gdata->transform[3] = scale;
	item->gdata->transform[4] = x;
	item->gdata->transform[5] = y;

	while( *text ){
		glyph = DaoxFont_GetCharGlyph( self->font, *text++ );
		printf( "%3i %2lc %9p\n", *(text-1), *(text-1), glyph );
		if( glyph == NULL ) break;
		DaoxPath_ExportGraphicsData( glyph->shape, item->gdata );
		item->gdata->transform[4] += scale * glyph->advanceWidth;
		printf( "<<<<<<<<<<<<<<<<<<<<<<<<< text polygons: %i\n", item->gdata->fillPolygons->polygons->count );
	}

	return item;
}








static void ITEM_SetStrokeWidth( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) p[0];
	DaoxGraphicsItem_SetStrokeWidth( self, p[1]->xDouble.value );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void ITEM_SetStrokeColor( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) p[0];
	double red   = p[1]->xTuple.items[0]->xDouble.value;
	double green = p[1]->xTuple.items[1]->xDouble.value;
	double blue  = p[1]->xTuple.items[2]->xDouble.value;
	double alpha = p[2]->xDouble.value;
	DaoxGraphicsItem_SetStrokeColor( self, red, green, blue, alpha );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void ITEM_SetFillColor( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) p[0];
	double red   = p[1]->xTuple.items[0]->xDouble.value;
	double green = p[1]->xTuple.items[1]->xDouble.value;
	double blue  = p[1]->xTuple.items[2]->xDouble.value;
	double alpha = p[2]->xDouble.value;
	DaoxGraphicsItem_SetFillColor( self, red, green, blue, alpha );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void ITEM_AddChild( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) p[0];
	DaoxGraphicsItem *child = (DaoxGraphicsItem*) p[1];
	if( self->children == NULL ) self->children = DArray_New(D_VALUE);
	DArray_PushBack( self->children, child );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}

static void DaoxGraphicsItem_GetGCFields( void *p, DArray *values, DArray *arrays, DArray *maps, int remove )
{
	daoint i, n;
	DaoxGraphicsItem *self = (DaoxGraphicsItem*) p;
	if( self->children == NULL ) return;
	DArray_Append( arrays, self->children );
}

static DaoFuncItem DaoxGraphicsItemMeths[]=
{
	{ ITEM_SetStrokeWidth,  "SetStrokeWidth( self : GraphicsItem, width = 1.0D ) => GraphicsItem" },
	{ ITEM_SetStrokeColor,  "SetStrokeColor( self : GraphicsItem, color : tuple<red:double,green:double,blue:double>, alpha = 1.0D ) => GraphicsItem" },
	{ ITEM_SetFillColor,  "SetFillColor( self : GraphicsItem, color : tuple<red:double,green:double,blue:double>, alpha = 1.0D ) => GraphicsItem" },

	{ ITEM_AddChild,  "AddChild( self : GraphicsItem, child : GraphicsItem ) => GraphicsItem" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsItem_Typer =
{
	"GraphicsItem", NULL, NULL, (DaoFuncItem*) DaoxGraphicsItemMeths, {0}, {0},
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};




static void LINE_SetData( DaoxGraphicsLine *self, DaoValue *p[] )
{
	double x1 = p[0]->xDouble.value;
	double y1 = p[1]->xDouble.value;
	double x2 = p[2]->xDouble.value;
	double y2 = p[3]->xDouble.value;
	DaoxGraphicsLine_Set( self, x1, y1, x2, y2 );
}
static void LINE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsLine *self = DaoxGraphicsItem_New( DAOX_GS_LINE );
	DaoProcess_PutValue( proc, (DaoValue*) self );
	LINE_SetData( self, p );
}
static void LINE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsLine *self = (DaoxGraphicsLine*) p[0];
	LINE_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsLineMeths[]=
{
	{ LINE_New,     "GraphicsLine( x1 = 0.0D, y1 = 0.0D, x2 = 1.0D, y2 = 1.0D )" },
	{ LINE_Set,     "Set( self : GraphicsLine, x1 = 0.0D, y1 = 0.0D, x2 = 1.0D, y2 = 1.0D ) => GraphicsLine" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsLine_Typer =
{
	"GraphicsLine", NULL, NULL, (DaoFuncItem*) DaoxGraphicsLineMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};





static void RECT_SetData( DaoxGraphicsRect *self, DaoValue *p[] )
{
	double x1 = p[0]->xDouble.value;
	double y1 = p[1]->xDouble.value;
	double x2 = p[2]->xDouble.value;
	double y2 = p[3]->xDouble.value;
	DaoxGraphicsRect_Set( self, x1, y1, x2, y2 );
}
static void RECT_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsRect *self = DaoxGraphicsItem_New( DAOX_GS_RECT );
	DaoProcess_PutValue( proc, (DaoValue*) self );
	RECT_SetData( self, p );
}
static void RECT_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsRect *self = (DaoxGraphicsRect*) p[0];
	RECT_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsRectMeths[]=
{
	{ RECT_New,     "GraphicsRect( x1 = 0.0D, y1 = 0.0D, x2 = 1.0D, y2 = 1.0D )" },
	{ RECT_Set,     "Set( self : GraphicsRect, x1 = 0.0D, y1 = 0.0D, x2 = 1.0D, y2 = 1.0D ) => GraphicsRect" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsRect_Typer =
{
	"GraphicsRect", NULL, NULL, (DaoFuncItem*) DaoxGraphicsRectMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};






static void CIRCLE_SetData( DaoxGraphicsCircle *self, DaoValue *p[] )
{
	double x = p[0]->xDouble.value;
	double y = p[1]->xDouble.value;
	double r = p[2]->xDouble.value;
	DaoxGraphicsCircle_Set( self, x, y, r );
}
static void CIRCLE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsCircle *self = DaoxGraphicsItem_New( DAOX_GS_CIRCLE );
	DaoProcess_PutValue( proc, (DaoValue*) self );
	CIRCLE_SetData( self, p );
}
static void CIRCLE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsCircle *self = (DaoxGraphicsCircle*) p[0];
	CIRCLE_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsCircleMeths[]=
{
	{ CIRCLE_New,     "GraphicsCircle( cx = 0.0D, cy = 0.0D, r = 1.0D )" },
	{ CIRCLE_Set,     "Set( self : GraphicsCircle, cx = 0.0D, cy = 0.0D, r = 1.0D ) => GraphicsCircle" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsCircle_Typer =
{
	"GraphicsCircle", NULL, NULL, (DaoFuncItem*) DaoxGraphicsCircleMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};






static void ELLIPSE_SetData( DaoxGraphicsEllipse *self, DaoValue *p[] )
{
	double x1 = p[0]->xDouble.value;
	double y1 = p[1]->xDouble.value;
	double x2 = p[2]->xDouble.value;
	double y2 = p[3]->xDouble.value;
	DaoxGraphicsEllipse_Set( self, x1, y1, x2, y2 );
}
static void ELLIPSE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsEllipse *self = DaoxGraphicsItem_New( DAOX_GS_ELLIPSE );
	DaoProcess_PutValue( proc, (DaoValue*) self );
	ELLIPSE_SetData( self, p );
}
static void ELLIPSE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsEllipse *self = (DaoxGraphicsEllipse*) p[0];
	ELLIPSE_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsEllipseMeths[]=
{
	{ ELLIPSE_New,  "GraphicsEllipse( cx = 0.0D, cy = 0.0D, rx = 1.0D, ry = 1.0D )" },
	{ ELLIPSE_Set,  "Set( self : GraphicsEllipse, cx = 0.0D, cy = 0.0D, rx = 1.0D, ry = 1.0D ) => GraphicsEllipse" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsEllipse_Typer =
{
	"GraphicsEllipse", NULL, NULL, (DaoFuncItem*) DaoxGraphicsEllipseMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};





static void POLYLINE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolyLine *self = DaoxGraphicsItem_New( DAOX_GS_POLYLINE );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void POLYLINE_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolyLine *self = (DaoxGraphicsPolyLine*) p[0];
	double x1 = p[1]->xDouble.value;
	double y1 = p[2]->xDouble.value;
	double x2 = p[3]->xDouble.value;
	double y2 = p[4]->xDouble.value;
	DaoxGraphicsPolyLine_Add( self, x1, y1, x2, y2 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsPolyLineMeths[]=
{
	{ POLYLINE_New,   "GraphicsPolyLine()" },
	{ POLYLINE_Add,   "Add( self : GraphicsPolyLine, x1 = 0.0D, y1 = 0.0D, x2 = 1.0D, y2 = 1.0D ) => GraphicsPolyLine" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsPolyLine_Typer =
{
	"GraphicsPolyLine", NULL, NULL, (DaoFuncItem*) DaoxGraphicsPolyLineMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};





static void POLYGON_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolygon *self = DaoxGraphicsItem_New( DAOX_GS_POLYGON );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void POLYGON_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolygon *self = (DaoxGraphicsPolygon*) p[0];
	double x = p[1]->xDouble.value;
	double y = p[2]->xDouble.value;
	DaoxGraphicsPolygon_Add( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void POLYGON_SetJunction( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPolygon *self = (DaoxGraphicsPolygon*) p[0];
	self->junction = p[1]->xEnum.value;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsPolygonMeths[]=
{
	{ POLYGON_New,   "GraphicsPolygon()" },
	{ POLYGON_Add,   "Add( self : GraphicsPolygon, x : double, y : double ) => GraphicsPolygon" },
	{ POLYGON_SetJunction, 
		"SetJunction( self : GraphicsPolygon, junction: enum<none,sharp,flat,round> = $sharp ) => GraphicsPolygon" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsPolygon_Typer =
{
	"GraphicsPolygon", NULL, NULL, (DaoFuncItem*) DaoxGraphicsPolygonMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};






static void PATH_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = DaoxGraphicsItem_New( DAOX_GS_PATH );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_MoveTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double x = p[1]->xDouble.value;
	double y = p[2]->xDouble.value;
	DaoxGraphicsPath_MoveTo( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_LineTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double x = p[1]->xDouble.value;
	double y = p[2]->xDouble.value;
	DaoxGraphicsPath_LineTo( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_ArcTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double x = p[1]->xDouble.value;
	double y = p[2]->xDouble.value;
	double d = p[3]->xDouble.value;
	DaoxGraphicsPath_ArcTo( self, x, y, d );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_QuadTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double cx = p[1]->xDouble.value;
	double cy = p[2]->xDouble.value;
	double x = p[3]->xDouble.value;
	double y = p[4]->xDouble.value;
	DaoxGraphicsPath_QuadTo( self, cx, cy, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_CubicTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double cx = p[1]->xDouble.value;
	double cy = p[2]->xDouble.value;
	double x = p[3]->xDouble.value;
	double y = p[4]->xDouble.value;
	DaoxGraphicsPath_CubicTo( self, cx, cy, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_CubicTo2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	double cx0 = p[1]->xDouble.value;
	double cy0 = p[2]->xDouble.value;
	double cx = p[3]->xDouble.value;
	double cy = p[4]->xDouble.value;
	double x = p[5]->xDouble.value;
	double y = p[6]->xDouble.value;
	DaoxGraphicsPath_CubicTo2( self, cx0, cy0, cx, cy, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_Close( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsPath *self = (DaoxGraphicsPath*) p[0];
	DaoxGraphicsPath_Close( self );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFuncItem DaoxGraphicsPathMeths[]=
{
	{ PATH_New,       "GraphicsPath()" },
	{ PATH_MoveTo,    "MoveTo( self : GraphicsPath, x : double, y : double ) => GraphicsPath" },
	{ PATH_LineTo,    "LineTo( self : GraphicsPath, x : double, y : double ) => GraphicsPath" },
	{ PATH_ArcTo,     "ArcTo( self : GraphicsPath, x : double, y : double, degrees : double ) => GraphicsPath" },
	{ PATH_QuadTo,   "QuadTo( self : GraphicsPath, cx : double, cy : double, x : double, y : double ) => GraphicsPath" },
	{ PATH_CubicTo,   "CubicTo( self : GraphicsPath, cx : double, cy : double, x : double, y : double ) => GraphicsPath" },
	{ PATH_CubicTo2,  "CubicTo( self : GraphicsPath, cx0 : double, cy0 : double, cx : double, cy : double, x : double, y : double ) => GraphicsPath" },
	{ PATH_Close,     "Close( self : GraphicsPath ) => GraphicsPath" },
	{ POLYGON_SetJunction, 
		"SetJunction( self : GraphicsPath, junction: enum<none,sharp,flat,round> = $sharp ) => GraphicsPolygon" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsPath_Typer =
{
	"GraphicsPath", NULL, NULL, (DaoFuncItem*) DaoxGraphicsPathMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};




static DaoFuncItem DaoxGraphicsTextMeths[]=
{
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsText_Typer =
{
	"GraphicsText", NULL, NULL, (DaoFuncItem*) DaoxGraphicsTextMeths,
	{ & DaoxGraphicsItem_Typer, NULL }, { NULL },
	(FuncPtrDel)DaoxGraphicsItem_Delete, DaoxGraphicsItem_GetGCFields
};




static void SCENE_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = DaoxGraphicsScene_New();
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void SCENE_AddLine( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	double x1 = p[1]->xDouble.value, y1 = p[2]->xDouble.value;
	double x2 = p[3]->xDouble.value, y2 = p[4]->xDouble.value;
	DaoxGraphicsLine *item = DaoxGraphicsScene_AddLine( self, x1, y1, x2, y2 );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddRect( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	double x1 = p[1]->xDouble.value, y1 = p[2]->xDouble.value;
	double x2 = p[3]->xDouble.value, y2 = p[4]->xDouble.value;
	DaoxGraphicsRect *item = DaoxGraphicsScene_AddRect( self, x1, y1, x2, y2 );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddCircle( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	double x = p[1]->xDouble.value, y = p[2]->xDouble.value;
	double r = p[3]->xDouble.value;
	DaoxGraphicsCircle *item = DaoxGraphicsScene_AddCircle( self, x, y, r );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddEllipse( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	double x = p[1]->xDouble.value, y = p[2]->xDouble.value;
	double rx = p[3]->xDouble.value, ry = p[4]->xDouble.value;
	DaoxGraphicsEllipse *item = DaoxGraphicsScene_AddEllipse( self, x, y, rx, ry );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddPolyLine( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsPolyLine *item = DaoxGraphicsScene_AddPolyLine( self );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddPolygon( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsPolygon *item = DaoxGraphicsScene_AddPolygon( self );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddPath( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxGraphicsPath *item = DaoxGraphicsScene_AddPath( self );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_AddText( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DString *text = DaoValue_TryGetString( p[1] );
	double x = p[2]->xDouble.value;
	double y = p[3]->xDouble.value;
	DaoxGraphicsText *item = DaoxGraphicsScene_AddText( self, DString_GetWCS( text ), x, y );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void SCENE_SetFont( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	DaoxFont *font = (DaoxFont*) p[1];
	DaoxGraphicsScene_SetFont( self, font, p[2]->xDouble.value );
}
static void SCENE_Test( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p[0];
	int i, n = self->items->size;
	for(i=0; i<n; i++){
		DaoxGraphicsPath *item = (DaoxGraphicsItem*) self->items->items.pVoid[i];
		DaoxGraphicsItem_UpdatePolygons( item, self );
	}
}

static void DaoxGraphicsScene_GetGCFields( void *p, DArray *values, DArray *arrays, DArray *maps, int remove )
{
	daoint i, n;
	DaoxGraphicsScene *self = (DaoxGraphicsScene*) p;
	if( self->font ) DArray_Append( values, self->font );
	if( remove ) self->font = NULL;
	if( self->items == NULL ) return;
	DArray_Append( arrays, self->items );
}

static DaoFuncItem DaoxGraphicsSceneMeths[]=
{
	{ SCENE_New,       "GraphicsScene()" },
	{ SCENE_AddLine,   "AddLine( self: GraphicsScene, x1: double, y1: double, x2: double, y2: double ) => GraphicsLine" },
	{ SCENE_AddRect,   "AddRect( self: GraphicsScene, x1: double, y1: double, x2: double, y2: double ) => GraphicsRect" },
	{ SCENE_AddCircle,    "AddCircle( self: GraphicsScene, x: double, y: double, r: double ) => GraphicsCircle" },
	{ SCENE_AddEllipse,   "AddEllipse( self: GraphicsScene, x: double, y: double, rx: double, ry: double ) => GraphicsEllipse" },
	{ SCENE_AddPolyLine,  "AddPolyLine( self: GraphicsScene ) => GraphicsPolyLine" },
	{ SCENE_AddPolygon,   "AddPolygon( self: GraphicsScene ) => GraphicsPolygon" },
	{ SCENE_AddPath,      "AddPath( self: GraphicsScene ) => GraphicsPath" },
	{ SCENE_AddText,      "AddText( self: GraphicsScene, text : string, x :double, y :double ) => GraphicsText" },
	{ SCENE_SetFont,      "SetFont( self: GraphicsScene, font : Font, size = 12D )" },
	{ SCENE_Test,         "Test( self: GraphicsScene ) => GraphicsPath" },
	{ NULL, NULL }
};

DaoTypeBase DaoxGraphicsScene_Typer =
{
	"GraphicsScene", NULL, NULL, (DaoFuncItem*) DaoxGraphicsSceneMeths, {0}, {0},
	(FuncPtrDel)DaoxGraphicsScene_Delete, DaoxGraphicsScene_GetGCFields
};





DAO_DLL int DaoTriangulator_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns );
DAO_DLL int DaoFont_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns );

DAO_DLL int DaoOnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoFont_OnLoad( vmSpace, ns );

	daox_type_graphics_scene = DaoNamespace_WrapType( ns, & DaoxGraphicsScene_Typer, 0 );
	daox_type_graphics_item = DaoNamespace_WrapType( ns, & DaoxGraphicsItem_Typer, 0 );
	daox_type_graphics_line = DaoNamespace_WrapType( ns, & DaoxGraphicsLine_Typer, 0 );
	daox_type_graphics_rect = DaoNamespace_WrapType( ns, & DaoxGraphicsRect_Typer, 0 );
	daox_type_graphics_circle = DaoNamespace_WrapType( ns, & DaoxGraphicsCircle_Typer, 0 );
	daox_type_graphics_ellipse = DaoNamespace_WrapType( ns, & DaoxGraphicsEllipse_Typer, 0 );
	daox_type_graphics_polyline = DaoNamespace_WrapType( ns, & DaoxGraphicsPolyLine_Typer, 0 );
	daox_type_graphics_polygon = DaoNamespace_WrapType( ns, & DaoxGraphicsPolygon_Typer, 0 );
	daox_type_graphics_path = DaoNamespace_WrapType( ns, & DaoxGraphicsPath_Typer, 0 );
	daox_type_graphics_text = DaoNamespace_WrapType( ns, & DaoxGraphicsText_Typer, 0 );

	DaoTriangulator_OnLoad( vmSpace, ns );
	return 0;
}
