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


#include <string.h>
#include "dao_font.h"

typedef unsigned long  ulong_t;



enum DaoxTTPlatformIDs
{
   DAOTT_PLATFORM_ID_UNICODE   = 0,
   DAOTT_PLATFORM_ID_MAC       = 1,
   DAOTT_PLATFORM_ID_ISO       = 2,
   DAOTT_PLATFORM_ID_MICROSOFT = 3
};

enum DaoxTTUnicodeEncodings
{
	DAOTT_UNICODE_DEFAULT = 0,
	DAOTT_UNICODE_1_1     = 1,
	DAOTT_UNICODE_ISO     = 2,
	DAOTT_UNICODE_2_0     = 3
};




short daox_tt_short( uchar_t *data )
{
	return (data[0]<<8) + data[1];
}
ushort_t daox_tt_ushort( uchar_t *data )
{
	return (data[0]<<8) + data[1];
}
long daox_tt_long( uchar_t *data )
{
	return (data[0]<<24) + (data[1]<<16) + (data[2]<<8) + data[3];
}
ulong_t daox_tt_ulong( uchar_t *data )
{
	return (data[0]<<24) + (data[1]<<16) + (data[2]<<8) + data[3];
}


DaoxFont* DaoxFont_New()
{
	DaoxFont *self = (DaoxFont*) calloc( 1, sizeof(DaoxFont) );
	DaoCdata_InitCommon( (DaoCdata*)self, daox_type_font );
	self->buffer = DString_New(0);
	self->glyphs = DMap_New(0,0);
	return self;
}
void DaoxFont_Delete( DaoxFont *self )
{
	DNode *it;
	for(it=DMap_First(self->glyphs); it; it=DMap_Next(self->glyphs,it)){
		DaoxGlyph_Delete( (DaoxGlyph*) it->value.pVoid );
	}
	DMap_Delete( self->glyphs );
	DString_Delete( self->buffer );
	DaoCdata_FreeCommon( (DaoCdata*) self );
	dao_free( self );
}

void DaoxFont_ResetGlyphs( DaoxFont *self )
{
	DNode *it;
	for(it=DMap_First(self->glyphs); it; it=DMap_Next(self->glyphs,it)){
		DaoxGlyph *glyph = (DaoxGlyph*) it->value.pVoid;
		glyph->outline->commands->count = 0;
		glyph->outline->points->count = 0;
	}
}
int DaoxFont_Open( DaoxFont *self, const char *file )
{
	FILE *fin = fopen( file, "r" );
	char tag[5] = {0};
	uchar_t *cmap;
	int numberSubtables;
	int i, numTables;

	DaoxFont_ResetGlyphs( self );
	DString_Reset( self->buffer, 0 );
	DaoFile_ReadAll( fin, self->buffer, 1 );
	self->fontData = (uchar_t*)DString_GetMBS( self->buffer );

	numTables = daox_tt_ushort( self->fontData + 4 );
	printf( "numTables = %i\n", numTables );
	for(i=0; i<numTables; ++i){
		uchar_t *dir = self->fontData + 12 + 16*i;
		strncpy( tag, (char*) dir, 4 );
		printf( "%3i: %s\n", (int)i, tag );
	}
#if 0
#endif

	self->head = DaoxFont_FindTable( self, "head" );
	self->cmap = DaoxFont_FindTable( self, "cmap" );
	self->loca = DaoxFont_FindTable( self, "loca" );
	self->glyf = DaoxFont_FindTable( self, "glyf" );
	if( self->head < 0 || self->cmap < 0 || self->loca < 0 || self->glyf < 0 ) return 0;

	self->indexToLocFormat = daox_tt_ushort( self->fontData + self->head + 50 );
	printf( "%i\n", self->cmap );

	self->enc_map = -1;
	cmap = self->fontData + self->cmap;
	numberSubtables = daox_tt_ushort( cmap + 2 );
	printf( "numberSubtables = %i\n", numberSubtables );
	for(i=0; i<numberSubtables; ++i){
		ushort_t pid = daox_tt_ushort( cmap + 4 + 8*i );
		ushort_t enc = daox_tt_ushort( cmap + 4 + 8*i + 2 );
		if( pid == DAOTT_PLATFORM_ID_UNICODE && enc == DAOTT_UNICODE_2_0 ){
			self->enc_map = self->cmap + daox_tt_ulong( cmap + 4 + 8*i + 4 );
			break;
		}
	}
	printf( "enc_map = %i\n", self->enc_map );
	if( self->enc_map < 0 ) return 0;
	self->enc_format = daox_tt_ushort( self->fontData + self->enc_map );
	printf( "format = %i\n", (int) self->enc_format );

	return 1;
}
int DaoxFont_FindTable( DaoxFont *self, const char *tag )
{
	int numTables = daox_tt_ushort( self->fontData + 4 );
	int cmp, mid, first = 0, last = numTables - 1;

	while( first <= last ){
		mid = (first + last) / 2;
		cmp = strncmp( tag, (char*) self->fontData + 12 + 16*mid, 4 );
		if( cmp == 0 ){
			return daox_tt_ulong( self->fontData + 12 + 16 * mid + 8 );
		}else if( cmp < 0 ){
			last = mid - 1;
		}else{
			first = mid + 1;
		}
	}
	return -1;
}
int DaoxFont_FindGlyphIndexF0( DaoxFont *self, wchar_t ch )
{
	return 0;
}
int DaoxFont_FindGlyphIndexF2( DaoxFont *self, wchar_t ch )
{
	return 0;
}
int DaoxFont_FindGlyphIndexF4( DaoxFont *self, wchar_t ch )
{
	uchar_t *enc_map = self->fontData + self->enc_map;
	ushort_t segCount = daox_tt_ushort( enc_map + 6 ) >> 1;
	uchar_t *endCodes = enc_map + 14;
	uchar_t *startCodes = enc_map + 16 + 2*segCount;
	uchar_t *idDeltas = enc_map + 16 + 4*segCount;
	uchar_t *idRangeOffsets = enc_map + 16 + 6*segCount;
	ushort_t code = (ushort_t) ch;
	int first = 0, last = segCount - 1;
	if( ch > 0xffff ) return 0;
	while( first <= last ){
		int i = (first + last) / 2;
		ushort_t startCode = daox_tt_ushort( startCodes + 2*i );
		ushort_t endCode = daox_tt_ushort( endCodes + 2*i );
		printf( "%6i: %6i %6i\n", i, (int)startCode, (int)endCode );
		if( code < startCode ){
			last = i - 1;
		}else if( code > endCode ){
			first = i + 1;
		}else{
			ushort_t idRangeOffset = daox_tt_ushort( idRangeOffsets + 2*i );
			ushort_t idDelta = daox_tt_ushort( idDeltas + 2*i );
			printf( "%i\n", (int) idRangeOffset );
			if( idRangeOffset == 0 ) return (idDelta + code) & 0xffff;
			return daox_tt_ushort(idRangeOffsets + 2*i + idRangeOffset / 2 + (code - startCode));
		}
	}
	return 0;
}
int DaoxFont_FindGlyphIndexF6( DaoxFont *self, wchar_t ch )
{
	return 0;
}
int DaoxFont_FindGlyphIndexF12( DaoxFont *self, wchar_t ch )
{
	return 0;
}
int DaoxFont_FindGlyphIndex( DaoxFont *self, wchar_t ch )
{
	switch( self->enc_format ){
	case 0 : return DaoxFont_FindGlyphIndexF0( self, ch );
	case 2 : return DaoxFont_FindGlyphIndexF2( self, ch );
	case 4 : return DaoxFont_FindGlyphIndexF4( self, ch );
	case 6 : return DaoxFont_FindGlyphIndexF6( self, ch );
	case 12 : return DaoxFont_FindGlyphIndexF12( self, ch );
	}
	return 0;
}
int DaoxFont_FindGlyphLocation( DaoxFont *self, wchar_t ch )
{
	int gid = DaoxFont_FindGlyphIndex( self, ch );
	printf( "%i %i GlyphIndex = %i\n", self->indexToLocFormat, self->loca, gid );
	if( self->indexToLocFormat ) return daox_tt_ulong( self->fontData + self->loca + 4*gid );
	return daox_tt_ushort( self->fontData + self->loca + 2*gid );
}
int DaoxFont_MakeGlyph( DaoxFont *self, wchar_t ch, DaoxGlyph *glyph )
{
	int gloc = DaoxFont_FindGlyphLocation( self, ch );
	printf( "%i %i\n", gloc, gloc + self->glyf );
	uchar_t *gdata = self->fontData + self->glyf + 2*gloc;
	uchar_t *simdata = gdata + 10;
	uchar_t *endPtsOfContours, *flags, *points, *xCoordinates, *yCoordinates;
	short numberOfContours = daox_tt_short( gdata );
	short instructionLength, pointCount;
	short i, dx, dy, x = 0, y = 0, flagCount = 0;
	uchar_t flag = 0;
	if( numberOfContours < 0 ) return 0; // compound glyph not supported!

	instructionLength = daox_tt_ushort( simdata + 2*numberOfContours );
	endPtsOfContours = simdata;
	flags = simdata + 2*numberOfContours + 2 + instructionLength;
	pointCount = 1 + daox_tt_ushort( endPtsOfContours + 2*numberOfContours - 2 );

	printf( "numberOfContours = %i\n", numberOfContours );
	printf( "pointCount = %i\n", pointCount );

	/* load flags and coordinates into the glyph->points: */
	glyph->points = (DaoxGlyphPoint*) realloc( glyph->points, pointCount*sizeof(DaoxGlyphPoint) );

	for(i=0; i<pointCount; ++i){
		if( flagCount == 0 ){
			flag = *flags++;
			if( flag & 8 ) flagCount = *flags++; /* repeats; */
		}else{
			flagCount -= 1;
		}
		glyph->points[i].flag = flag;
	}
	points = flags;
	for(i=0; i<pointCount; ++i){
		flag = glyph->points[i].flag;
		if( flag & 2 ){ /* 1 byte long x-coordinate: */
			dx = *points++;
			x += (flag & 16) ? dx : -dx; /* ??? */
		}else{
			if( !(flag & 16) ){
				x += daox_tt_short( points );
				points += 2;
			}
		}
		glyph->points[i].x = x;
	}
	for(i=0; i<pointCount; ++i){
		flag = glyph->points[i].flag;
		if( flag & 4 ){ /* 1 byte long x-coordinate: */
			dy = *points++;
			y += (flag & 32) ? dy : -dy;
		}else{
			if( !(flag & 32) ){
				y += daox_tt_short( points );
				points += 2;
			}
		}
		glyph->points[i].y = y;
	}

	glyph->outline->commands->count = 0;
	glyph->outline->points->count = 0;
	for(i=0; i<numberOfContours; ++i){
		int start = i == 0 ? 0 : 1 + daox_tt_ushort( endPtsOfContours + 2*(i-1) );
		int end = daox_tt_ushort( endPtsOfContours + 2*i );
		int xcur, ycur, xnext, ynext;
		int j, x0, y0, start2 = start;
		int cx0 = -1, cy0 = -1;
		flag = glyph->points[start].flag;
		x0 = glyph->points[start].x;
		y0 = glyph->points[start].y;
		if( ! (flag & 1) ){ /* off curve point: */
			cx0 = x0;
			cy0 = y0;
			x = glyph->points[start].x;
			y = glyph->points[start].y;
			if( glyph->points[start].flag & 1 ){ /* on curve: */
				x0 = x;
				y0 = y;
				start2 += 1;
			}else{ /* also off curve, interpolate on-curve point: */
				x0 = (cx0 + x) >> 1;
				y0 = (cy0 + y) >> 1;
			}
		}
		DaoxPath_MoveTo( glyph->outline, x0, y0 );
		xcur = x0;
		ycur = y0;
		for(j=start2+1; j<=end; j++){
			ushort_t next = start + ((j+1-start) % (end - start + 1));
			flag = glyph->points[j].flag;
			x = glyph->points[j].x;
			y = glyph->points[j].y;
			if( flag & 1 ){ /* on curve point: */
				DaoxPath_LineTo( glyph->outline, x - xcur, y - ycur );
				xcur = x;
				ycur = y;
			}else if( glyph->points[next].flag & 1 ){ /* on curve point: */
				xnext = glyph->points[next].x;
				ynext = glyph->points[next].y;
				DaoxPath_QuadTo( glyph->outline, x - xcur, y - ycur, xnext - xcur, ynext - ycur );
				xcur = xnext;
				ycur = ynext;
				j += 1;
			}else{ /* off curve point, interpolate on-curve point: */
				xnext = (x + glyph->points[next].x) >> 1;
				ynext = (y + glyph->points[next].y) >> 1;
				DaoxPath_QuadTo( glyph->outline, x - xcur, y - ycur, xnext - xcur, ynext - ycur );
				xcur = xnext;
				ycur = ynext;
			}
		}
		if( cx0 >= 0 ){
			DaoxPath_QuadTo( glyph->outline, cx0 - xcur, cy0 - ycur, x0 - xcur, y0 - ycur );
		}
		DaoxPath_Close( glyph->outline );
	}
#if 0
	for(i=0; i<pointCount; ++i){
		flag = glyph->points[i].flag;
		x = glyph->points[i].x;
		y = glyph->points[i].y;
		printf( "%3i: %2i %5i %5i\n", i, flag&1, x, y );
	}
#endif
	free( glyph->points );
	glyph->points = NULL;
	return 1;
}

DaoxGlyph* DaoxFont_GetGlyph( DaoxFont *self, wchar_t ch )
{
	DaoxGlyph *glyph;
	DNode *node = DMap_Find( self->glyphs, (void*)(size_t) ch );
	if( node == NULL ){
		glyph = DaoxGlyph_New();
		DaoxFont_MakeGlyph( self, ch, glyph );
		return glyph;
	}
	glyph = (DaoxGlyph*) node->value.pVoid;
	if( glyph->outline->commands->count == 0 ) DaoxFont_MakeGlyph( self, ch, glyph );
	return glyph;
}


DaoxGlyph* DaoxGlyph_New()
{
	DaoxGlyph *self = (DaoxGlyph*) calloc(1,sizeof(DaoxGlyph));
	self->outline = DaoxPath_New();
	return self;
}
void DaoxGlyph_Delete( DaoxGlyph *self )
{
	DaoxPath_Delete( self->outline );
	if( self->points ) free( self->points );
	free( self );
}







static void FONT_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxFont *self = DaoxFont_New();
	DaoProcess_PutValue( proc, (DaoValue*) self );
	if( N ) DaoxFont_Open( self, DaoValue_TryGetMBString( p[0] ) );
}
static void FONT_Open( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoxFont *self = (DaoxFont*) p[0];
	DaoProcess_PutInteger( proc, DaoxFont_Open( self, DaoValue_TryGetMBString( p[1] ) ) );
}
static DaoFuncItem DaoxFontMeths[]=
{
	{ FONT_New,     "Font( file = '' )" },
	{ FONT_Open,    "Open( self : Font, file : string ) => int" },
	{ NULL, NULL }
};

DaoTypeBase DaoxFont_Typer =
{
	"Font", NULL, NULL, (DaoFuncItem*) DaoxFontMeths, { NULL }, { NULL },
	(FuncPtrDel)DaoxFont_Delete, NULL
};


DAO_DLL int DaoFont_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_type_font = DaoNamespace_WrapType( ns, & DaoxFont_Typer, 0 );
	return 0;
}
