/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2013,2014, Limin Fu
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
// THIS SOFTWARE IS PROVIDED  BY THE COPYRIGHT HOLDERS AND  CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED  WARRANTIES,  INCLUDING,  BUT NOT LIMITED TO,  THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL  THE COPYRIGHT HOLDER OR CONTRIBUTORS  BE LIABLE FOR ANY DIRECT,
// INDIRECT,  INCIDENTAL, SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL  DAMAGES (INCLUDING,
// BUT NOT LIMITED TO,  PROCUREMENT OF  SUBSTITUTE  GOODS OR  SERVICES;  LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY OF
// LIABILITY,  WHETHER IN CONTRACT,  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// 2013-12: Danilov Aleksey, initial implementation.

#include<string.h>
#include"dao_time.h"

#ifdef WIN32
#define tzset _tzset
#define daylight _daylight
#define timezone _timezone
#define tzname _tzname
#endif

static const char timeerr[] = "Time";
static DaoType *daox_type_time = NULL;

int FloorDiv( int a, int b )
{
	return (a - (a < 0? b - 1 : 0))/b;
}

int GetJulianDay( int year, int month, int day )
{
	int a = FloorDiv(14 - month, 12);
	year += 4800 - a;
	month += 12*a - 3;
	return day + FloorDiv( 153*month + 2, 5 ) + 365*year + FloorDiv( year, 4 ) - FloorDiv( year, 100 ) + FloorDiv( year, 400 ) - 32045;
}

void FromJulianDay( int jday, int *year, int *month, int *day )
{
	int a = jday + 32044;
	int b = FloorDiv( 4*a + 3, 146097 );
	int c = a - FloorDiv( 146097*b, 4 );
	int d = FloorDiv( 4*c + 3, 1461 );
	int e = c - FloorDiv( 1461*d, 4 );
	int f = FloorDiv( 5*e + 2, 153 );
	*day = e - FloorDiv( 153*f + 2, 5 ) + 1;
	*month = f + 3 - 12*FloorDiv( f, 10 );
	*year = 100*b + d - 4800 + FloorDiv( f, 10 );
}

int LeapYear( int y )
{
	return (y%4 == 0 && y%100) || y%400 == 0;
}

int DaysInMonth( int y, int m )
{
	const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if ( m > 12 )
		return 0;
	return ( LeapYear( y ) && m == 2 )? 29 : days[m - 1];
}

DaoTime* DaoTime_New()
{
	DaoTime *res = dao_malloc( sizeof(DaoTime) );
	res->value = 0;
	res->local = 0;
	res->jday = 0;
	memset( &res->parts, 0, sizeof(struct tm) );
	return res;
}

void DaoTime_Delete( DaoTime *self )
{
	dao_free( self );
}

int DaoTime_GetTime( DaoTime *self )
{
#ifdef WIN32
	struct tm *tp = self->local? localtime( &self->value ) : gmtime( &self->value );
	if ( tp )
		self->parts = *tp;
	return tp != NULL;
#else
	if ( self->local )
		return localtime_r( &self->value, &self->parts ) != NULL;
	else
		return gmtime_r( &self->value, &self->parts ) != NULL;
#endif
}

void DaoTime_CalcJulianDay( DaoTime *self )
{
	self->jday = GetJulianDay( self->parts.tm_year + 1900, self->parts.tm_mon + 1, self->parts.tm_mday );
}

static void DaoTime_Count( DaoProcess *proc, DaoValue *p[], int N )
{
	time_t tm = time(NULL);
	if ( tm == (time_t)-1 ){
		DaoProcess_RaiseError( proc, timeerr, "Failed to get current datetime" );
		return;
	}
	DaoProcess_PutInteger( proc, tm );
}
static void DaoTime_Get( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = DaoTime_New();
	if ( time( &self->value ) == (time_t)-1 ){
		DaoProcess_RaiseError( proc, timeerr, "Failed to get current datetime" );
		DaoTime_Delete( self );
		return;
	}
	self->local = ( p[0]->xEnum.value == 0 );
	if ( !DaoTime_GetTime( self ) ){
		DaoProcess_RaiseError( proc, timeerr, "Failed to get current datetime" );
		DaoTime_Delete( self );
		return;
	}
	DaoTime_CalcJulianDay( self );
	DaoProcess_PutCdata( proc, self, daox_type_time );
}

static void DaoTime_Time( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = DaoTime_New();
	self->value = p[0]->xInteger.value;
	self->local = ( p[1]->xEnum.value == 0 );
	if ( !DaoTime_GetTime( self ) ){
		DaoProcess_RaiseError( proc, timeerr, "Invalid datetime" );
		DaoTime_Delete( self );
		return;
	}
	DaoTime_CalcJulianDay( self );
	DaoProcess_PutCdata( proc, self, daox_type_time );
}

static void DaoTime_MakeTime( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = DaoTime_New();
	self->local = 1;
	self->parts.tm_isdst = -1;
	self->parts.tm_year = p[0]->xInteger.value - 1900;
	self->parts.tm_mon = p[1]->xInteger.value - 1;
	self->parts.tm_mday = p[2]->xInteger.value;
	self->parts.tm_hour = p[3]->xInteger.value;
	self->parts.tm_min = p[4]->xInteger.value;
	self->parts.tm_sec = p[5]->xInteger.value;
	self->value = mktime( &self->parts );
	if ( self->value == (time_t)-1){
		DaoProcess_RaiseError( proc, timeerr, "Invalid datetime" );
		DaoTime_Delete( self );
		return;
	}
	DaoTime_CalcJulianDay( self );
	DaoProcess_PutCdata( proc, self, daox_type_time );
}

static int IsNum( char *str, int len )
{
	int i;
	for ( i = 0; i < len; i++ )
		if ( !isdigit( str[i] ) )
			return 0;
	return 1;
}

static int GetNum( char *str, int len )
{
	int i, res = 0, mul = 1;
	for ( i = len - 1; i >= 0; i-- ){
		res += ( str[i] - '0' )*mul;
		mul *= 10;
	}
	return res;
}

static void DaoTime_Parse( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *str = DString_Copy( p[0]->xString.value );
	DString *sdate = NULL, *stime = NULL;
	DaoTime *self = DaoTime_New();
	daoint pos;
	int del = 0;
	DString_Trim( str, 1, 1, 0 );
	pos = DString_FindChar( str, ' ', 0 );
	if ( pos >= 0 ){
		sdate = DString_New( 1 );
		stime = DString_New( 1 );
		DString_SubString( str, sdate, 0, pos );
		DString_SubString( str, stime, pos + 1, str->size - pos - 1 );
	}
	else if ( DString_FindChar( str, ':', 0 ) > 0 )
		stime = str;
	else if ( DString_FindChar( str, '-', 0 ) > 0 )
		sdate = str;
	else
		goto Error;
	self->local = 1;
	if ( time( &self->value ) == (time_t)-1 ){
		DaoProcess_RaiseError( proc, timeerr, "Failed to get current datetime" );
		DaoTime_Delete( self );
		return;
	}
	DaoTime_GetTime( self );
	self->parts.tm_isdst = -1;
	if ( sdate ){
		/* YYYY-MM-DD */
		if ( sdate->size == 10 && IsNum( sdate->chars, 4 ) && IsNum( sdate->chars + 5, 2 ) && IsNum( sdate->chars + 8, 2 ) &&
			 sdate->chars[4] == '-' && sdate->chars[7] == '-' ){
			self->parts.tm_year = GetNum( sdate->chars, 4 ) - 1900;
			self->parts.tm_mon = GetNum( sdate->chars + 5, 2 ) - 1;
			self->parts.tm_mday = GetNum( sdate->chars + 8, 2 );
		}
		/* YYYY-MM */
		else if ( sdate->size == 7 && IsNum( sdate->chars, 4 ) && IsNum( sdate->chars + 5, 2 ) && sdate->chars[4] == '-' ){
			self->parts.tm_year = GetNum( sdate->chars, 4 ) - 1900;
			self->parts.tm_mon = GetNum( sdate->chars + 5, 2 ) - 1;
			self->parts.tm_mday = 1;
		}
		/* MM-DD */
		else if ( sdate->size == 5 && IsNum( sdate->chars, 2 ) && IsNum( sdate->chars + 3, 2 ) && sdate->chars[2] == '-' ){
			self->parts.tm_mon = GetNum( sdate->chars, 2 ) - 1;
			self->parts.tm_mday = GetNum( sdate->chars + 3, 2 );
		}
		else
			goto Error;
	}
	if ( stime ){
		/* HH:MM:SS */
		if ( stime->size == 8 && IsNum( stime->chars, 2 ) && IsNum( stime->chars + 3, 2 ) && IsNum( stime->chars + 6, 2 ) && stime->chars[2] == ':' &&
			 stime->chars[5] == ':'){
			self->parts.tm_hour = GetNum( stime->chars, 2 );
			self->parts.tm_min = GetNum( stime->chars + 3, 2 );
			self->parts.tm_sec = GetNum( stime->chars + 6, 2 );
		}
		/* HH:MM */
		else if ( stime->size == 5 && IsNum( stime->chars, 2 ) && IsNum( stime->chars + 3, 2 ) && stime->chars[2] == ':'){
			 self->parts.tm_hour = GetNum( stime->chars, 2 );
			 self->parts.tm_min = GetNum( stime->chars + 3, 2 );
			 self->parts.tm_sec = 0;
		 }
		else
			goto Error;
	}
	else {
		self->parts.tm_hour = 0;
		self->parts.tm_min = 0;
		self->parts.tm_sec = 0;
	}
	self->value = mktime( &self->parts );
	if ( self->value == (time_t)-1){
		DaoProcess_RaiseError( proc, timeerr, "Invalid datetime" );
		DaoTime_Delete( self );
		return;
	}
	DaoTime_CalcJulianDay( self );
	DaoProcess_PutCdata( proc, self, daox_type_time );
	goto Clean;
Error:
	DaoProcess_RaiseError( proc, "Param", "Invalid format ('YYYY-MM-DD HH:MM:SS' or its part is required)" );
	DaoTime_Delete( self );
Clean:
	DString_Delete( str );
	if ( del ){
		DString_Delete( sdate );
		DString_Delete( stime );
	}
}

static void DaoTime_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoTime *res = DaoTime_New();
	daoint i;
	*res = *self;
	DaoProcess_PutCdata( proc, res, daox_type_time );
	if ( !res->local ){
		DaoProcess_RaiseError( proc, timeerr, "Only changing the parts of a local datetime is supported" );
		return;
	}
	for ( i = 1; i < N; i++ ){
		dao_integer val = p[i]->xTuple.values[1]->xInteger.value;
		switch ( p[i]->xTuple.values[0]->xEnum.value ){
		case 0:		res->parts.tm_year = val - 1900; break; // year
		case 1:		res->parts.tm_mon = val - 1; break; // month
		case 2:		res->parts.tm_mday = val; break; // day
		case 3:		res->parts.tm_hour = val; break; // hour
		case 4:		res->parts.tm_min = val; break; // min
		case 5:		res->parts.tm_sec = val; break; // sec
		default:	break;
		}
	}
	res->value = mktime( &res->parts );
	if ( res->value == (time_t)-1){
		DaoProcess_RaiseError( proc, timeerr, "Invalid datetime" );
		return;
	}
	DaoTime_CalcJulianDay( res );
}

static void DaoTime_Copy( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *other = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoTime *res = DaoTime_New();
	*res = *other;
	DaoProcess_PutCdata( proc, res, daox_type_time );
}

static void DaoTime_Value( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, (dao_integer)self->value );
}

static void DaoTime_Type( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutEnum( proc, self->local? "local" : "utc" );
}

static void DaoTime_Convert( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoTime *res = DaoTime_New();
	*res = *self;
	DaoProcess_PutCdata( proc, res, daox_type_time );
	res->local = ( p[1]->xEnum.value == 0 );
	DaoTime_GetTime( res );
	DaoTime_CalcJulianDay( res );
}

static void DaoTime_Second( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->parts.tm_sec );
}

static void DaoTime_Minute( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->parts.tm_min );
}

static void DaoTime_Hour( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->parts.tm_hour );
}

static void DaoTime_Day( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->parts.tm_mday );
}

static void DaoTime_Month( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->parts.tm_mon + 1 );
}

static void DaoTime_Year( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->parts.tm_year + 1900 );
}

static void DaoTime_WeekDay( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->parts.tm_wday + 1 );
}

static void DaoTime_YearDay( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, self->parts.tm_yday + 1 );
}

static void DaoTime_Zone( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTuple *res = DaoProcess_PutTuple( proc, 4 );
	tzset();
	res->values[0]->xInteger.value = daylight;
	res->values[1]->xInteger.value = timezone;
	DaoString_SetChars( &res->values[2]->xString, tzname[0] );
	DaoString_SetChars( &res->values[3]->xString, tzname[1] );
}

void DaoTime_GetTimezoneShift( DaoTime *self, char *buf, size_t size )
{
	int shift = self->local? -timezone : 0;
	int hours = shift/3600, minutes = shift - hours*3600;

	if ( minutes < 0 )
		minutes = -minutes;

	snprintf( buf, size, "%+02i:%02i", hours, minutes );
}

static void DaoTime_Format( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DString *fmt = p[1]->xString.value;
	char buf[100];
	daoint i;

	for ( i = 0; i < fmt->size; i++ )
		if ( fmt->chars[i] == '%' ){
			if ( fmt->chars[i + 1] == 't' ){
				DaoTime_GetTimezoneShift( self, buf, sizeof(buf) );
				DString_InsertChars( fmt, buf, i, 2, 0 );
				i += 5;
			}
			else
				i++;
		}

	if ( strftime( buf, sizeof(buf), fmt->chars, &self->parts ))
		DaoProcess_PutChars( proc, buf );
	else
		DaoProcess_RaiseError( proc, "Param", "Invalid format" );
}
static void DaoTime_ToString( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	char buf[100];
	if ( strftime( buf, sizeof(buf), "%Y-%m-%d %H:%M:%S ", &self->parts )){
		int len = strlen( buf );

		DaoTime_GetTimezoneShift( self, buf + len, sizeof(buf) - len );
		DaoProcess_PutChars( proc, buf );
	}
	else
		DaoProcess_RaiseError( proc, "Param", "Invalid format" );
}

static int addStringFromMap( DaoValue *self, DString *S, DaoMap *sym, const char *key, int id )
{
	DNode *node;

	if( S==NULL || sym==NULL ) return 0;
	DString_SetChars( self->xString.value, key );
	node = DMap_Find( sym->value, & self );
	if( node ){
		DaoList *list = & node->value.pValue->xList;
		if( list->type == DAO_LIST && list->value->size > id ){
			DaoValue *p = list->value->items.pValue[ id ];
			if( p->type == DAO_STRING ){
				DString_Append( S, p->xString.value );
				return 1;
			}
		}
	}
	return 0;
}

static void DaoTime_Format2( DaoProcess *proc, DaoValue *p[], int N )
{
	int  i;
	int halfday = 0;
	const int size = p[2]->xString.value->size;
	const char *format = DString_GetData( p[2]->xString.value );
	char buf[100];
	char *p1 = buf+1;
	char *p2;
	DaoMap *sym = (DaoMap*)p[1];
	DaoString *ds = DaoString_New(1);
	DaoValue *key = (DaoValue*) ds;
	DString *S;
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	struct tm *ctime = &self->parts;

	if( sym->value->size == 0 ) sym = NULL;
	S = DaoProcess_PutChars( proc, "" );

	for( i=0; i+1<size; i++ ){
		if( format[i] == '%' && ( format[i+1] == 'a' || format[i+1] == 'A' ) ){
			halfday = 1;
			break;
		}
	}
	buf[0] = '0'; /* for padding */

	for( i=0; i+1<size; i++ ){
		p2 = p1;
		p1[0] = 0;
		if( format[i] == '%' ){
			const char ch = format[i+1];
			switch( ch ){
			case 'Y' :
				sprintf( p1, "%i", ctime->tm_year+1900 );
				break;
			case 'y' :
				sprintf( p1, "%i", ctime->tm_year+1900 );
				p2 += 2;
				break;
			case 'M' :
			case 'm' :
				if( ! addStringFromMap( key, S, sym, "month", ctime->tm_mon ) ){
					sprintf( p1, "%i", ctime->tm_mon+1 );
					if( ch=='M' && p1[1]==0 ) p2 = buf; /* padding 0; */
				}else p2 = NULL;
				break;
			case 'D' :
			case 'd' :
				if( ! addStringFromMap( key, S, sym, "date", ctime->tm_mday ) ){
					sprintf( p1, "%i", ctime->tm_mday );
					if( ch=='D' && p1[1]==0 ) p2 = buf; /* padding 0; */
				}else p2 = NULL;
				break;
			case 'W' :
			case 'w' :
				if( ! addStringFromMap( key, S, sym, "week", ctime->tm_wday ) )
					sprintf( p1, "%i", ctime->tm_wday+1 );
				else p2 = NULL;
				break;
			case 'H' :
			case 'h' :
				if( halfday )
					sprintf( p1, "%i", ctime->tm_hour %12 );
				else
					sprintf( p1, "%i", ctime->tm_hour );
				if( ch=='H' && p1[1]==0 ) p2 = buf; /* padding 0; */
				break;
			case 'I' :
			case 'i' :
				sprintf( p1, "%i", ctime->tm_min );
				if( ch=='I' && p1[1]==0 ) p2 = buf; /* padding 0; */
				break;
			case 'S' :
			case 's' :
				sprintf( p1, "%i", ctime->tm_sec );
				if( ch=='S' && p1[1]==0 ) p2 = buf; /* padding 0; */
				break;
			case 'a' :
				if( ! addStringFromMap( key, S, sym, "halfday", 0 ) ){
					if( ctime->tm_hour >= 12 ) strcpy( p1, "pm" );
					else strcpy( p1, "am" );
				}else p2 = NULL;
				break;
			case 'A' :
				if( ! addStringFromMap( key, S, sym, "halfday", 1 ) ){
					if( ctime->tm_hour >= 12 ) strcpy( p1, "PM" );
					else strcpy( p1, "AM" );
				}else p2 = NULL;
				break;
			default : break;
			}
			if( p2 ) DString_AppendChars( S, p2 );
			i ++;
		}else{
			DString_AppendChar( S, format[i] );
		}
	}
	if( i+1 == size ) DString_AppendChar( S, format[i] );
	DaoString_Delete( ds );
}

static void DaoTime_Diff( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *start = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoTime *end = (DaoTime*)DaoValue_TryGetCdata( p[1] );
	DaoTuple *res = DaoProcess_PutTuple( proc, 2 );
	if ( start->local != end->local ){
		DaoProcess_RaiseError( proc, timeerr, "Start and end datetime should be of the same kind (local or utc)" );
		return;
	}
	res->values[0]->xInteger.value = end->jday - start->jday;
	res->values[1]->xInteger.value = end->value - start->value;
}

static void DaoTime_Days( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	if ( p[1]->xEnum.value == 0 )
		DaoProcess_PutInteger( proc, DaysInMonth( self->parts.tm_year + 1900, self->parts.tm_mon + 1 ) );
	else
		DaoProcess_PutInteger( proc, LeapYear( self->parts.tm_year + 1900 )? 366 : 365 );
}

static void DaoTime_Equal( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoTime *b = (DaoTime*)DaoValue_TryGetCdata( p[1] );
	DaoProcess_PutBoolean( proc, a->value == b->value );
}

static void DaoTime_NotEqual( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoTime *b = (DaoTime*)DaoValue_TryGetCdata( p[1] );
	DaoProcess_PutBoolean( proc, a->value != b->value );
}

static void DaoTime_Lesser( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoTime *b = (DaoTime*)DaoValue_TryGetCdata( p[1] );
	DaoProcess_PutBoolean( proc, a->value < b->value );
}

static void DaoTime_LessOrEq( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoTime *b = (DaoTime*)DaoValue_TryGetCdata( p[1] );
	DaoProcess_PutBoolean( proc, a->value <= b->value );
}

static void DaoTime_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	daoint years = 0;
	daoint months = 0;
	daoint days = 0;
	int i;
	int y, m, d;
	DaoTime *res = DaoTime_New();
	*res = *self;
	DaoProcess_PutCdata( proc, res, daox_type_time );
	if ( N == 0 )
		return;
	if ( p[1]->type == DAO_INTEGER ){
		res->value += p[1]->xInteger.value;
		if ( !DaoTime_GetTime( res ) )
			DaoProcess_RaiseError( proc, timeerr, "Invalid resulting datetime" );
		else
			DaoTime_CalcJulianDay( res );
		return;
	}
	if ( !res->local ){
		DaoProcess_RaiseError( proc, timeerr, "Adding years, months and days is only supported for a local datetime" );
		return;
	}
	for ( i = 1; i < N; i++ ){
		dao_integer count = p[i]->xTuple.values[1]->xInteger.value;
		switch ( p[i]->xTuple.values[0]->xEnum.value ){
		case 0:	years = count; break;
		case 1:	months = count; break;
		case 2:	days = count; break;
		}
	}
	if ( years || months ){
		res->parts.tm_year += years;
		res->parts.tm_year += months/12;
		res->parts.tm_mon += months%12;
		if ( res->parts.tm_mon > 11 ){
			res->parts.tm_year++;
			res->parts.tm_mon -= 12;
		}
		else if ( res->parts.tm_mon < 0 ){
			res->parts.tm_year--;
			res->parts.tm_mon = 12 - res->parts.tm_mon;
		}
		d = DaysInMonth( res->parts.tm_year + 1900, res->parts.tm_mon + 1 );
		if ( res->parts.tm_mday > d ){
			days += d - res->parts.tm_mday;
			res->parts.tm_mday = d;
		}
		DaoTime_CalcJulianDay( res );
	}
	if ( days ){
		res->jday += days;
		FromJulianDay( res->jday, &y, &m, &d );
		res->parts.tm_year = y - 1900;
		res->parts.tm_mon = m - 1;
		res->parts.tm_mday = d;
	}
	res->value = mktime( &res->parts );
	if ( res->value == (time_t)-1 )
		DaoProcess_RaiseError( proc, timeerr, "Invalid resulting datetime" );
}

static DaoFuncItem timeMeths[] =
{
	/*! Sets one or more datetime parts using named values and returns new datetime object representing the result.
	 *
	 * \note Only setting parts of a local datetime is supported */
	{ DaoTime_Set,		"with(self: DateTime, ...: tuple<enum<year,month,day,hour,min,sec>,int>) => DateTime" },

	/*! \c time_t value representing date and time information (number of seconds elapsed since the Epoch,
	 * 1970-01-01 00:00:00, UTC) */
	{ DaoTime_Value,	".value(self: DateTime) => int" },
	{ DaoTime_Value,	"(int)(self: DateTime, hashing = false)" },

	/*! Returns datetime kind with regard to the time zone: UTC or local */
	{ DaoTime_Type,		".kind(self: DateTime) => enum<local,utc>" },

	/*! Converts datetime to the given \a kind and returns new datetime object representing the result */
	{ DaoTime_Convert,	"convert(self: DateTime, kind: enum<local,utc>) => DateTime" },

	/*! Specific datetime part */
	{ DaoTime_Second,	".sec(self: DateTime) => int" },
	{ DaoTime_Minute,	".min(self: DateTime) => int" },
	{ DaoTime_Hour,		".hour(self: DateTime) => int" },
	{ DaoTime_Day,		".day(self: DateTime) => int" },
	{ DaoTime_Month,	".month(self: DateTime) => int" },
	{ DaoTime_Year,		".year(self: DateTime) => int" },

	/*! Day of week */
	{ DaoTime_WeekDay,	".wday(self: DateTime) => int" },

	/*! Day of year */
	{ DaoTime_YearDay,	".yday(self: DateTime) => int" },

	/*! Returns datetime formatted to string using \a format, which follows the rules for C \c strftime() with the exception of
	 * '%t' serving as time zone offset specifier
	 *
	 * \warning Available format specifiers are platform-dependent. */
	{ DaoTime_Format,	"format(self: DateTime, format = '%Y-%m-%d %H:%M:%S %t') => string" },

	/*! Converts datetime to string; identical to calling \c format() with default format string */
	{ DaoTime_ToString,	"(string)(self: DateTime)" },

	/*! Returns datetime formatted to string using template \a format. \a names can specify custome names for months
	 * ('month' => {<12 names>}), days of week ('week' => {<7 names>}), days of year ('day' => {<365/366 names>}) or
	 * halfday names ('halfday' => {<2 names>}) */
	{ DaoTime_Format2,	"format(self: DateTime, invar names: map<string,list<string>>, format = '%Y-%M-%D, %H:%I:%S' ) => string" },

	/*! Returns the number of day in the month or year of the given datetime depending on the \a period parameter */
	{ DaoTime_Days,		"days(self: DateTime, period: enum<month,year>) => int" },

	/*! Adds the specified number of years, months or days (provided as named values) to the given datetime and returns new datetime
	 *  object representing the result.
	 *
	 * \note Adding years, months and days is only supported for a local datetime*/
	{ DaoTime_Add,		"add(self: DateTime, ...: tuple<enum<years,months,days>,int>) => DateTime" },

	/*! Adds the specified number of \a seconds to the given datetime (may be used for both local and global datetime)
	 * And returns new datetime object representing the result	*/
	{ DaoTime_Add,		"add(self: DateTime, seconds: int) => DateTime" },

	/*! Datetime comparison */
	{ DaoTime_Equal,	"==(a: DateTime, b: DateTime) => bool" },
	{ DaoTime_NotEqual,	"!=(a: DateTime, b: DateTime) => bool" },
	{ DaoTime_Lesser,	"<(a: DateTime, b: DateTime) => bool" },
	{ DaoTime_LessOrEq,	"<=(a: DateTime, b: DateTime) => bool" },
	{ NULL, NULL }
};

/*! Represents date and time information */
DaoTypeBase timeTyper = {
	"DateTime", NULL, NULL, timeMeths, {NULL}, {0},
	(FuncPtrDel)DaoTime_Delete, NULL
};

static DaoFuncItem timeFuncs[] =
{
	/*! Returns the number of seconds passed since the epoch time (1970-1-1, 00:00:00 UTC) */
	{ DaoTime_Count,	"count() => int" }, /* TODO: better name? */

	/*! Returns current datetime of the given \a kind */
	{ DaoTime_Get,		"now(kind: enum<local,utc> = $local) => DateTime" },

	/*! Returns datetime of the given \a kind corresponding to \c time_t \a value */
	{ DaoTime_Time,		"fromValue(value: int, kind: enum<local,utc> = $local) => DateTime" },

	/*! Returns local datetime composed of the specified \a year, \a month, \a day, \a hour, \a min and \a sec */
	{ DaoTime_MakeTime,	"make(year: int, month: int, day: int, hour = 0, min = 0, sec = 0) => DateTime" },

	/*! Returns local datetime parsed from \a value, which should contain date ('YYYY-MM-DD', 'YYYY-MM' or 'MM-DD')
	 * and/or time ('HH:MM:SS' or 'HH:MM') separated by ' ' */
	{ DaoTime_Parse,	"parse(value: string) => DateTime" },

	/*! Returns \a other copy */
	{ DaoTime_Copy,		"copy(other: DateTime) => DateTime" },

	/*! Returns the difference between \a start and \a end datetime in days and seconds.
	 *
	 * \note \a start and \a end should be of the same kind */
	{ DaoTime_Diff,		"diff(start: DateTime, end: DateTime) => tuple<days: int, seconds: int>" },

	/*! Returns local time zone information (environment variable *TZ*):
	 * -\c dst -- is Daylight Saving Time (DST) used
	 * -\c shift -- shift in seconds from GMT;
	 * -\c name -- zone name
	 * -\c dstName -- DST zone name
	 *
	 * \note You can set *TZ* using `sys.putenv()` in order to work with datetime from a different time zone */
	{ DaoTime_Zone,		"zone() => tuple<dst: int, shift: int, name: string, dstName: string>" },
	{ NULL, NULL }
};

DAO_DLL int DaoTime_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *timens = DaoNamespace_GetNamespace( ns, "time" );
	daox_type_time = DaoNamespace_WrapType( timens, &timeTyper, DAO_CTYPE_INVAR|DAO_CTYPE_OPAQUE );
	DaoNamespace_WrapFunctions( timens, timeFuncs );
	return 0;
}
