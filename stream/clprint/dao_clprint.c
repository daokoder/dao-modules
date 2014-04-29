/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011,2012, Limin Fu
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

#include<string.h>
#include<stdio.h>

#include"dao.h"
#include"daoStream.h"

#ifdef WIN32

#include<windows.h>
#include<io.h>

int SetCharForeground( DaoStream *stream, int color, int mbs )
{
	WORD attr;
	int res = 0;
	struct _CONSOLE_SCREEN_BUFFER_INFO info;
	HANDLE fd = (HANDLE)_get_osfhandle( _fileno( DaoStream_GetFile( stream ) ) );
	if( fd == INVALID_HANDLE_VALUE )
		fd = GetStdHandle( STD_OUTPUT_HANDLE );
	if( !GetConsoleScreenBufferInfo( fd, &info ) )
		return -1;
	attr = info.wAttributes;
	if( attr & FOREGROUND_BLUE )
		res |= 1;
	if( attr & FOREGROUND_GREEN )
		res |= 2;
	if( attr & FOREGROUND_RED )
		res |= 4;
	attr = attr & ~FOREGROUND_BLUE & ~FOREGROUND_GREEN & ~FOREGROUND_RED;
	if( color & 1 )
		attr |= FOREGROUND_BLUE;
	if( color & 2 )
		attr |= FOREGROUND_GREEN;
	if( color & 4 )
		attr |= FOREGROUND_RED;
	if( !SetConsoleTextAttribute( fd, attr ) )
		return -1;
	return res;
}

int SetCharBackground( DaoStream *stream, int color, int mbs )
{
	WORD attr;
	int res = 0;
	struct _CONSOLE_SCREEN_BUFFER_INFO info;
	HANDLE fd = (HANDLE)_get_osfhandle( _fileno( DaoStream_GetFile( stream ) ) );
	if( fd == INVALID_HANDLE_VALUE )
		fd = GetStdHandle( STD_OUTPUT_HANDLE );
	if( !GetConsoleScreenBufferInfo( fd, &info ) )
		return -1;
	attr = info.wAttributes;
	if( attr & BACKGROUND_BLUE )
		res |= 1;
	if( attr & BACKGROUND_GREEN )
		res |= 2;
	if( attr & BACKGROUND_RED )
		res |= 4;
	attr = attr & ~BACKGROUND_BLUE & ~BACKGROUND_GREEN & ~BACKGROUND_RED;
	if( color & 1 )
		attr |= BACKGROUND_BLUE;
	if( color & 2 )
		attr |= BACKGROUND_GREEN;
	if( color & 4 )
		attr |= BACKGROUND_RED;
	if( !SetConsoleTextAttribute( fd, attr ) )
		return -1;
	return res;
}

#else

#define CSI_RESET "\033[0m"
#define CSI_FCOLOR "\033[3%im"
#define CSI_BCOLOR "\033[4%im"

#define CSI_LRESET L"\033[0m"
#define CSI_LFCOLOR L"\033[3%im"
#define CSI_LBCOLOR L"\033[4%im"

int SetCharForeground( DaoStream *stream, int color, int mbs )
{
	char buf[20];
	wchar_t wbuf[20];
	if( color == -2 )
		snprintf( buf, sizeof( buf ), CSI_RESET );
	else
		snprintf( buf, sizeof( buf ), CSI_FCOLOR, color );
	DaoStream_WriteChars( stream, buf );
	return -2;
}

int SetCharBackground( DaoStream *stream, int color, int mbs )
{
	char buf[20];
	wchar_t wbuf[20];
	if( color == -2 )
		snprintf( buf, sizeof( buf ), CSI_RESET );
	else
		snprintf( buf, sizeof( buf ), CSI_BCOLOR, color );
	DaoStream_WriteChars( stream, buf );
	return -2;
}

#endif

int ParseColor( DaoProcess *proc, char *mbs, int n )
{
#ifdef WIN32
	const char* colors[8] = {"black", "blue", "green", "cyan", "red", "magenta", "yellow", "white"};
#else
	const char* colors[8] = {"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"};
#endif
	int i;
	if( n )
		for( i = 0; i < 8; i++ )
			if( strlen( colors[i] ) == n && !strncmp( colors[i], mbs, n ) )
				return i;
	return -1;
}

int ParseColorW( DaoProcess *proc, wchar_t *wcs, int n )
{
#ifdef WIN32
	const wchar_t* colors[8] = {L"black", L"blue", L"green", L"cyan", L"red", L"magenta", L"yellow", L"white"};
#else
	const wchar_t* colors[8] = {L"black", L"red", L"green", L"yellow", L"blue", L"magenta", L"cyan", L"white"};
#endif
	int i;
	if( n )
		for( i = 0; i < 8; i++ )
			if( wcslen( colors[i] ) == n && !wcsncmp( colors[i], wcs, n ) )
				return i;
	return -1;
}

static void DaoColorPrint( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoStream *stream = DaoValue_CastStream( p[0] );
	DString *fmt = DaoString_Get( DaoValue_CastString( p[1] ) );
	DString *str = DString_New();
	int pos, prevpos, colorf, colorb, pos2;
	char *mbs;
	wchar_t *wcs;
	/*MBS*/
	mbs = DString_GetData( fmt );
	for( pos = 0; pos < DString_Size( fmt ); pos += 2 ){
		colorf = colorb = 0;
		prevpos = pos;
		pos = DString_FindChar( fmt, '#', pos );
		if( pos == -1 ){
			DString_SetBytes( str, mbs + prevpos, DString_Size( fmt ) - prevpos );
			DaoStream_WriteString( stream, str );
			break;
		}
		else{
			DString_SetBytes( str, mbs + prevpos, pos - prevpos );
			DaoStream_WriteString( stream, str );
			if( mbs[pos + 1] == '#' ){
				DaoStream_WriteChars( stream, "#" );
				continue;
			}
			prevpos = DString_FindChar( fmt, '(', pos + 1 );
			if( prevpos == -1 ){
				DaoProcess_RaiseException( proc, DAO_WARNING, "Colored block: opening bracket not found!" );
				break;
			}
			pos2 = DString_FindChar( fmt, ':', pos + 1 );
			if( pos2 >= 0 && pos2 < prevpos ){
				if( pos2 == pos + 1 )
					colorf = -1;
				if( pos2 == prevpos - 1 )
					colorb = -1;
			}
			else{
				pos2 = prevpos;
				colorb = -1;
			}
			if( colorf != -1 )
				colorf = ParseColor( proc, mbs + pos + 1, pos2 - pos - 1 );
			if( colorb != -1 )
				colorb = ParseColor( proc, mbs + pos2 + 1, prevpos - pos2 - 1 );
			prevpos++;
			pos = DString_FindChars( fmt, ")#", prevpos );
			if( pos == -1 ){
				DaoProcess_RaiseException( proc, DAO_WARNING, "Colored block: bracket not closed!" );
				break;
			}
			else if( pos == prevpos )
				continue;
			if( colorf != -1 )
				colorf = SetCharForeground( stream, colorf, 1 );
			if( colorb != -1 )
				colorb = SetCharBackground( stream, colorb, 1 );
			DString_SetBytes( str, mbs + prevpos, pos - prevpos );
			DaoStream_WriteString( stream, str );
			if( colorb != -1 )
				colorb = SetCharBackground( stream, colorb, 1 );
			if( colorf != -1 )
				colorf = SetCharForeground( stream, colorf, 1 );
		}
	}
	DString_Delete( str );
}

DAO_DLL int DaoOnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace_WrapFunction( ns, (DaoCFunction)DaoColorPrint, "clprint( self: io::stream, format: string )" );
	return 0;
}
