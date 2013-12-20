/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011-2013, Limin Fu
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

#include<string.h>
#include"dao_time.h"

#ifdef WIN32
#define tzset _tzset
#define daylight _daylight
#define timezone _timezone
#define tzname _tzname
#endif

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

static void DaoTime_Get( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = DaoTime_New();
	if ( time( &self->value ) == (time_t)-1 ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "Failed to get current time" );
		DaoTime_Delete( self );
		return;
	}
	self->local = ( p[0]->xEnum.value == 0 );
	if ( !DaoTime_GetTime( self ) ){
		DaoProcess_RaiseException( proc, DAO_ERROR, "Failed to get current time" );
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
		DaoProcess_RaiseException( proc, DAO_ERROR, "Invalid time" );
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
		DaoProcess_RaiseException( proc, DAO_ERROR, "Invalid time" );
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
	DString *str = DString_Copy( p[0]->xString.data );
	DString *sdate = NULL, *stime = NULL;
	DaoTime *self = DaoTime_New();
	daoint pos;
	int del = 0;
	DString_Trim( str );
	DString_ToMBS( str );
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
		DaoProcess_RaiseException( proc, DAO_ERROR, "Failed to get current time" );
		DaoTime_Delete( self );
		return;
	}
	DaoTime_GetTime( self );
	self->parts.tm_isdst = -1;
	if ( sdate ){
		/* YYYY-MM-DD */
		if ( sdate->size == 10 && IsNum( sdate->mbs, 4 ) && IsNum( sdate->mbs + 5, 2 ) && IsNum( sdate->mbs + 8, 2 ) &&
			 sdate->mbs[4] == '-' && sdate->mbs[7] == '-' ){
			self->parts.tm_year = GetNum( sdate->mbs, 4 ) - 1900;
			self->parts.tm_mon = GetNum( sdate->mbs + 5, 2 ) - 1;
			self->parts.tm_mday = GetNum( sdate->mbs + 8, 2 );
		}
		/* YYYY-MM */
		else if ( sdate->size == 7 && IsNum( sdate->mbs, 4 ) && IsNum( sdate->mbs + 5, 2 ) && sdate->mbs[4] == '-' ){
			self->parts.tm_year = GetNum( sdate->mbs, 4 ) - 1900;
			self->parts.tm_mon = GetNum( sdate->mbs + 5, 2 ) - 1;
			self->parts.tm_mday = 1;
		}
		/* MM-DD */
		else if ( sdate->size == 5 && IsNum( sdate->mbs, 2 ) && IsNum( sdate->mbs + 3, 2 ) && sdate->mbs[2] == '-' ){
			self->parts.tm_mon = GetNum( sdate->mbs, 2 ) - 1;
			self->parts.tm_mday = GetNum( sdate->mbs + 3, 2 );
		}
		else
			goto Error;
	}
	if ( stime ){
		/* HH:MM:SS */
		if ( stime->size == 8 && IsNum( stime->mbs, 2 ) && IsNum( stime->mbs + 3, 2 ) && IsNum( stime->mbs + 6, 2 ) && stime->mbs[2] == ':' &&
			 stime->mbs[5] == ':'){
			self->parts.tm_hour = GetNum( stime->mbs, 2 );
			self->parts.tm_min = GetNum( stime->mbs + 3, 2 );
			self->parts.tm_sec = GetNum( stime->mbs + 6, 2 );
		}
		/* HH:MM */
		else if ( stime->size == 5 && IsNum( stime->mbs, 2 ) && IsNum( stime->mbs + 3, 2 ) && stime->mbs[2] == ':'){
			 self->parts.tm_hour = GetNum( stime->mbs, 2 );
			 self->parts.tm_min = GetNum( stime->mbs + 3, 2 );
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
		DaoProcess_RaiseException( proc, DAO_ERROR, "Invalid time" );
		DaoTime_Delete( self );
		return;
	}
	DaoTime_CalcJulianDay( self );
	DaoProcess_PutCdata( proc, self, daox_type_time );
	goto Clean;
Error:
	DaoProcess_RaiseException( proc, DAO_ERROR, "Invalid format ('YYYY-MM-DD HH:MM:SS' or its part is required)" );
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
	daoint year = p[1]->xInteger.value;
	daoint month = p[2]->xInteger.value;
	daoint day = p[3]->xInteger.value;
	daoint hour = p[4]->xInteger.value;
	daoint minute = p[5]->xInteger.value;
	daoint second = p[6]->xInteger.value;
	time_t value;
	struct tm old = self->parts;
	if ( year >= 0 ) self->parts.tm_year = year - 1900;
	if ( month > 0 ) self->parts.tm_mon = month - 1;
	if ( day > 0 ) self->parts.tm_mday = day;
	if ( hour >= 0 ) self->parts.tm_hour = hour;
	if ( minute >= 0 ) self->parts.tm_min = minute;
	if ( second >= 0 ) self->parts.tm_sec = second;
	value = mktime( &self->parts );
	if ( value == (time_t)-1){
		self->parts = old;
		DaoProcess_RaiseException( proc, DAO_ERROR, "Invalid time" );
		return;
	}
	self->value = value;
	DaoTime_CalcJulianDay( self );
}

static void DaoTime_Copy( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = DaoTime_New();
	DaoTime *other = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	self->value = other->value;
	self->jday = other->jday;
	self->parts = other->parts;
	self->local = other->local;
	DaoProcess_PutCdata( proc, self, daox_type_time );
}

static void DaoTime_Value( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, (daoint)self->value );
}

static void DaoTime_Type( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutEnum( proc, self->local? "local" : "utc" );
}

static void DaoTime_Convert( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	self->local = ( p[1]->xEnum.value == 0 );
	DaoTime_GetTime( self );
	DaoTime_CalcJulianDay( self );
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
	res->items[0]->xInteger.value = daylight;
	res->items[1]->xInteger.value = timezone;
	DaoString_SetMBS( &res->items[2]->xString, tzname[0] );
	DaoString_SetMBS( &res->items[3]->xString, tzname[1] );
}

static void DaoTime_Format( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DString *fmt = p[1]->xString.data;
	if ( fmt->size ){
		if ( fmt->mbs ){
			char buf[100];
			if ( strftime( buf, sizeof(buf), fmt->mbs, &self->parts ))
				DaoProcess_PutMBString( proc, buf );
			else
				DaoProcess_RaiseException( proc, DAO_ERROR, "Invalid format" );
		}
		else {
			wchar_t buf[100];
			if ( wcsftime( buf, sizeof(buf),fmt->wcs, &self->parts ))
				DaoProcess_PutWCString( proc, buf );
			else
				DaoProcess_RaiseException( proc, DAO_ERROR, "Invalid format" );
		}
	}
	else
		DaoProcess_PutMBString( proc, asctime( &self->parts ) );
}

static int addStringFromMap( DaoValue *self, DString *S, DaoMap *sym, const char *key, int id )
{
	DNode *node;

	if( S==NULL || sym==NULL ) return 0;
	DString_SetMBS( self->xString.data, key );
	node = DMap_Find( sym->items, & self );
	if( node ){
		DaoList *list = & node->value.pValue->xList;
		if( list->type == DAO_LIST && list->items.size > id ){
			DaoValue *p = list->items.items.pValue[ id ];
			if( p->type == DAO_STRING ){
				DString_Append( S, p->xString.data );
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
	const int size = p[2]->xString.data->size;
	const char *format = DString_GetMBS( p[2]->xString.data );
	char buf[100];
	char *p1 = buf+1;
	char *p2;
	DaoMap *sym = (DaoMap*)p[1];
	DaoString *ds = DaoString_New(1);
	DaoValue *key = (DaoValue*) ds;
	DString *S;
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	struct tm *ctime = &self->parts;

	if( sym->items->size == 0 ) sym = NULL;
	S = DaoProcess_PutMBString( proc, "" );

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
			if( p2 ) DString_AppendMBS( S, p2 );
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
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoTime *other = (DaoTime*)DaoValue_TryGetCdata( p[1] );
	DaoTuple *res = DaoProcess_PutTuple( proc, 2 );
	res->items[0]->xInteger.value = other->jday - self->jday;
	res->items[1]->xDouble.value = difftime( other->value, self->value );
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
	DaoTime *b = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, a->value == b->value );
}

static void DaoTime_NotEqual( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoTime *b = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoProcess_PutInteger( proc, a->value != b->value );
}

static void DaoTime_Lesser( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoTime *b = (DaoTime*)DaoValue_TryGetCdata( p[1] );
	DaoProcess_PutInteger( proc, difftime( b->value, a->value ) > 0 );
}

static void DaoTime_LessOrEq( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	DaoTime *b = (DaoTime*)DaoValue_TryGetCdata( p[1] );
	DaoProcess_PutInteger( proc, difftime( b->value, a->value ) >= 0 );
}

static void DaoTime_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*)DaoValue_TryGetCdata( p[0] );
	daoint years = p[1]->xInteger.value;
	daoint months = p[2]->xInteger.value;
	daoint days = p[3]->xInteger.value;
	int y, m, d;
	if ( years || months ){
		self->parts.tm_year += years;
		self->parts.tm_year += months/12;
		self->parts.tm_mon += months%12;
		if ( self->parts.tm_mon > 11 ){
			self->parts.tm_year++;
			self->parts.tm_mon -= 12;
		}
		else if ( self->parts.tm_mon < 0 ){
			self->parts.tm_year--;
			self->parts.tm_mon = 12 - self->parts.tm_mon;
		}
		d = DaysInMonth( self->parts.tm_year + 1900, self->parts.tm_mon + 1 );
		if ( self->parts.tm_mday > d ){
			days += d - self->parts.tm_mday;
			self->parts.tm_mday = d;
		}
		DaoTime_CalcJulianDay( self );
	}
	if ( days ){
		self->jday += days;
		FromJulianDay( self->jday, &y, &m, &d );
		self->parts.tm_year = y - 1900;
		self->parts.tm_mon = m - 1;
		self->parts.tm_mday = d;
	}
	self->value = mktime( &self->parts );
}

static DaoFuncItem timeMeths[] =
{
	{ DaoTime_Get,		"time(type: enum<local, utc> = $local) => time" },
	{ DaoTime_Time,		"time(value: int, type: enum<local, utc> = $local) => time" },
	{ DaoTime_MakeTime,	"time(year: int, month: int, day: int, hour = 0, min = 0, sec = 0) => time" },
	{ DaoTime_Parse,	"time(value: string) => time" },
	{ DaoTime_Copy,		"time(other: time) => time" },

	{ DaoTime_Set,		"set(self: time, year = -1, month = -1, day = -1, hour = -1, min = -1, sec = -1)" },
	{ DaoTime_Value,	"value(self: time) => int" },
	{ DaoTime_Type,		"type(self: time) => enum<local, utc>" },
	{ DaoTime_Convert,	"convert(self: time, type: enum<local, utc>)" },
	{ DaoTime_Second,	"sec(self: time) => int" },
	{ DaoTime_Minute,	"min(self: time) => int" },
	{ DaoTime_Hour,		"hour(self: time) => int" },
	{ DaoTime_Day,		"day(self: time) => int" },
	{ DaoTime_Month,	"month(self: time) => int" },
	{ DaoTime_Year,		"year(self: time) => int" },
	{ DaoTime_WeekDay,	"wday(self: time) => int" },
	{ DaoTime_YearDay,	"yday(self: time) => int" },
	{ DaoTime_Format,	"format(self: time, format = '') => string" },
	{ DaoTime_Format2,	"format(self: time, names: map<string, list<string>>, format = '%Y-%M-%D, %H:%I:%S' ) => string" },
	{ DaoTime_Days,		"days(self: time, period: enum<month, year>) => int" },
	{ DaoTime_Add,		"add(self: time, years = 0, months = 0, days = 0)" },

	{ DaoTime_Equal,	"==(a: time, b: time) => int" },
	{ DaoTime_NotEqual,	"!=(a: time, b: time) => int" },
	{ DaoTime_Lesser,	"<(a: time, b: time) => int" },
	{ DaoTime_LessOrEq,	"<=(a: time, b: time) => int" },
	{ DaoTime_Diff,		"diff(start: time, end: time) => tuple<days: int, seconds: double>" },
	{ DaoTime_Zone,		"zone() => tuple<dst: int, shift: int, name: string, dst_name: string>" },
	{ NULL, NULL }
};

DaoTypeBase timeTyper = {
	"time", NULL, NULL, timeMeths, {NULL}, {0},
	(FuncPtrDel)DaoTime_Delete, NULL
};

DAO_DLL int DaoTime_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	daox_type_time = DaoNamespace_WrapType( ns, &timeTyper, 1 );
	return 0;
}
