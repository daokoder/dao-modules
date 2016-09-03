/*
// Dao Graphics Engine
// http://www.daovm.net
//
// Copyright (c) 2013-2014, Limin Fu
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


#include "dao_canvas.h"
#include "dao_painter.h"
#include "dao_rasterizer.h"


DaoVmSpace *dao_vmspace_graphics = NULL;


static void PATH_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *path = DaoxPath_New();
	DaoProcess_PutValue( proc, (DaoValue*) path );
}
static void PATH_MoveTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	DaoxPath_MoveTo( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_LineTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	DaoxPath_LineTo( self, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_ArcTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	float d = p[3]->xFloat.value;
	DaoxPath_ArcTo( self, x, y, d );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_ArcBy( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	float d = p[3]->xFloat.value;
	DaoxPath_ArcBy( self, x, y, d );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_QuadTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	float cx = p[1]->xFloat.value;
	float cy = p[2]->xFloat.value;
	float x = p[3]->xFloat.value;
	float y = p[4]->xFloat.value;
	DaoxPath_QuadTo( self, cx, cy, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_CubicTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	float cx = p[1]->xFloat.value;
	float cy = p[2]->xFloat.value;
	float x = p[3]->xFloat.value;
	float y = p[4]->xFloat.value;
	DaoxPath_CubicTo( self, cx, cy, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_CubicTo2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	float cx0 = p[1]->xFloat.value;
	float cy0 = p[2]->xFloat.value;
	float cx = p[3]->xFloat.value;
	float cy = p[4]->xFloat.value;
	float x = p[5]->xFloat.value;
	float y = p[6]->xFloat.value;
	DaoxPath_CubicTo2( self, cx0, cy0, cx, cy, x, y );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PATH_Close( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_Close( self );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}

static void PATH_LineRelTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_SetRelativeMode( self, 1 );
	PATH_LineTo( proc, p, N );
}
static void PATH_ArcRelTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_SetRelativeMode( self, 1 );
	PATH_ArcTo( proc, p, N );
}
static void PATH_ArcRelBy( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_SetRelativeMode( self, 1 );
	PATH_ArcBy( proc, p, N );
}
static void PATH_QuadRelTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_SetRelativeMode( self, 1 );
	PATH_QuadTo( proc, p, N );
}
static void PATH_CubicRelTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_SetRelativeMode( self, 1 );
	PATH_CubicTo( proc, p, N );
}
static void PATH_CubicRelTo2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_SetRelativeMode( self, 1 );
	PATH_CubicTo2( proc, p, N );
}
static void PATH_LineAbsTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_SetRelativeMode( self, 0 );
	PATH_LineTo( proc, p, N );
}
static void PATH_ArcAbsTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_SetRelativeMode( self, 0 );
	PATH_ArcTo( proc, p, N );
}
static void PATH_ArcAbsBy( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_SetRelativeMode( self, 0 );
	PATH_ArcBy( proc, p, N );
}
static void PATH_QuadAbsTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_SetRelativeMode( self, 0 );
	PATH_QuadTo( proc, p, N );
}
static void PATH_CubicAbsTo( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_SetRelativeMode( self, 0 );
	PATH_CubicTo( proc, p, N );
}
static void PATH_CubicAbsTo2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPath *self = (DaoxPath*) p[0];
	DaoxPath_SetRelativeMode( self, 0 );
	PATH_CubicTo2( proc, p, N );
}

static DaoFunctionEntry daoPathMeths[]=
{
	{ PATH_New,    "Path() => Path" },

	{ PATH_MoveTo,    "MoveTo( self: Path, x: float, y: float ) => Path" },

	{ PATH_LineRelTo,    "LineTo( self: Path, x: float, y: float ) => Path" },

	{ PATH_ArcRelTo,     "ArcTo( self: Path, x: float, y: float, degrees: float ) => Path" },

	{ PATH_ArcRelBy,     "ArcBy( self: Path, cx: float, cy: float, degrees: float ) => Path" },

	{ PATH_QuadRelTo,   "QuadTo( self: Path, cx: float, cy: float, x: float, y: float ) => Path" },

	{ PATH_CubicRelTo,   "CubicTo( self: Path, cx: float, cy: float, x: float, y: float ) => Path" },

	{ PATH_CubicRelTo2,  "CubicTo( self: Path, cx0: float, cy0: float, cx: float, cy: float, x: float, y: float ) => Path" },

	{ PATH_LineAbsTo,    "LineAbsTo( self: Path, x: float, y: float ) => Path" },

	{ PATH_ArcAbsTo,     "ArcAbsTo( self: Path, x: float, y: float, degrees: float ) => Path" },

	{ PATH_ArcAbsBy,     "ArcAbsBy( self: Path, cx: float, cy: float, degrees: float ) => Path" },

	{ PATH_QuadAbsTo,   "QuadAbsTo( self: Path, cx: float, cy: float, x: float, y: float ) => Path" },

	{ PATH_CubicAbsTo,   "CubicAbsTo( self: Path, cx: float, cy: float, x: float, y: float ) => Path" },

	{ PATH_CubicAbsTo2,  "CubicAbsTo( self: Path, cx0: float, cy0: float, cx: float, cy: float, x: float, y: float ) => Path" },

	{ PATH_Close,     "Close( self: Path ) => Path" },
	{ NULL, NULL }
};

DaoTypeCore daoPathCore =
{
	"Path",                                            /* name */
	sizeof(DaoxPath),                                  /* size */
	{ NULL },                                          /* bases */
	NULL,                                              /* numbers */
	daoPathMeths,                                      /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* GetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxPath_Delete,               /* Delete */
	NULL                                               /* HandleGC */
};


static void DaoxPathMesh_HandleGC( DaoValue *p, DList *values, DList *lists, DList *maps, int remove )
{
	DaoxPathMesh *self = (DaoxPathMesh*) p;
	if( self->path ) DList_Append( values, self->path );
	if( self->stroke ) DList_Append( values, self->stroke );
	if( remove ){
		self->path = NULL;
		self->stroke = NULL;
	}
}

DaoTypeCore daoPathMeshCore =
{
	"PathMesh",                                        /* name */
	sizeof(DaoxPathMesh),                              /* size */
	{ NULL },                                          /* bases */
	NULL,                                              /* numbers */
	NULL,                                              /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* GetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxPathMesh_Delete,           /* Delete */
	DaoxPathMesh_HandleGC                              /* HandleGC */
};



static void DaoxPathCache_HandleGC( DaoValue *p, DList *values, DList *lists, DList *maps, int remove )
{
	DNode *it;
	DaoxPathCache *self = (DaoxPathCache*) p;
	for(it=DMap_First(self->paths); it; it=DMap_Next(self->paths,it)){
		DList_Append( lists, it->value.pVoid );
	}
	for(it=DMap_First(self->meshes); it; it=DMap_Next(self->meshes,it)){
		DList_Append( lists, it->value.pVoid );
	}
}

DaoTypeCore daoPathCacheCore =
{
	"PathCache",                                       /* name */
	sizeof(DaoxPathCache),                             /* size */
	{ NULL },                                          /* bases */
	NULL,                                              /* numbers */
	NULL,                                              /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* GetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxPathCache_Delete,          /* Delete */
	DaoxPathCache_HandleGC                             /* HandleGC */
};





static void DaoxColor_FromDaoValues( DaoxColor *self, DaoValue *values[] )
{
	self->red = values[0]->xFloat.value;
	self->green = values[1]->xFloat.value;
	self->blue = values[2]->xFloat.value;
	self->alpha = values[3]->xFloat.value;
}

static void GRAD_AddStop( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGradient *self = (DaoxGradient*) p[0];
	DaoxColor color = {0.0,0.0,0.0,0.0};
	DaoxColor_FromDaoValues( & color, p[2]->xTuple.values );
	DaoxGradient_Add( self, p[1]->xFloat.value, color );
}
static void GRAD_AddStops( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGradient *self = (DaoxGradient*) p[0];
	DaoList *stops = (DaoList*) p[1];
	int i, n = DaoList_Size( stops );
	for(i=0; i<n; ++i){
		DaoTuple *item = (DaoTuple*) DaoList_GetItem( stops, i );
		DaoxColor color = {0.0,0.0,0.0,0.0};
		DaoxColor_FromDaoValues( & color, item->values[1]->xTuple.values );
		DaoxGradient_Add( self, item->values[0]->xFloat.value, color );
	}
}

static DaoFunctionEntry daoGradientMeths[]=
{
	{ GRAD_AddStop,  "AddStop( self: ColorGradient, at: float, color: Color ) => ColorGradient" },
	{ GRAD_AddStops,  "AddStops( self: ColorGradient, stops: list<tuple<at:float,color:Color>> ) => ColorGradient" },
	{ NULL, NULL }
};


DaoTypeCore daoGradientCore =
{
	"ColorGradient",                                   /* name */
	sizeof(DaoxGradient),                              /* size */
	{ NULL },                                          /* bases */
	NULL,                                              /* numbers */
	daoGradientMeths,                                  /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* GetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxGradient_Delete,           /* Delete */
	NULL                                               /* HandleGC */
};



static void LGRAD_SetStart( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGradient *self = (DaoxGradient*) p[0];
	self->points[0].x = p[1]->xFloat.value;
	self->points[0].y = p[2]->xFloat.value;
}
static void LGRAD_SetEnd( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGradient *self = (DaoxGradient*) p[0];
	self->points[1].x = p[1]->xFloat.value;
	self->points[1].y = p[2]->xFloat.value;
}

static DaoFunctionEntry daoLinearGradientMeths[]=
{
	{ LGRAD_SetStart,  "SetStart( self: LinearGradient, x: float, y: float )" },
	{ LGRAD_SetEnd,    "SetEnd( self: LinearGradient, x: float, y: float )" },
	{ NULL, NULL }
};


DaoTypeCore daoLinearGradientCore =
{
	"LinearGradient",                                  /* name */
	sizeof(DaoxGradient),                              /* size */
	{ & daoGradientCore, NULL },                       /* bases */
	NULL,                                              /* numbers */
	daoLinearGradientMeths,                            /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* GetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxGradient_Delete,           /* Delete */
	NULL                                               /* HandleGC */
};



static void RGRAD_SetRadius( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxGradient *self = (DaoxGradient*) p[0];
	self->radius = p[1]->xFloat.value;
}

static DaoFunctionEntry daoRadialGradientMeths[]=
{
	{ LGRAD_SetStart,  "SetCenter( self: RadialGradient, x: float, y: float )" },
	{ LGRAD_SetEnd,    "SetFocus( self: RadialGradient, x: float, y: float )" },
	{ RGRAD_SetRadius, "SetRadius( self: RadialGradient, r: float )" },
	{ NULL, NULL }
};


DaoTypeCore daoRadialGradientCore =
{
	"RadialGradient",                                  /* name */
	sizeof(DaoxGradient),                              /* size */
	{ & daoGradientCore, NULL },                       /* bases */
	NULL,                                              /* numbers */
	daoRadialGradientMeths,                            /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* GetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxGradient_Delete,           /* Delete */
	NULL                                               /* HandleGC */
};





static void BRUSH_SetStrokeWidth( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxBrush *self = (DaoxBrush*) p[0];
	self->strokeStyle.width = p[1]->xFloat.value;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void BRUSH_SetStrokeColor( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxBrush *self = (DaoxBrush*) p[0];
	self->strokeColor.red   = p[1]->xFloat.value;
	self->strokeColor.green = p[2]->xFloat.value;
	self->strokeColor.blue  = p[3]->xFloat.value;
	self->strokeColor.alpha = p[4]->xFloat.value;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void BRUSH_SetFillColor( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxBrush *self = (DaoxBrush*) p[0];
	self->fillColor.red   = p[1]->xFloat.value;
	self->fillColor.green = p[2]->xFloat.value;
	self->fillColor.blue  = p[3]->xFloat.value;
	self->fillColor.alpha = p[4]->xFloat.value;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void BRUSH_SetLineCap( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxBrush *self = (DaoxBrush*) p[0];
	self->strokeStyle.cap = p[1]->xEnum.value;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void BRUSH_SetJunction( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxBrush *self = (DaoxBrush*) p[0];
	self->strokeStyle.junction = p[1]->xEnum.value;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void BRUSH_SetDash( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxBrush *self = (DaoxBrush*) p[0];
	DaoArray *array = (DaoArray*) p[1];
	DaoxBrush_SetDashPattern( self, DaoArray_ToFloat32( array ), array->size );
	DaoArray_FromFloat32( array );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void BRUSH_SetStrokeGradient( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxBrush *self = (DaoxBrush*) p[0];
	DaoxGradient *grad = DaoxGradient_New( DAOX_GRADIENT_BASE );
	GC_Assign( & self->strokeGradient, grad );
	DaoProcess_PutValue( proc, (DaoValue*) self->strokeGradient );
}
static void BRUSH_SetFillGradient( DaoProcess *proc, DaoValue *p[], int N, int type )
{
	DaoxBrush *self = (DaoxBrush*) p[0];
	DaoxGradient *grad = DaoxGradient_New( type );
	GC_Assign( & self->fillGradient, grad );
	DaoProcess_PutValue( proc, (DaoValue*) self->fillGradient );
}
static void BRUSH_SetLinearGradient( DaoProcess *proc, DaoValue *p[], int N )
{
	BRUSH_SetFillGradient( proc, p, N, DAOX_GRADIENT_LINEAR );
}
static void BRUSH_SetRadialGradient( DaoProcess *proc, DaoValue *p[], int N )
{
	BRUSH_SetFillGradient( proc, p, N, DAOX_GRADIENT_RADIAL );
}
static void BRUSH_SetFont( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxBrush *self = (DaoxBrush*) p[0];
	DaoxFont *font = (DaoxFont*) p[1];
	DaoxBrush_SetFont( self, font, p[2]->xFloat.value );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}

static DaoFunctionEntry daoBrushMeths[]=
{
	{ BRUSH_SetStrokeWidth,  "SetStrokeWidth( self: Brush, width = 1.0 ) => Brush" },

	{ BRUSH_SetStrokeColor,  "SetStrokeColor( self: Brush, red: float, green: float, blue: float, alpha = 1.0 ) => Brush" },

	{ BRUSH_SetFillColor,  "SetFillColor( self: Brush, red: float, green: float, blue: float, alpha = 1.0 ) => Brush" },

	{ BRUSH_SetLineCap, "SetLineCap( self: Brush, cap: enum<none,flat,sharp,round> = $none ) => Brush" },
	{ BRUSH_SetJunction, "SetJunction( self: Brush, junction: enum<none,flat,sharp,round> = $sharp ) => Brush" },

	{ BRUSH_SetDash, "SetDashPattern( self: Brush, pattern = [3.0,2.0] ) => Brush" },

	{ BRUSH_SetStrokeGradient, "SetStrokeGradient( self: Brush ) => ColorGradient" },

	{ BRUSH_SetLinearGradient, "SetLinearGradient( self: Brush ) => LinearGradient" },

	{ BRUSH_SetRadialGradient, "SetRadialGradient( self: Brush ) => RadialGradient" },

	{ BRUSH_SetFont,      "SetFont( self: Brush, font: Font, size = 12.0 ) => Brush" },
	{ NULL, NULL }
};

static void DaoxBrush_HandleGC( DaoValue *p, DList *values, DList *lists, DList *maps, int remove )
{
	DaoxBrush *self = (DaoxBrush*) p;
	if( self->strokeGradient ) DList_Append( values, self->strokeGradient );
	if( self->fillGradient ) DList_Append( values, self->fillGradient );
	if( self->image ) DList_Append( values, self->image );
	if( self->font ) DList_Append( values, self->font );
	if( remove ){
		self->strokeGradient = NULL;
		self->fillGradient = NULL;
		self->image = NULL;
		self->font = NULL;
	}
}


DaoTypeCore daoBrushCore =
{
	"Brush",                                           /* name */
	sizeof(DaoxBrush),                                 /* size */
	{ NULL },                                          /* bases */
	NULL,                                              /* numbers */
	daoBrushMeths,                                     /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxBrush_Delete,              /* Delete */
	DaoxBrush_HandleGC                                 /* HandleGC */
};



static void ITEM_SetVisible( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvasNode *self = (DaoxCanvasNode*) p[0];
	self->visible = p[1]->xEnum.value;
	self->moved = 1;
}
static void ITEM_Scale( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvasNode *self = (DaoxCanvasNode*) p[0];
	self->scale *= p[1]->xFloat.value;
	self->changed = 1;
}
static void ITEM_Rotate( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvasNode *self = (DaoxCanvasNode*) p[0];
	float angle = M_PI * p[1]->xFloat.value / 180.0;
	float relative = p[2]->xInteger.value;
	float cos1 = self->rotation.x;
	float sin1 = self->rotation.y;
	float cos2 = cos( angle );
	float sin2 = sin( angle );
	if( relative ){
		self->rotation.x = cos1 * cos2 - sin1 * sin2;
		self->rotation.y = sin1 * cos2 + cos1 * sin2;
	}else{
		self->rotation.x = cos2;
		self->rotation.y = sin2;
	}
	self->moved = 1;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void ITEM_Move( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvasNode *self = (DaoxCanvasNode*) p[0];
	float x = p[1]->xFloat.value;
	float y = p[2]->xFloat.value;
	float relative = p[3]->xInteger.value;
	if( relative ){
		self->translation.x += x;
		self->translation.y += y;
	}else{
		self->translation.x = x;
		self->translation.y = y;
	}
	self->moved = 1;
	DaoProcess_PutValue( proc, (DaoValue*) self );
}

static void DaoxCanvasNode_HandleGC( DaoValue *p, DList *values, DList *lists, DList *maps, int remove )
{
	DaoxCanvasNode *self = (DaoxCanvasNode*) p;
	if( self->children ) DList_Append( lists, self->children );
	if( self->parent ) DList_Append( values, self->parent );
	if( self->brush ) DList_Append( values, self->brush );
	if( self->path ) DList_Append( values, self->path );
	if( self->mesh ) DList_Append( values, self->mesh );
	if( remove ){
		self->parent = NULL;
		self->brush = NULL;
		self->path = NULL;
		self->mesh = NULL;
	}
}

static DaoFunctionEntry daoCanvasNodeMeths[]=
{
	{ ITEM_SetVisible,
		"SetVisible( self: CanvasNode, visible = true )"
	},
	{ ITEM_Scale,
		"Scale( self: CanvasNode, ratio: float ) => CanvasNode"
	},
	{ ITEM_Rotate,
		"Rotate( self: CanvasNode, degree: float, relative = 0 ) => CanvasNode"
	},
	{ ITEM_Move,
		"Move( self: CanvasNode, x: float, y: float, relative = 0 ) => CanvasNode"
	},
	{ NULL, NULL }
};


DaoTypeCore daoCanvasNodeCore =
{
	"CanvasNode",                                      /* name */
	sizeof(DaoxCanvasNode),                            /* size */
	{ NULL },                                          /* bases */
	NULL,                                              /* numbers */
	daoCanvasNodeMeths,                                /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxCanvasNode_Delete,         /* Delete */
	DaoxCanvasNode_HandleGC                            /* HandleGC */
};



void DaoxCanvasLine_Set( DaoxCanvasLine *self, float x1, float y1, float x2, float y2 );
void DaoxCanvasRect_Set( DaoxCanvasRect *self, float x1, float y1, float x2, float y2 );
void DaoxCanvasCircle_Set( DaoxCanvasCircle *self, float x, float y, float r );
void DaoxCanvasEllipse_Set( DaoxCanvasEllipse *self, float x, float y, float rx, float ry );

static void LINE_SetData( DaoxCanvasLine *self, DaoValue *p[] )
{
	float x1 = p[0]->xFloat.value;
	float y1 = p[1]->xFloat.value;
	float x2 = p[2]->xFloat.value;
	float y2 = p[3]->xFloat.value;
	DaoxCanvasLine_Set( self, x1, y1, x2, y2 );
}
static void LINE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvasLine *self = (DaoxCanvasLine*) p[0];
	LINE_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFunctionEntry daoCanvasLineMeths[]=
{
	{ LINE_Set, "Set( self: CanvasLine, x1 = 0.0, y1 = 0.0, x2 = 1.0, y2 = 1.0 ) => CanvasLine" },
	{ NULL, NULL }
};


DaoTypeCore daoCanvasLineCore =
{
	"CanvasLine",                                      /* name */
	sizeof(DaoxCanvasLine),                            /* size */
	{ & daoCanvasNodeCore, NULL },                     /* bases */
	NULL,                                              /* numbers */
	daoCanvasLineMeths,                                /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxCanvasNode_Delete,         /* Delete */
	DaoxCanvasNode_HandleGC                            /* HandleGC */
};





#if 0
static void RECT_SetData( DaoxCanvasRect *self, DaoValue *p[] )
{
	float x1 = p[0]->xFloat.value;
	float y1 = p[1]->xFloat.value;
	float x2 = p[2]->xFloat.value;
	float y2 = p[3]->xFloat.value;
	float rx = p[4]->xFloat.value;
	float ry = p[5]->xFloat.value;
	DaoxCanvasRect_Set( self, x1, y1, x2, y2, rx, ry );
}
static void RECT_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvasRect *self = (DaoxCanvasRect*) p[0];
	RECT_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
#endif
static DaoFunctionEntry daoCanvasRectMeths[]=
{
	//{ RECT_Set,   "Set( self: CanvasRect, x1 = 0.0, y1 = 0.0, x2 = 1.0, y2 = 1.0 ) => CanvasRect" },
	{ NULL, NULL }
};


DaoTypeCore daoCanvasRectCore =
{
	"CanvasRect",                                      /* name */
	sizeof(DaoxCanvasRect),                            /* size */
	{ & daoCanvasNodeCore, NULL },                     /* bases */
	NULL,                                              /* numbers */
	daoCanvasRectMeths,                                /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxCanvasNode_Delete,         /* Delete */
	DaoxCanvasNode_HandleGC                            /* HandleGC */
};



static void CIRCLE_SetData( DaoxCanvasCircle *self, DaoValue *p[] )
{
	float x = p[0]->xFloat.value;
	float y = p[1]->xFloat.value;
	float r = p[2]->xFloat.value;
	DaoxCanvasCircle_Set( self, x, y, r );
}
static void CIRCLE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvasCircle *self = (DaoxCanvasCircle*) p[0];
	CIRCLE_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFunctionEntry daoCanvasCircleMeths[]=
{
	{ CIRCLE_Set,  "Set( self: CanvasCircle, cx = 0.0, cy = 0.0, r = 1.0 ) => CanvasCircle" },
	{ NULL, NULL }
};


DaoTypeCore daoCanvasCircleCore =
{
	"CanvasCircle",                                    /* name */
	sizeof(DaoxCanvasCircle),                          /* size */
	{ & daoCanvasNodeCore, NULL },                     /* bases */
	NULL,                                              /* numbers */
	daoCanvasCircleMeths,                              /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxCanvasNode_Delete,         /* Delete */
	DaoxCanvasNode_HandleGC                            /* HandleGC */
};





static void ELLIPSE_SetData( DaoxCanvasEllipse *self, DaoValue *p[] )
{
	float x1 = p[0]->xFloat.value;
	float y1 = p[1]->xFloat.value;
	float x2 = p[2]->xFloat.value;
	float y2 = p[3]->xFloat.value;
	DaoxCanvasEllipse_Set( self, x1, y1, x2, y2 );
}
static void ELLIPSE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvasEllipse *self = (DaoxCanvasEllipse*) p[0];
	ELLIPSE_SetData( self, p + 1 );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static DaoFunctionEntry daoCanvasEllipseMeths[]=
{
	{ ELLIPSE_Set,  "Set( self: CanvasEllipse, cx = 0.0, cy = 0.0, rx = 1.0, ry = 1.0 ) => CanvasEllipse" },
	{ NULL, NULL }
};


DaoTypeCore daoCanvasEllipseCore =
{
	"CanvasEllipse",                                   /* name */
	sizeof(DaoxCanvasEllipse),                         /* size */
	{ & daoCanvasNodeCore, NULL },                     /* bases */
	NULL,                                              /* numbers */
	daoCanvasEllipseMeths,                             /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxCanvasNode_Delete,         /* Delete */
	DaoxCanvasNode_HandleGC                            /* HandleGC */
};




static DaoFunctionEntry daoCanvasPathMeths[]=
{
	{ NULL, NULL }
};


DaoTypeCore daoCanvasPathCore =
{
	"CanvasPath",                                      /* name */
	sizeof(DaoxCanvasPath),                            /* size */
	{ & daoCanvasNodeCore, NULL },                     /* bases */
	NULL,                                              /* numbers */
	daoCanvasPathMeths,                                /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxCanvasNode_Delete,         /* Delete */
	DaoxCanvasNode_HandleGC                            /* HandleGC */
};




static DaoFunctionEntry daoCanvasTextMeths[]=
{
	{ NULL, NULL }
};


DaoTypeCore daoCanvasTextCore =
{
	"CanvasText",                                      /* name */
	sizeof(DaoxCanvasText),                            /* size */
	{ & daoCanvasNodeCore, NULL },                     /* bases */
	NULL,                                              /* numbers */
	daoCanvasTextMeths,                                /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxCanvasNode_Delete,         /* Delete */
	DaoxCanvasNode_HandleGC                            /* HandleGC */
};



static DaoFunctionEntry daoCanvasImageMeths[]=
{
	{ NULL, NULL }
};


DaoTypeCore daoCanvasImageCore =
{
	"CanvasImage",                                     /* name */
	sizeof(DaoxCanvasImage),                           /* size */
	{ & daoCanvasNodeCore, NULL },                     /* bases */
	NULL,                                              /* numbers */
	daoCanvasImageMeths,                               /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxCanvasNode_Delete,         /* Delete */
	DaoxCanvasNode_HandleGC                            /* HandleGC */
};




static void CANVAS_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = DaoxCanvas_New( NULL );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void CANVAS_SetViewport( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	self->viewport.left = p[1]->xFloat.value;
	self->viewport.right = p[2]->xFloat.value;
	self->viewport.bottom = p[3]->xFloat.value;
	self->viewport.top = p[4]->xFloat.value;
}
static void CANVAS_GetViewport( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	DaoProcess_NewFloat( proc, self->viewport.left );
	DaoProcess_NewFloat( proc, self->viewport.right );
	DaoProcess_NewFloat( proc, self->viewport.bottom );
	DaoProcess_NewFloat( proc, self->viewport.top );
	DaoProcess_PutTuple( proc, -4 );
}
static void CANVAS_SetBackground( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxColor color;
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	color.red   = p[1]->xFloat.value;
	color.green = p[2]->xFloat.value;
	color.blue  = p[3]->xFloat.value;
	color.alpha = p[4]->xFloat.value;
	DaoxCanvas_SetBackground( self, color );
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void CANVAS_AddGroup( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	DaoxCanvasLine *item = DaoxCanvas_AddGroup( self );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void CANVAS_AddLine( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	float x1 = p[1]->xFloat.value, y1 = p[2]->xFloat.value;
	float x2 = p[3]->xFloat.value, y2 = p[4]->xFloat.value;
	DaoxCanvasLine *item = DaoxCanvas_AddLine( self, x1, y1, x2, y2 );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void CANVAS_AddRect( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	float x1 = p[1]->xFloat.value, y1 = p[2]->xFloat.value;
	float x2 = p[3]->xFloat.value, y2 = p[4]->xFloat.value;
	float rx = p[5]->xFloat.value, ry = p[6]->xFloat.value;
	DaoxCanvasRect *item = DaoxCanvas_AddRect( self, x1, y1, x2, y2, rx, ry );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void CANVAS_AddCircle( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	float x = p[1]->xFloat.value, y = p[2]->xFloat.value;
	float r = p[3]->xFloat.value;
	DaoxCanvasCircle *item = DaoxCanvas_AddCircle( self, x, y, r );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void CANVAS_AddEllipse( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	float x = p[1]->xFloat.value, y = p[2]->xFloat.value;
	float rx = p[3]->xFloat.value, ry = p[4]->xFloat.value;
	DaoxCanvasEllipse *item = DaoxCanvas_AddEllipse( self, x, y, rx, ry );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void CANVAS_AddText( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	DString *text = DaoValue_TryGetString( p[1] );
	float x = p[2]->xFloat.value;
	float y = p[3]->xFloat.value;
	float a = p[4]->xFloat.value;
	DaoxCanvasText *item = DaoxCanvas_AddText( self, DString_GetData( text ), x, y, a );
	if( item == NULL ){
		DaoProcess_RaiseError( proc, NULL, "no font is set" );
		return;
	}
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void CANVAS_AddText2( DaoProcess *proc, DaoValue *p[], int N )
{
	float a = p[3]->xFloat.value;
	char *text = DaoValue_TryGetChars( p[1] );
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	DaoxPath *path = (DaoxPath*) p[2];
	DaoxCanvasText *item = DaoxCanvas_AddPathText( self, text, path, a );
	if( item == NULL ){
		DaoProcess_RaiseError( proc, NULL, "no font is set" );
		return;
	}
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void CANVAS_AddImage( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	DaoImage *image = (DaoImage*) p[1];
	float x = p[2]->xFloat.value;
	float y = p[3]->xFloat.value;
	float w = p[4]->xFloat.value;
	DaoxCanvasImage *item = DaoxCanvas_AddImage( self, image, x, y, w );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}


static void CANVAS_AddPath( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	DaoxPath *path = (DaoxPath*) p[1];
	DaoxCanvasPath *item = DaoxCanvas_AddPath( self, path );
	DaoProcess_PutValue( proc, (DaoValue*) item );
}
static void CANVAS_PushBrush( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	DaoxBrush *brush;
	int index = N > 1 ? p[1]->xInteger.value : -1;
	brush = DaoxCanvas_PushBrush( self, index );
	DaoProcess_PutValue( proc, (DaoValue*) brush );
}
static void CANVAS_PopBrush( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxCanvas *self = (DaoxCanvas*) p[0];
	DaoxCanvas_PopBrush( self );
}



static DaoFunctionEntry daoCanvasMeths[]=
{
	{ CANVAS_New,         "Canvas()" },

	{ CANVAS_SetViewport,   "SetViewport( self: Canvas, left: float, right: float, bottom: float, top: float )" },

	{ CANVAS_GetViewport,   "GetViewport( self: Canvas ) => tuple<left:float,right:float,bottom:float,top:float>" },

	{ CANVAS_SetBackground,  "SetBackground( self: Canvas, red: float, green: float, blue: float, alpha = 1.0 ) => Canvas" },

	{ CANVAS_PushBrush,   "PushBrush( self: Canvas ) => Brush" },
	{ CANVAS_PushBrush,   "PushBrush( self: Canvas, index: int ) => invar<Brush>" },

	{ CANVAS_PopBrush,    "PopBrush( self: Canvas )" },

	{ CANVAS_AddGroup,   "AddGroup( self: Canvas ) => CanvasNode" },
	{ CANVAS_AddCircle,    "AddCircle( self: Canvas, x: float, y: float, r: float ) => CanvasCircle" },
	{ CANVAS_AddLine,   "AddLine( self: Canvas, x1: float, y1: float, x2: float, y2: float ) => CanvasLine" },

	{ CANVAS_AddRect,   "AddRect( self: Canvas, x1: float, y1: float, x2: float, y2: float, rx = 0.0, ry = 0.0 ) => CanvasRect" },


	{ CANVAS_AddEllipse,   "AddEllipse( self: Canvas, x: float, y: float, rx: float, ry: float ) => CanvasEllipse" },

	{ CANVAS_AddPath,      "AddPath( self: Canvas, path: Path ) => CanvasPath" },
	{ CANVAS_AddText,      "AddText( self: Canvas, text: string, x: float, y: float, degrees = 0.0 ) => CanvasText" },

	{ CANVAS_AddText2,     "AddText( self: Canvas, text: string, path: Path, degrees = 0.0 ) => CanvasText" },

	{ CANVAS_AddImage,     "AddImage( self: Canvas, image: Image, x: float, y: float, w: float ) => CanvasImage" },
	{ NULL, NULL }
};


static void DaoxCanvas_HandleGC( DaoValue *p, DList *values, DList *lists, DList *maps, int remove )
{
	DaoxCanvas *self = (DaoxCanvas*) p;
	DList_Append( lists, self->nodes );
	DList_Append( lists, self->brushes );
	DList_Append( values, self->auxPath );
	DList_Append( values, self->pathCache );
	if( remove ) self->auxPath = NULL;
	if( remove ) self->pathCache = NULL;
}


DaoTypeCore daoCanvasCore =
{
	"Canvas",                                          /* name */
	sizeof(DaoxCanvas),                                /* size */
	{ NULL },                                          /* bases */
	NULL,                                              /* numbers */
	daoCanvasMeths,                                    /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxCanvas_Delete,             /* Delete */
	DaoxCanvas_HandleGC                                /* HandleGC */
};



static void PAINTER_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPainter *self = DaoxPainter_New();
	DaoProcess_PutValue( proc, (DaoValue*) self );
}
static void PAINTER_RenderToImage( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPainter *self = (DaoxPainter*) p[0];
	DaoxCanvas *canvas = (DaoxCanvas*) p[1];
	DaoImage *image = (DaoImage*) p[2];
	int width = p[3]->xInteger.value;
	int height = p[4]->xInteger.value;
	DaoxPainter_PaintCanvasImage( self, canvas, canvas->viewport, image, width, height );
}
static void PAINTER_Paint( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxPainter *self = (DaoxPainter*) p[0];
	DaoxCanvas *canvas = (DaoxCanvas*) p[1];
	DaoxPainter_Paint( self, canvas, canvas->viewport );
}
static DaoFunctionEntry daoPainterMeths[]=
{
	{ PAINTER_New,            "Painter()" },
	{ PAINTER_RenderToImage,  "RenderToImage( self: Painter, canvas: Canvas, image: Image, width: int, height: int )" },
	{ PAINTER_Paint,  "Paint( self: Painter, canvas: Canvas )" },
	{ NULL, NULL }
};
static void DaoxPainter_HandleGC( DaoValue *p, DList *values, DList *lists, DList *maps, int remove )
{
	DaoxPainter *self = (DaoxPainter*) p;
}

DaoTypeCore daoPainterCore =
{
	"Painter",                                         /* name */
	sizeof(DaoxPainter),                               /* size */
	{ NULL },                                          /* bases */
	NULL,                                              /* numbers */
	daoPainterMeths,                                   /* methods */
	DaoCstruct_CheckGetField,  DaoCstruct_DoGetField,  /* GetField */
	NULL,                      NULL,                   /* SetField */
	NULL,                      NULL,                   /* GetItem */
	NULL,                      NULL,                   /* SetItem */
	NULL,                      NULL,                   /* Unary */
	NULL,                      NULL,                   /* Binary */
	NULL,                      NULL,                   /* Conversion */
	NULL,                      NULL,                   /* ForEach */
	NULL,                                              /* Print */
	NULL,                                              /* Slice */
	NULL,                                              /* Compare */
	NULL,                                              /* Hash */
	NULL,                                              /* Create */
	NULL,                                              /* Copy */
	(DaoDeleteFunction) DaoxPainter_Delete,            /* Delete */
	DaoxPainter_HandleGC                               /* HandleGC */
};




DaoType* daox_type_path = NULL;
DaoType *daox_type_path_mesh = NULL;
DaoType *daox_type_path_cache = NULL;

DaoType *daox_type_gradient = NULL;
DaoType *daox_type_linear_gradient = NULL;
DaoType *daox_type_radial_gradient = NULL;
DaoType *daox_type_brush = NULL;
DaoType *daox_type_canvas = NULL;

DaoType *daox_type_canvas_node = NULL;
DaoType *daox_type_canvas_line = NULL;
DaoType *daox_type_canvas_rect = NULL;
DaoType *daox_type_canvas_circle = NULL;
DaoType *daox_type_canvas_ellipse = NULL;
DaoType *daox_type_canvas_path = NULL;
DaoType *daox_type_canvas_text = NULL;
DaoType *daox_type_canvas_image = NULL;

DaoType *daox_type_painter = NULL;


DAO_CANVAS_DLL int DaoFont_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns );

DAO_CANVAS_DLL int DaoCanvas_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *nspace )
{
	DaoNamespace *sqlns = DaoVmSpace_LinkModule( vmSpace, nspace, "image" );
	DaoNamespace *ns = nspace;

	dao_vmspace_graphics = vmSpace;

	DaoFont_OnLoad( vmSpace, ns );

	DaoNamespace_DefineType( ns, "tuple<red:float,green:float,blue:float,alpha:float>", "Color" );

	daox_type_path = DaoNamespace_WrapType( ns, & daoPathCore, DAO_CSTRUCT, 0 );
	daox_type_path_mesh = DaoNamespace_WrapType( ns, & daoPathMeshCore, DAO_CSTRUCT, 0 );
	daox_type_path_cache = DaoNamespace_WrapType( ns, & daoPathCacheCore, DAO_CSTRUCT, 0 );

	daox_type_gradient = DaoNamespace_WrapType( ns, & daoGradientCore, DAO_CSTRUCT, 0 );
	daox_type_linear_gradient = DaoNamespace_WrapType( ns, & daoLinearGradientCore, DAO_CSTRUCT, 0 );
	daox_type_radial_gradient = DaoNamespace_WrapType( ns, & daoRadialGradientCore, DAO_CSTRUCT, 0 );

	daox_type_brush = DaoNamespace_WrapType( ns, & daoBrushCore, DAO_CSTRUCT, 0 );
	daox_type_canvas_node = DaoNamespace_WrapType( ns, & daoCanvasNodeCore, DAO_CSTRUCT, 0 );
	daox_type_canvas_line = DaoNamespace_WrapType( ns, & daoCanvasLineCore, DAO_CSTRUCT, 0 );
	daox_type_canvas_rect = DaoNamespace_WrapType( ns, & daoCanvasRectCore, DAO_CSTRUCT, 0 );
	daox_type_canvas_circle = DaoNamespace_WrapType( ns, & daoCanvasCircleCore, DAO_CSTRUCT, 0 );
	daox_type_canvas_ellipse = DaoNamespace_WrapType( ns, & daoCanvasEllipseCore, DAO_CSTRUCT, 0 );
	daox_type_canvas_path = DaoNamespace_WrapType( ns, & daoCanvasPathCore, DAO_CSTRUCT, 0 );
	daox_type_canvas_text = DaoNamespace_WrapType( ns, & daoCanvasTextCore, DAO_CSTRUCT, 0 );
	daox_type_canvas_image = DaoNamespace_WrapType( ns, & daoCanvasImageCore, DAO_CSTRUCT, 0 );

	daox_type_canvas = DaoNamespace_WrapType( ns, & daoCanvasCore, DAO_CSTRUCT, 0 );
	daox_type_painter = DaoNamespace_WrapType( ns, & daoPainterCore, DAO_CSTRUCT, 0 );

	return 0;
}

