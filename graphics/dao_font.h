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


#ifndef __DAO_FONT_H__
#define __DAO_FONT_H__

#include "daoStdtype.h"
#include "dao_geometry.h"


typedef struct DaoxFont   DaoxFont;
typedef struct DaoxGlyph  DaoxGlyph;


struct DaoxFont
{
	DAO_CDATA_COMMON;

	DString  *buffer;
	uchar_t  *fontData;

	int  head;  /* font header table; */
	int  cmap;  /* character code mapping table; */
	int  loca;  /* glyph location table; */
	int  glyf;  /* glyph outline table; */

	int      enc_map;
	uchar_t  enc_format;
	uchar_t  indexToLocFormat;

	DMap *glyphs;
};

static DaoType* daox_type_font = NULL;



DaoxFont* DaoxFont_New();
int DaoxFont_Open( DaoxFont *self, const char *file );
int DaoxFont_FindTable( DaoxFont *self, const char *tag );
int DaoxFont_FindGlyphIndex( DaoxFont *self, wchar_t ch );

DaoxGlyph* DaoxFont_GetGlyph( DaoxFont *self, wchar_t ch );


typedef struct DaoxGlyphPoint  DaoxGlyphPoint;
struct DaoxGlyphPoint
{
	uchar_t flag;
	short   x, y;
};

struct DaoxGlyph
{
	DaoxPath  *outline;

	DaoxGlyphPoint *points;
};

DaoxGlyph* DaoxGlyph_New();
void DaoxGlyph_Delete( DaoxGlyph *self );



#endif
