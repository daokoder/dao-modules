/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2013-2015, Limin Fu
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

#define DAO_TIME

#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include<limits.h>
#include<errno.h>
#include<math.h>
#include<time.h>
#include"dao_time.h"
#include"daoPlatforms.h"

#ifdef WIN32
#define tzset _tzset
#define daylight _daylight
#define timezone _timezone
#define tzname _tzname
#endif


static const char timeerr[] = "Time";
static DaoType *daox_type_date = NULL;
static DaoType *daox_type_time = NULL;
static DaoType *daox_type_date_span = NULL;
static DaoType *daox_type_time_span = NULL;

static dao_time_t epoch2000_days = 0;
static dao_time_t epoch2000_useconds = 0;
static dao_time_t epoch1970_seconds = 0;

static int FloorDiv( int a, int b )
{
	return (a - (a < 0? b - 1 : 0))/b;
}

static int GetJulianDay( int year, int month, int day )
{
	int a = FloorDiv(14 - month, 12);
	year += 4800 - a;
	month += 12*a - 3;
	return day + FloorDiv( 153*month + 2, 5 ) + 365*year + FloorDiv( year, 4 ) - FloorDiv( year, 100 ) + FloorDiv( year, 400 ) - 32045;
}

static void FromJulianDay( int jday, int *year, int *month, int *day )
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

static int LeapYear( int y )
{
	return (y%4 == 0 && y%100) || y%400 == 0;
}



DDate DDate_Today( int local )
{
	DTime time = DTime_Now( local );
	return DDate_FromTime( time );
}
DDate DDate_FromTime( DTime time )
{
	return time.date;
}
DDate DDate_FromJulianDay( int jday )
{
	DDate date = {0,0,0};
	int F = jday + 1401 + (((4*jday + 274277) / 146097) * 3) / 4 - 38;
	int E = 4*F + 3;
	int G = (E%1461) / 4;
	int H = 5*G + 2;
	date.day = (H%153) / 5 + 1;
	date.month = ((H/153 + 2) % 12) + 1;
	date.year = (E/1461) - 4716 + (12 + 2 - date.month) / 12;
	return date;
}
int DDate_ToJulianDay( DDate date )
{
	int a = FloorDiv(14 - date.month, 12);
	int year = date.year + 4800 - a;
	int month = date.month + 12*a - 3;
	int day = date.day + FloorDiv( 153*month + 2, 5 ) + 365*year;
	day += FloorDiv( year, 4 ) - FloorDiv( year, 100 ) + FloorDiv( year, 400 );
	return day - 32045;
}
DDate DDate_FromDay( int day )
{
	return DDate_FromJulianDay( epoch2000_days + day );
}
int DDate_ToDay( DDate time )
{
	return DDate_ToJulianDay( time ) - epoch2000_days;
}
int DDate_MonthDays( DDate date )
{
	const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if ( date.month > 12 ) return 0;
	return (LeapYear( date.year ) && date.month == 2) ? 29 : days[date.month - 1];
}
int DDate_Compare( DDate first, DDate second )
{
	if( first.year != second.year ) return first.year < second.year ? -1 : 1;
	if( first.month != second.month ) return first.month < second.month ? -1 : 1;
	if( first.day != second.day ) return first.day < second.day ? -1 : 1;
	return 0;
}
int DDate_IsValid( DDate date )
{
	return date.month > 0 && date.month < 13 && date.day > 0 && date.day <= DDate_MonthDays( date );
}



DTime DTime_Now( int local )
{
	DTime res = {0};
#ifdef WIN32
	SYSTEMTIME systime;
	FILETIME ftime;
	dao_time_t microsecs;
	if( local ){
		GetLocalTime( & systime );
	}else{
		GetSystemTime( & systime );
	}
	GetSystemTimeAsFileTime( & ftime );
	microsecs = (((dao_time_t) ftime.dwHighDateTime << 32) | ftime.dwLowDateTime) / 10;
	res.date.year   = systime.wYear;
	res.date.month  = systime.wMonth;
	res.date.day    = systime.wDay;
	res.hour   = systime.wHour;
	res.minute = systime.wMinute;
	res.second = systime.wSecond + (microsecs % 1000000) / 1.0E6;

#else
	struct tm parts;
	struct timeval now;
	int ret = 1;
	gettimeofday( & now, NULL );
	if( local ){
		ret = localtime_r( & now.tv_sec, & parts ) != NULL;
	}else{
		ret = gmtime_r( & now.tv_sec, & parts ) != NULL;
	}
	res.date.year   = parts.tm_year + 1900;
	res.date.month  = parts.tm_mon + 1;
	res.date.day    = parts.tm_mday;
	res.hour   = parts.tm_hour;
	res.minute = parts.tm_min;
	res.second = parts.tm_sec + now.tv_usec / 1.0E6;
#endif

	return res;
}

DTime DTime_FromDate( DDate date )
{
	DTime time = {0,0,0,0,0,0.0};
	time.date = date;
	return time;
}
DTime DTime_FromJulianDay( int jday )
{
	DDate date = DDate_FromJulianDay( jday );
	return DTime_FromDate( date );
}
int DTime_ToJulianDay( DTime time )
{
	DDate date = DDate_FromTime( time );
	return DDate_ToJulianDay( date );
}

DTime DTime_FromTime( time_t time )
{
	return DTime_FromSeconds( time + epoch1970_seconds - epoch2000_useconds/1E6 );
}
time_t DTime_ToTime( DTime time )
{
	return DTime_ToSeconds( time ) + epoch2000_useconds/1E6 - epoch1970_seconds;
}

DTime DTime_FromDay( int day )
{
	return DTime_FromJulianDay( epoch2000_days + day );
}
int DTime_ToDay( DTime time )
{
	return DTime_ToJulianDay( time ) - epoch2000_days;
}

DTime DTime_FromSeconds( dao_time_t seconds )
{
	dao_time_t seconds2 = seconds + epoch2000_useconds / 1E6;
	dao_time_t jday = seconds2 / (24*3600);
	dao_time_t daysecs = seconds2 - jday*24*3600;
	DTime time = DTime_FromJulianDay( jday );

	time.hour = daysecs / 3600;
	time.minute = (daysecs % 3600) / 60;
	time.second = daysecs % 60;
	return time;
}
dao_time_t DTime_ToSeconds( DTime time )
{
	dao_time_t jday = DTime_ToJulianDay( time );
	return jday*24*3600 + time.hour*3600 + time.minute*60 + time.second - epoch2000_useconds / 1E6;
}

DTime DTime_FromMicroSeconds( dao_time_t useconds )
{
	dao_time_t seconds = (useconds + epoch2000_useconds) / 1E6;
	dao_time_t jday = seconds / (24*3600);
	dao_time_t daysecs = seconds - jday*24*3600;
	DTime time = DTime_FromJulianDay( jday );

	time.hour = daysecs / 3600;
	time.minute = (daysecs % 3600) / 60;
	time.second = daysecs % 60 + ((useconds + epoch2000_useconds) % (dao_time_t)1E6) / 1.0E6;
	return time;
}
dao_time_t DTime_ToMicroSeconds( DTime time )
{
	dao_time_t jday = DTime_ToJulianDay( time );
	dao_time_t usec = (jday*24*3600 + time.hour*3600 + time.minute*60) * 1E6;
	return usec + (dao_time_t)(time.second*1E6) - epoch2000_useconds;
}

void DTime_ToStructTM( DTime time, struct tm *parts )
{
	parts->tm_year = time.date.year - 1900;
	parts->tm_mon  = time.date.month - 1;
	parts->tm_mday = time.date.day;
	parts->tm_hour = time.hour;
	parts->tm_min  = time.minute;
	parts->tm_sec  = time.second;
	parts->tm_isdst = -1;
}

DTime DTime_LocalToUtc( DTime local )
{
	double fract = local.second - (dao_time_t)local.second;
	DTime res;
#ifdef WIN32
	SYSTEMTIME loc = {0};
	SYSTEMTIME gmt = {0};
	loc.wYear = local.year;
	loc.wMonth = local.month;
	loc.wDay = local.day;
	loc.wHour = local.hour;
	loc.wMinute = local.minute;
	loc.wSecond = local.second;
	TzSpecificLocalTimeToSystemTime( NULL, &loc, &gmt );
	res.date.year = gmt.wYear;
	res.date.month = gmt.wMonth;
	res.date.day = gmt.wDay;
	res.hour = gmt.wHour;
	res.minute = gmt.wMinute;
	res.second = gmt.wSecond;
#else
	time_t t;
	struct tm ts = {0};
	DTime_ToStructTM( local, &ts );
	t = mktime( &ts );
	gmtime_r( &t, &ts );
	res.date.year = ts.tm_year + 1900;
	res.date.month = ts.tm_mon + 1;
	res.date.day = ts.tm_mday;
	res.hour = ts.tm_hour;
	res.minute = ts.tm_min;
	res.second = ts.tm_sec;
#endif
	res.second += fract;
	return res;
}

DTime DTime_UtcToLocal( DTime utc )
{
	double fract = utc.second - (dao_time_t)utc.second;
	DTime res;
#ifdef WIN32
	SYSTEMTIME loc = {0};
	SYSTEMTIME gmt = {0};
	gmt.wYear = utc.year;
	gmt.wMonth = utc.month;
	gmt.wDay = utc.day;
	gmt.wHour = utc.hour;
	gmt.wMinute = utc.minute;
	gmt.wSecond = utc.second;
	SystemTimeToTzSpecificLocalTime(NULL, &gmt, &loc);
	res.date.year = loc.wYear;
	res.date.month = loc.wMonth;
	res.date.day = loc.wDay;
	res.hour = loc.wHour;
	res.minute = loc.wMinute;
	res.second = loc.wSecond;
#else
	time_t t;
	struct tm ts = {0};
	DTime_ToStructTM( utc, &ts );
#if defined(LINUX) || defined(MACOSX) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
	t = timegm( &ts );
#else
	char tz[200];
	char *ev = getenv( "TZ" );
	if ( ev )
		snprintf( tz, sizeof(tz), "%s", ev );
	else
		tz[0] = 0;
	setenv( "TZ", "", 1 );
	tzset();
	t = mktime( &ts );
	if ( *tz )
		setenv( "TZ", tz, 1 );
	else
		unsetenv( "TZ" );
	tzset();
#endif
	gmtime_r( &t, &ts );
	res.date.year = ts.tm_year + 1900;
	res.date.month = ts.tm_mon + 1;
	res.date.day = ts.tm_mday;
	res.hour = ts.tm_hour;
	res.minute = ts.tm_min;
	res.second = ts.tm_sec;
#endif
	res.second += fract;
	return res;
}

int DTime_Compare( DTime first, DTime second )
{
	int msa, msb;
	if( first.date.year != second.date.year ) return first.date.year < second.date.year ? -1 : 1;
	if( first.date.month != second.date.month ) return first.date.month < second.date.month ? -1 : 1;
	if( first.date.day != second.date.day ) return first.date.day < second.date.day ? -1 : 1;
	if( first.hour != second.hour ) return first.hour < second.hour ? -1 : 1;
	if( first.minute != second.minute ) return first.minute < second.minute ? -1 : 1;
	msa = (int)(first.second*1E6);
	msb = (int)(second.second*1E6);
	if( msa != msb ) return msa < msb ? -1 : 1;
	return 0;
}

int DTime_IsValid( DTime time )
{
	return time.date.month > 0 && time.date.month < 13 && time.date.day > 0 &&
			time.date.day <= DDate_MonthDays( time.date ) && time.hour >= 0 &&
			time.hour < 24 && time.minute >= 0 && time.minute < 60 &&
			time.second >= 0.0 && time.second < 60.0;
}



DDateSpan DDateSpan_FromDates( DDate start, DDate end )
{
	DDateSpan span = {0};
	int days1 = DDate_ToDay( start );
	int days2 = DDate_ToDay( end );

	if( DDate_Compare( start, end ) > 0 ) return span;

	span.value = days2 - days1;
	span.years = end.year - start.year;
	span.months = end.month - start.month - 1;
	span.days = end.day + DDate_MonthDays( start ) - start.day; /* Add remaining days; */
	if( span.days >= DDate_MonthDays( end ) ){
		span.days -= DDate_MonthDays( end );
		span.months += 1;
	}
	if( span.months < 0 ){
		span.months += 12;
		span.years -= 1;
	}
	return span;
}
int DDateSpan_Compare( DDateSpan first, DDateSpan second )
{
	if( first.years != second.years ) return first.years < second.years ? -1 : 1;
	if( first.months != second.months ) return first.months < second.months ? -1 : 1;
	if( first.days != second.days ) return first.days < second.days ? -1 : 1;
	if( first.value != second.value ) return first.value < second.value ? -1 : 1;
	return 0;
}



DTimeSpan DTimeSpan_FromUSeconds( dao_time_t useconds )
{
	DTimeSpan span = {0};
	dao_time_t seconds = useconds / 1E6;
	dao_time_t minutes = seconds / 60;
	dao_time_t hours = minutes / 60;

	span.seconds = (seconds % 60) + (useconds % (dao_time_t)1E6) / 1.0E6;
	span.minutes = minutes % 60;
	span.hours = hours % 24;
	span.days = hours / 24;
	return span;
}
dao_time_t DTimeSpan_ToUSeconds( DTimeSpan span )
{
	dao_time_t useconds = span.seconds * 1E6;
	useconds += (span.days * 24 * 3600 + span.hours * 3600 + span.minutes * 60) * 1E6;
	return useconds;
}
int DTimeSpan_Compare( DTimeSpan first, DTimeSpan second )
{
	int msa, msb;
	if( first.days != second.days ) return first.days < second.days ? -1 : 1;
	if( first.hours != second.hours ) return first.hours < second.hours ? -1 : 1;
	if( first.minutes != second.minutes ) return first.minutes < second.minutes ? -1 : 1;
	msa = (int)(first.seconds*1E6);
	msb = (int)(second.seconds*1E6);
	if( msa != msb ) return msa < msb ? -1 : 1;
	return 0;
}




DaoDate* DaoDate_New()
{
	DaoDate *self = (DaoDate*) DaoCpod_New( daox_type_date, sizeof(DaoDate) );
	return self;
}

void DaoDate_Delete( DaoDate *self )
{
	DaoCpod_Delete( (DaoCpod*) self );
}

int DaoDate_Today( DaoDate *self, int local )
{
	self->date = DDate_Today( local );
	return self->date.month;
}

DaoType* DaoDate_Type()
{
	return daox_type_date;
}



DaoTime* DaoTime_New()
{
	DaoTime *self = (DaoTime*) DaoCpod_New( daox_type_time, sizeof(DaoTime) );
	return self;
}

void DaoTime_Delete( DaoTime *self )
{
	DaoCpod_Delete( (DaoCpod*) self );
}

int DaoTime_Now( DaoTime *self )
{
	self->time = DTime_Now( self->local );
	return self->time.date.month;
}

DaoType* DaoTime_Type()
{
	return daox_type_time;
}

int DaoTime_Compare( DaoTime *a, DaoTime *b )
{
	DTime first = a->time;
	DTime second = b->time;
	if ( a->local != b->local ){
		if ( a->local )
			first = DTime_LocalToUtc( first );
		else
			second = DTime_LocalToUtc( second );
	}
	return DTime_Compare( first, second );
}



DaoDateSpan* DaoDateSpan_New()
{
	DaoDateSpan *self = (DaoDateSpan*) DaoCpod_New( daox_type_date_span, sizeof(DaoDateSpan) );
	return self;
}
void DaoDateSpan_Delete( DaoDateSpan *self )
{
	DaoCpod_Delete( (DaoCpod*) self );
}
DaoType* DaoDateSpan_Type()
{
	return daox_type_date_span;
}



DaoTimeSpan* DaoTimeSpan_New()
{
	DaoTimeSpan *self = (DaoTimeSpan*) DaoCpod_New( daox_type_time_span, sizeof(DaoTimeSpan) );
	return self;
}
void DaoTimeSpan_Delete( DaoTimeSpan *self )
{
	DaoCpod_Delete( (DaoCpod*) self );
}
DaoType* DaoTimeSpan_Type()
{
	return daox_type_time_span;
}




static void DATE_Value( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *self = (DaoDate*) p[0];
	DaoProcess_PutInteger( proc, (dao_integer) DDate_ToDay( self->date ) );
}

static void DATE_GetYear( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *self = (DaoDate*) p[0];
	DaoProcess_PutInteger( proc, self->date.year );
}

static void DATE_GetMonth( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *self = (DaoDate*) p[0];
	DaoProcess_PutInteger( proc, self->date.month );
}

static void DATE_GetDay( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *self = (DaoDate*) p[0];
	DaoProcess_PutInteger( proc, self->date.day );
}

static void DATE_SetYear( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *self = (DaoDate*) p[0];
	self->date.year = p[1]->xInteger.value;
}

static void DATE_SetMonth( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *self = (DaoDate*) p[0];
	int value = p[1]->xInteger.value;
	if( value < 1 || value > 12 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid month" );
		return;
	}
	self->date.month = value;
}

static void DATE_SetDay( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *self = (DaoDate*) p[0];
	int value = p[1]->xInteger.value;
	int days = DDate_MonthDays( self->date );
	if( value < 1 || value > days ){
		DaoProcess_RaiseError( proc, "Param", "Invalid day" );
		return;
	}
	self->date.day = value;
}

static void DATE_WeekDay( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *self = (DaoDate*) p[0];
	int jday = DDate_ToJulianDay( self->date );
	DaoProcess_PutInteger( proc, jday%7 + 1 );
}

static void DATE_YearDay( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *self = (DaoDate*) p[0];
	DDate nyday = self->date;
	nyday.month = 1;
	nyday.day = 1;
	DaoProcess_PutInteger( proc, DDate_ToJulianDay( self->date ) - DDate_ToJulianDay( nyday ) + 1 );
}

static void DATE_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *self = (DaoDate*) p[0];
	struct tm parts;
	daoint i;

	for ( i = 1; i < N; i++ ){
		dao_integer val = p[i]->xTuple.values[1]->xInteger.value;
		switch ( p[i]->xTuple.values[0]->xEnum.value ){
		case 0:  self->date.year  = val; break; // year
		case 1:  self->date.month  = val; break; // month
		case 2:  self->date.day    = val; break; // day
		default: break;
		}
	}
	// TODO: check;
}

static void DATE_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *self = (DaoDate*) p[0];
	DDate resdate = self->date;
	daoint years = 0;
	daoint months = 0;
	daoint days = 0;
	int i, y, m, d;

	if ( N == 0 )
		return;
	for ( i = 1; i < N; i++ ){
		dao_integer count = p[i]->xTuple.values[1]->xInteger.value;
		switch ( p[i]->xTuple.values[0]->xEnum.value ){
		case 0:	years = count; break;
		case 1:	months = count; break;
		case 2:	days = count; break;
		}
	}
	if ( years || months || days ){
		if ( years || months ){
			resdate.year += years;
			resdate.year += months/12;
			resdate.month += months%12;
			if ( resdate.month > 12 ){
				resdate.year += 1;
				resdate.month -= 12;
			} else if ( resdate.month < 1 ){
				resdate.year--;
				resdate.month = 12 - resdate.month;
			}
			d = DDate_MonthDays( resdate );
			if ( resdate.day > d ){
				days += d - resdate.day;
				resdate.day = d;
			}
		}
		if ( days ){
			int jday = DDate_ToJulianDay( self->date ) + days;
			resdate = DDate_FromJulianDay( jday );
		}
	}
	// TODO: check;
	self->date = resdate;
}

static void DATE_Days( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *self = (DaoDate*) p[0];
	if ( p[1]->xEnum.value == 0 )
		DaoProcess_PutInteger( proc, DDate_MonthDays( self->date ) );
	else
		DaoProcess_PutInteger( proc, LeapYear( self->date.year )? 366 : 365 );
}

static void DATE_EQ( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *a = (DaoDate*) p[0];
	DaoDate *b = (DaoDate*) p[1];
	DaoProcess_PutBoolean( proc, DDate_Compare( a->date, b->date ) == 0 );
}

static void DATE_NE( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *a = (DaoDate*) p[0];
	DaoDate *b = (DaoDate*) p[1];
	DaoProcess_PutBoolean( proc, DDate_Compare( a->date, b->date ) != 0 );
}

static void DATE_LT( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *a = (DaoDate*) p[0];
	DaoDate *b = (DaoDate*) p[1];
	DaoProcess_PutBoolean( proc, DDate_Compare( a->date, b->date ) < 0 );
}

static void DATE_LE( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *a = (DaoDate*) p[0];
	DaoDate *b = (DaoDate*) p[1];
	DaoProcess_PutBoolean( proc, DDate_Compare( a->date, b->date ) <= 0 );
}
static void DATE_Plus( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *a = (DaoDate*) p[0];
	DaoDateSpan *b = (DaoDateSpan*) p[1];
	int days1 = DDate_ToDay( a->date );
	DDate res = DDate_FromDay( days1 + b->span.value );
	DaoProcess_PutDate( proc, res );
}
static void DATE_Plus2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *a = (DaoDate*) p[0];
	DaoDateSpan *b = (DaoDateSpan*) p[1];
	int days1 = DDate_ToDay( a->date );
	a->date = DDate_FromDay( days1 + b->span.value );
}
static void DATE_Minus( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *a = (DaoDate*) p[0];
	DaoDateSpan *b = (DaoDateSpan*) p[1];
	int days1 = DDate_ToDay( a->date );
	DDate res = DDate_FromDay( days1 - b->span.value );
	DaoProcess_PutDate( proc, res );
}
static void DATE_Minus2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *a = (DaoDate*) p[0];
	DaoDateSpan *b = (DaoDateSpan*) p[1];
	int days1 = DDate_ToDay( a->date );
	a->date = DDate_FromDay( days1 - b->span.value );
}
static void DATE_Minus3( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *a = (DaoDate*) p[0];
	DaoDate *b = (DaoDate*) p[1];
	if( DDate_Compare( a->date, b->date ) > 0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid date subtraction" );
		return;
	}
	DaoProcess_PutDateSpan( proc, DDateSpan_FromDates( a->date, b->date ) );
}

static void DATE_DayDiff( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDate *a = (DaoDate*) p[0];
	DaoDate *b = (DaoDate*) p[1];
	DaoProcess_PutInteger( proc, DDate_ToJulianDay( b->date ) - DDate_ToJulianDay( a->date ) );
}

static DaoFuncItem dateMeths[] =
{
	/*! \c Returns the number of days since 2000-1-1, 00:00:00 UTC */
	{ DATE_Value,   ".value(invar self: Date) => int" },
	{ DATE_Value,   "(int)(invar self: Date)" },

	/*! Specific date part */
	{ DATE_GetYear,    ".year(invar self: Date) => int" },
	{ DATE_GetMonth,   ".month(invar self: Date) => int" },
	{ DATE_GetDay,     ".day(invar self: Date) => int" },

	{ DATE_SetYear,    ".year=(self: Date, value: int)" },
	{ DATE_SetMonth,   ".month=(self: Date, value: int)" },
	{ DATE_SetDay,     ".day=(self: Date, value: int)" },

	/*! Day of week */
	{ DATE_WeekDay, ".weekDay(invar self: Date) => int" },

	/*! Day of year */
	{ DATE_YearDay, ".yearDay(invar self: Date) => int" },

	/*! Sets one or more date parts using named values */
	{ DATE_Set,     "set(self: Date, ...: tuple<enum<year,month,day>,int>)" },

	{ DATE_Add,	    "add(self: Date, ...: tuple<enum<years,months,days>,int>)" },

	/*! Returns the number of day in the month or year of the given date depending on the \a period parameter */
	{ DATE_Days,  "daysIn(invar self: Date, period: enum<month,year>) => int" },

	/*! Returns date formatted to string using \a format, which follows the rules for C \c strfdate() with
	 * the following exceptions:
	 *
	 * \warning Available format specifiers are platform-dependent. */
	//{ DATE_Format, "format(invar self: Date, format = '%Y-%m-%d %H:%M:%S.%f %t') => string" },

	/*! Converts date to string; identical to calling \c format() with default format string */
	//{ DATE_ToString, "(string)(invar self: Date)" },

	/*! Returns date formatted to string using template \a format. \a names can specify custome names for months
	 * ('month' => {<12 names>}), days of week ('week' => {<7 names>}), days of year ('day' => {<365/366 names>}) or
	 * halfday names ('halfday' => {<2 names>}) */
	//{ DATE_Format2, "format(invar self: Date, invar names: map<string,list<string>>, format = '%Y-%M-%D, %H:%I:%S' ) => string" },

	{ DATE_Plus,    "+  (invar a: Date, b: DateSpan) => Date" },
	{ DATE_Minus,   "-  (invar a: Date, b: DateSpan) => Date" },
	{ DATE_Minus3,  "-  (invar a: Date, invar b: Date) => DateSpan" },
	{ DATE_Plus2,   "+= (self: Date, b: DateSpan)" },
	{ DATE_Minus2,  "-= (self: Date, b: DateSpan)" },

	/*! Datedate comparison */
	{ DATE_EQ,  "== (invar a: Date, invar b: Date) => bool" },
	{ DATE_NE,  "!= (invar a: Date, invar b: Date) => bool" },
	{ DATE_LT,  "<  (invar a: Date, invar b: Date) => bool" },
	{ DATE_LE,  "<= (invar a: Date, invar b: Date) => bool" },

	{ NULL, NULL }
};

/*! Represents date information */
DaoTypeBase dateTyper =
{
	"Date", NULL, NULL, dateMeths, {NULL}, {0},
	(FuncPtrDel)DaoDate_Delete, NULL
};




static void TIME_Now( DaoProcess *proc, DaoValue *p[], int N )
{
	int local = p[0]->xEnum.value == 0;
	DTime time = DTime_Now( local );
	DaoTime *self = DaoProcess_PutTime( proc, time, local );
	if ( time.date.month == 0 ){
		DaoProcess_RaiseError( proc, timeerr, "Failed to get current datetime" );
		return;
	}
}

static void TIME_Time( DaoProcess *proc, DaoValue *p[], int N )
{
	DTime time = DTime_FromMicroSeconds( p[0]->xInteger.value );
	if( p[1]->xEnum.value == 0 ) time = DTime_UtcToLocal( time );
	DaoProcess_PutTime( proc, time, p[1]->xEnum.value == 0 );
	if( time.date.month == 0 ){
		DaoProcess_RaiseError( proc, timeerr, "Invalid datetime" );
		return;
	}
}


static void TIME_MakeTime( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self;
	struct tm parts;
	DTime time;

	time.date.year   = p[0]->xInteger.value;
	time.date.month  = p[1]->xInteger.value;
	time.date.day    = p[2]->xInteger.value;
	time.hour   = p[3]->xInteger.value;
	time.minute = p[4]->xInteger.value;
	time.second = p[5]->xFloat.value;

	DaoProcess_PutTime( proc, time, 1 );
}

static int ToBits( const char *str, int len )
{
	int i, bits = 0;
	if( len > 10 ) return 0;
	for ( i = 0; i < len; i++ ){
		if( str[i] == '-' ){
			bits |= 1 << 3*i;
		}else if( str[i] == ':' ){
			bits |= 2 << 3*i;
		}else if( isdigit( str[i] ) ){
			bits |= 3 << 3*i;
		}else{
			bits |= 4 << 3*i;
		}
	}
	return bits;
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

DTime ParseRfc3339Time( DString *str )
{
	DTime inv_time = {0};
	DTime res = {0};
	int n, sign, offset = 0;
	const char *cp = str->chars;
	if ( !isdigit(cp[0]) || !isdigit(cp[1]) || !isdigit(cp[2]) || !isdigit(cp[3]) || cp[4] != '-' )
		return inv_time;
	res.date.year = ( cp[0] - '0' )*1000 + ( cp[1] - '0' )*100 + ( cp[2] - '0' )*10 + ( cp[3] - '0' );
	cp += 5;

	if ( !isdigit(cp[0]) || !isdigit(cp[1]) || cp[2] != '-' )
		return inv_time;
	res.date.month = ( cp[0] - '0' )*10 + ( cp[1] - '0' );
	if ( res.date.month == 0 || res.date.month > 12 )
		return inv_time;
	cp += 3;

	if ( !isdigit(cp[0]) || !isdigit(cp[1]) || cp[2] != 'T' )
		return inv_time;
	res.date.day = ( cp[0] - '0' )*10 + ( cp[1] - '0' );
	if ( res.date.day == 0 || res.date.day > DDate_MonthDays( res.date ) )
		return inv_time;
	cp += 3;

	if ( !isdigit(cp[0]) || !isdigit(cp[1]) || cp[2] != ':' )
		return inv_time;
	res.hour = ( cp[0] - '0' )*10 + ( cp[1] - '0' );
	if ( res.hour >= 24 )
		return inv_time;
	cp += 3;

	if ( !isdigit(cp[0]) || !isdigit(cp[1]) || cp[2] != ':' )
		return inv_time;
	res.minute = ( cp[0] - '0' )*10 + ( cp[1] - '0' );
	if ( res.minute >= 60 )
		return inv_time;
	cp += 3;

	if ( !isdigit(cp[0]) || !isdigit(cp[1]) )
		return inv_time;
	cp += 2;
	if ( *cp == '.' ){ // fractional seconds
		char *end;
		const char *start = cp - 2;
		double num;
		for ( ++cp; isdigit( *cp ); ++cp );
		num = strtod( start, &end );
		if ( end != cp || ( ( num == HUGE_VAL || num == -HUGE_VAL ) && errno == ERANGE ) || num < 0 || num > 60.0 )
			return inv_time;
		res.second = num;
	}
	else { // integer seconds
		res.second = ( *( cp - 2 ) - '0' )*10 + ( *( cp - 1 ) - '0' );
		if ( res.second >= 60.0 )
			return inv_time;
	}

	if ( *cp == 'Z' )
		return cp[1] == 0? res : inv_time;

	// utc offset
	if ( ( cp[0] != '+' && cp[0] != '-' ) || !isdigit(cp[1]) || !isdigit(cp[2]) || cp[3] != ':' ||
		 !isdigit(cp[4]) || !isdigit(cp[5]) || cp[6] != 0 )
		return inv_time;
	sign = *cp == '+'? 1 : -1;
	++cp;
	n = ( cp[0] - '0' )*10 + ( cp[1] - '0' );
	if ( n >= 24 )
		return inv_time;
	offset = n*3600*sign;
	cp += 3;
	n = ( cp[0] - '0' )*10 + ( cp[1] - '0' );
	if ( n >= 60 )
		return inv_time;
	offset += n*60*sign;

	if (1){
		double fract = res.second - (dao_time_t)res.second;
		res = DTime_FromSeconds( DTime_ToSeconds( res ) - offset );
		res.second += fract;
	}

	return res;
}

DTime ParseSimpleTime( DaoProcess *proc, DString *str )
{
	DTime inv_time = {0};
	DTime time = {0};
	DString *sdate = NULL, *stime = NULL;
	DString *sfrac = NULL;
	daoint pos;

	str = DString_Copy( str );
	pos = DString_FindChar( str, ' ', 0 );
	if ( pos >= 0 ){
		sdate = DString_New();
		stime = DString_New();
		DString_SubString( str, sdate, 0, pos );
		DString_SubString( str, stime, pos + 1, str->size - pos - 1 );
	} else if ( DString_FindChar( str, ':', 0 ) > 0 ){
		stime = str;
		str = NULL;
	} else if ( DString_FindChar( str, '-', 0 ) > 0 ){
		sdate = str;
		str = NULL;
	} else {
		goto Error;
	}

	if( stime ){
		pos = DString_FindChar( stime, '.', 0 );
		if( pos >= 0 ){
			sfrac = DString_New();
			DString_SubString( stime, sfrac, pos, -1 );
			DString_Erase( stime, pos, -1 );
		}
	}

	if ( sdate ){
		int bits = ToBits( sdate->chars, sdate->size );
		if ( bits == ToBits( "0000-00-00", 10 ) ){ /* YYYY-MM-DD */
			time.date.year = GetNum( sdate->chars, 4 );
			time.date.month = GetNum( sdate->chars + 5, 2 );
			time.date.day = GetNum( sdate->chars + 8, 2 );
		} else if ( bits == ToBits( "0000-00", 7 ) ){ /* YYYY-MM */
			time.date.year = GetNum( sdate->chars, 4 );
			time.date.month = GetNum( sdate->chars + 5, 2 );
			time.date.day = 1;
		} else if ( bits == ToBits( "00-00", 5 ) ){ /* MM-DD */
			time.date.month = GetNum( sdate->chars, 2 );
			time.date.day = GetNum( sdate->chars + 3, 2 );
		} else {
			goto Error;
		}
	}
	else {
		time = DTime_Now( 1 );
		if( time.date.month == 0 ){
			DaoProcess_RaiseError( proc, timeerr, "Failed to get current datetime" );
			return inv_time;
		}
	}
	if ( stime ){
		int bits = ToBits( stime->chars, stime->size );
		if ( bits == ToBits( "00:00:00", 8 ) ){ /* HH:MM:SS */
			time.hour = GetNum( stime->chars, 2 );
			time.minute = GetNum( stime->chars + 3, 2 );
			time.second = GetNum( stime->chars + 6, 2 );
		} else if ( bits == ToBits( "00:00", 5 ) ){ /* HH:MM */
			 time.hour = GetNum( stime->chars, 2 );
			 time.minute = GetNum( stime->chars + 3, 2 );
			 time.second = 0;
		} else {
			goto Error;
		}
	} else {
		time.hour = 0;
		time.minute = 0;
		time.second = 0;
	}
	if ( sfrac ){
		int i;
		for(i=1; i<sfrac->size; ++i){
			if( isdigit( sfrac->chars[i] ) == 0 ) goto Error;
		}
		time.second += strtod( sfrac->chars, NULL );
	}
	goto Clean;
Error:
	time = inv_time;
Clean:
	if( str ) DString_Delete( str );
	if( sdate ) DString_Delete( sdate );
	if( stime ) DString_Delete( stime );
	if( sfrac ) DString_Delete( sfrac );
	return time;
}

static void TIME_Parse( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *str = p[0]->xString.value;
	int local = 0;
	DTime t = ParseRfc3339Time( str );

	if ( !t.date.month ){
		local = 1;
		t = ParseSimpleTime( proc, str );

		if ( !t.date.month ){
			DaoProcess_RaiseError( proc, "Param", "Unsupported datetime format" );
			return;
		}
	}
	DaoProcess_PutTime( proc, t, local );
}


static void TIME_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	daoint i;
	DString *name = DString_New();

	for ( i = 1; i < N; i++ ){
		DaoEnum *en = &p[i]->xTuple.values[0]->xEnum;
		DaoEnum_MakeName( en, name );
		if ( strcmp( name->chars, "$second" ) == 0 )
			self->time.second =  p[i]->xTuple.values[1]->xFloat.value; //sec
		else {
			dao_integer val = p[i]->xTuple.values[1]->xInteger.value;
			switch ( p[i]->xTuple.values[0]->xEnum.value ){
			case 0:  self->time.date.year  = val; break; // year
			case 1:  self->time.date.month  = val; break; // month
			case 2:  self->time.date.day    = val; break; // day
			case 3:  self->time.hour   = val; break; // hour
			case 4:  self->time.minute = val; break; // min
			default: break;
			}
		}
	}
	DString_Delete( name );
	if ( !DTime_IsValid( self->time )){
		DaoProcess_RaiseError( proc, timeerr, "Invalid datetime" );
		return;
	}
}


static void TIME_Value( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutFloat( proc, (dao_integer) DTime_ToMicroSeconds( self->time ) / 1.0E6 );
}

static void TIME_Type( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutEnum( proc, self->local? "local" : "utc" );
}

static void TIME_Convert( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];

	if( self->local != (p[1]->xEnum.value == 0) ){
		self->time = p[1]->xEnum.value == 0? DTime_UtcToLocal( self->time ) : DTime_LocalToUtc( self->time );
		self->local = !self->local;
	}
}

static void TIME_GetDate( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutDate( proc, self->time.date );
}

static void TIME_GetSecond( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutFloat( proc, self->time.second );
}

static void TIME_GetMinute( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutInteger( proc, self->time.minute );
}

static void TIME_GetHour( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutInteger( proc, self->time.hour );
}

static void TIME_GetDay( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutInteger( proc, self->time.date.day );
}

static void TIME_GetMonth( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutInteger( proc, self->time.date.month );
}

static void TIME_GetYear( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutInteger( proc, self->time.date.year );
}

static void TIME_SetSecond( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	dao_float value = p[1]->xFloat.value;
	if( value < 0.0 || value >= 60.0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid seconds" );
		return;
	}
	self->time.second = value;
}

static void TIME_SetMinute( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	int value = p[1]->xInteger.value;
	if( value < 0 || value >= 60 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid minutes" );
		return;
	}
	self->time.minute = value;
}

static void TIME_SetHour( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	int value = p[1]->xInteger.value;
	if( value < 0 || value >= 24 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid hour" );
		return;
	}
	self->time.hour = value;
}

static void TIME_SetDay( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	int value = p[1]->xInteger.value;
	int days = DDate_MonthDays( self->time.date );
	if( value < 1 || value > days ){
		DaoProcess_RaiseError( proc, "Param", "Invalid day" );
		return;
	}
	self->time.date.day = value;
}

static void TIME_SetMonth( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	int value = p[1]->xInteger.value;
	if( value < 1 || value > 12 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid month" );
		return;
	}
	self->time.date.month = value;
}

static void TIME_SetYear( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	self->time.date.year = p[1]->xInteger.value;
}

static void TIME_WeekDay( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	int jday = DTime_ToJulianDay( self->time );
	DaoProcess_PutInteger( proc, jday%7 + 1 );
}

static void TIME_YearDay( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DTime nyday = self->time;
	nyday.date.month = 1;
	nyday.date.day = 1;
	DaoProcess_PutInteger( proc, DTime_ToJulianDay( self->time ) - DTime_ToJulianDay( nyday ) + 1 );
}

static void TIME_Zone( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTuple *res = DaoProcess_PutTuple( proc, 4 );
	tzset();
	res->values[0]->xBoolean.value = daylight > 0;
	res->values[1]->xInteger.value = timezone;
	DaoString_SetChars( &res->values[2]->xString, tzname[0] );
	DaoString_SetChars( &res->values[3]->xString, tzname[1] );
}


int DaoTime_GetTzOffset( DaoTime *self )
{
	if ( self->local ){
		return DTime_ToSeconds( self->time ) - DTime_ToSeconds( DTime_LocalToUtc( self->time ) );
	}
	return 0;
}

void DaoTime_GetTimezoneShift( DaoTime *self, char *buf, size_t size )
{
	int shift = DaoTime_GetTzOffset( self );
	int hours = shift/3600, minutes = shift - hours*3600;

	if ( minutes < 0 ) minutes = -minutes;

	snprintf( buf, size, "%+02i:%02i", hours, minutes );
}

static void GetFracSecond( double second, char *buf, size_t size )
{
	if ( size > 6 ){
		snprintf( buf, size, "%06f", second - (dao_time_t)second );
		memmove( buf, buf + 2, 6 );
		buf[6] = 0;
	}
}

static void TIME_Format( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DString *fmt = p[1]->xString.value;
	struct tm parts;
	char buf[100];
	daoint i;

	for ( i = 0; i < fmt->size; i++ ){
		if ( fmt->chars[i] == '%' ){
			if ( fmt->chars[i + 1] == 't' ){
				DaoTime_GetTimezoneShift( self, buf, sizeof(buf) );
				DString_InsertChars( fmt, buf, i, 2, 0 );
				i += 5;
			}
			else if ( fmt->chars[i + 1] == 'f' ){
				GetFracSecond( self->time.second, buf, sizeof(buf) );
				DString_InsertChars( fmt, buf, i, 2, 0 );
				i += 6;
			}
			else
				i++;
		}
	}

	DTime_ToStructTM( self->time, & parts );
	if ( strftime( buf, sizeof(buf), fmt->chars, &parts ))
		DaoProcess_PutChars( proc, buf );
	else
		DaoProcess_RaiseError( proc, "Param", "Invalid format" );
}
static void TIME_ToString( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	struct tm parts;
	char buf[100];

	DTime_ToStructTM( self->time, & parts );
	if ( strftime( buf, sizeof(buf), "%Y-%m-%d %H:%M:%S.", &parts )){
		int len = strlen( buf );
		GetFracSecond( self->time.second, buf + len, sizeof(buf) - len );
		strcat( buf, " " );
		len = strlen( buf );
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

static void TIME_Format2( DaoProcess *proc, DaoValue *p[], int N )
{
	int  i, wday;
	int halfday = 0;
	const int size = p[2]->xString.value->size;
	const char *format = DString_GetData( p[2]->xString.value );
	char buf[100];
	char *p1 = buf+1;
	char *p2;
	DaoTime *self = (DaoTime*) p[0];
	DaoMap *sym = (DaoMap*)p[1];
	DaoString *ds = DaoString_New(1);
	DaoValue *key = (DaoValue*) ds;
	DString *S;

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
				sprintf( p1, "%i", self->time.date.year );
				break;
			case 'y' :
				sprintf( p1, "%i", self->time.date.year );
				p2 += 2;
				break;
			case 'M' :
			case 'm' :
				if( ! addStringFromMap( key, S, sym, "month", self->time.date.month-1 ) ){
					sprintf( p1, "%i", self->time.date.month );
					if( ch=='M' && p1[1]==0 ) p2 = buf; /* padding 0; */
				}else p2 = NULL;
				break;
			case 'D' :
			case 'd' :
				if( ! addStringFromMap( key, S, sym, "date", self->time.date.day ) ){
					sprintf( p1, "%i", self->time.date.day );
					if( ch=='D' && p1[1]==0 ) p2 = buf; /* padding 0; */
				}else p2 = NULL;
				break;
			case 'W' :
			case 'w' :
				wday = DTime_ToJulianDay(self->time) % 7;
				if( ! addStringFromMap( key, S, sym, "week", wday ) )
					sprintf( p1, "%i", wday+1 );
				else p2 = NULL;
				break;
			case 'H' :
			case 'h' :
				if( halfday )
					sprintf( p1, "%i", self->time.hour %12 );
				else
					sprintf( p1, "%i", self->time.hour );
				if( ch=='H' && p1[1]==0 ) p2 = buf; /* padding 0; */
				break;
			case 'I' :
			case 'i' :
				sprintf( p1, "%i", self->time.minute );
				if( ch=='I' && p1[1]==0 ) p2 = buf; /* padding 0; */
				break;
			case 'S' :
			case 's' :
				sprintf( p1, "%6.3f", self->time.second );
				if( ch=='S' && p1[1]==0 ) p2 = buf; /* padding 0; */
				break;
			case 'a' :
				if( ! addStringFromMap( key, S, sym, "halfday", 0 ) ){
					if( self->time.hour >= 12 ) strcpy( p1, "pm" );
					else strcpy( p1, "am" );
				}else p2 = NULL;
				break;
			case 'A' :
				if( ! addStringFromMap( key, S, sym, "halfday", 1 ) ){
					if( self->time.hour >= 12 ) strcpy( p1, "PM" );
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

static void TIME_EQ( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*) p[0];
	DaoTime *b = (DaoTime*) p[1];
	DaoProcess_PutBoolean( proc, DaoTime_Compare( a, b ) == 0 );
}

static void TIME_NE( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*) p[0];
	DaoTime *b = (DaoTime*) p[1];
	DaoProcess_PutBoolean( proc, DaoTime_Compare( a, b ) != 0 );
}

static void TIME_LT( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*) p[0];
	DaoTime *b = (DaoTime*) p[1];
	DaoProcess_PutBoolean( proc, DaoTime_Compare( a, b ) < 0 );
}

static void TIME_LE( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*) p[0];
	DaoTime *b = (DaoTime*) p[1];
	DaoProcess_PutBoolean( proc, DaoTime_Compare( a, b ) <= 0 );
}

static void TIME_Plus( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	dao_time_t useconds1 = DTime_ToMicroSeconds( a->time );
	dao_time_t useconds2 = DTimeSpan_ToUSeconds( b->span );
	DTime res = DTime_FromMicroSeconds( useconds1 + useconds2 );
	DaoProcess_PutTime( proc, res, a->local );
}
static void TIME_Plus2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	dao_time_t useconds1 = DTime_ToMicroSeconds( a->time );
	dao_time_t useconds2 = DTimeSpan_ToUSeconds( b->span );
	a->time = DTime_FromMicroSeconds( useconds1 + useconds2 );
}
static void TIME_Minus( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	dao_time_t useconds1 = DTime_ToMicroSeconds( a->time );
	dao_time_t useconds2 = DTimeSpan_ToUSeconds( b->span );
	DTime res = DTime_FromMicroSeconds( useconds1 - useconds2 );
	DaoProcess_PutTime( proc, res, a->local );
}
static void TIME_Minus2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	dao_time_t useconds1 = DTime_ToMicroSeconds( a->time );
	dao_time_t useconds2 = DTimeSpan_ToUSeconds( b->span );
	a->time = DTime_FromMicroSeconds( useconds1 - useconds2 );
}
static void TIME_Minus3( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*) p[0];
	DaoTime *b = (DaoTime*) p[1];
	dao_time_t useconds1 = DTime_ToMicroSeconds( a->time );
	dao_time_t useconds2 = DTime_ToMicroSeconds( b->time );
	DTimeSpan res;
	if ( a->local != b->local ){
		if ( a->local )
			useconds1 -= DaoTime_GetTzOffset( a )*1000000LL;
		else
			useconds2 -= DaoTime_GetTzOffset( b )*1000000LL;
	}
	res = DTimeSpan_FromUSeconds( useconds1 - useconds2 );
	if( useconds1 < useconds2 ){
		DaoProcess_RaiseError( proc, "Param", "Subtracting leading to negative result" );
		return;
	}
	DaoProcess_PutTimeSpan( proc, res );
}

static void TIME_Since( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DTime now = DTime_Now( self->local );
	dao_time_t diff = DTime_ToMicroSeconds( now ) - DTime_ToMicroSeconds( self->time );
	DaoProcess_PutTimeSpan( proc, DTimeSpan_FromUSeconds( diff ) );
}

static void TIME_DayDiff( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*) p[0];
	DaoTime *b = (DaoTime*) p[1];
	DTime atime = a->time;
	DTime btime = a->time;
	if ( a->local != b->local ){
		if ( a->local )
			atime = DTime_LocalToUtc( atime );
		else
			btime = DTime_LocalToUtc( btime );
	}
	DaoProcess_PutInteger( proc, DTime_ToJulianDay( btime ) - DTime_ToJulianDay( atime ) );
}

static DaoFuncItem timeMeths[] =
{
	/*! Sets one or more datetime parts using named values */
	{ TIME_Set,     "set(self: DateTime, ...: tuple<enum<year,month,day,hour,minute>,int> | tuple<enum<second>,float>)" },

	/*! \c Returns the number of seconds since 2000-1-1, 00:00:00 UTC */
	{ TIME_Value,   ".value(invar self: DateTime) => float" },
	{ TIME_Value,   "(float)(invar self: DateTime)" },

	/*! Returns datetime kind with regard to the time zone: UTC or local */
	{ TIME_Type,    ".kind(invar self: DateTime) => enum<local,utc>" },

	/*! Converts datetime to the given \a kind */
	{ TIME_Convert, "convert(self: DateTime, kind: enum<local,utc>)" },

	{ TIME_GetDate,  ".date(invar self: DateTime) => Date" },

	/*! Specific datetime part */
	{ TIME_GetSecond,  ".second(invar self: DateTime) => float" },
	{ TIME_GetMinute,  ".minute(invar self: DateTime) => int" },
	{ TIME_GetHour,    ".hour(invar self: DateTime) => int" },
	{ TIME_GetDay,     ".day(invar self: DateTime) => int" },
	{ TIME_GetMonth,   ".month(invar self: DateTime) => int" },
	{ TIME_GetYear,    ".year(invar self: DateTime) => int" },

	{ TIME_SetSecond,  ".second=(self: DateTime, value: float)" },
	{ TIME_SetMinute,  ".minute=(self: DateTime, value: int)" },
	{ TIME_SetHour,    ".hour=(self: DateTime, value: int)" },
	{ TIME_SetDay,     ".day=(self: DateTime, value: int)" },
	{ TIME_SetMonth,   ".month=(self: DateTime, value: int)" },
	{ TIME_SetYear,    ".year=(self: DateTime, value: int)" },

	/*! Day of week */
	{ TIME_WeekDay, ".weekDay(invar self: DateTime) => int" },

	/*! Day of year */
	{ TIME_YearDay, ".yearDay(invar self: DateTime) => int" },

	/*! Returns datetime formatted to string using \a format, which follows the rules for C \c strftime() with
	 * the following exceptions:
	 * - '%t' -- time zone offset
	 * - '%f' -- fractional part of seconds
	 *
	 * \warning Available format specifiers are platform-dependent. */
	{ TIME_Format, "format(invar self: DateTime, format = '%Y-%m-%d %H:%M:%S.%f %t') => string" },

	/*! Converts datetime to string; identical to calling \c format() with default format string */
	{ TIME_ToString, "(string)(invar self: DateTime)" },

	/*! Returns datetime formatted to string using template \a format. \a names can specify custome names for months
	 * ('month' => {<12 names>}), days of week ('week' => {<7 names>}), days of year ('day' => {<365/366 names>}) or
	 * halfday names ('halfday' => {<2 names>}) */
	{ TIME_Format2, "format(invar self: DateTime, invar names: map<string,list<string>>, format = '%Y-%M-%D, %H:%I:%S' ) => string" },

	{ TIME_Plus,    "+  (invar a: DateTime, invar b: TimeSpan) => DateTime" },
	{ TIME_Minus,   "-  (invar a: DateTime, invar b: TimeSpan) => DateTime" },
	{ TIME_Minus3,  "-  (invar a: DateTime, invar b: DateTime) => TimeSpan" },
	{ TIME_Plus2,   "+= (self: DateTime, invar b: TimeSpan)" },
	{ TIME_Minus2,  "-= (self: DateTime, invar b: TimeSpan)" },

	/*! Datetime comparison */
	{ TIME_EQ,  "== (invar a: DateTime, invar b: DateTime) => bool" },
	{ TIME_NE,  "!= (invar a: DateTime, invar b: DateTime) => bool" },
	{ TIME_LT,  "<  (invar a: DateTime, invar b: DateTime) => bool" },
	{ TIME_LE,  "<= (invar a: DateTime, invar b: DateTime) => bool" },

	{ NULL, NULL }
};

/*! Represents date and time information */
DaoTypeBase timeTyper =
{
	"DateTime", NULL, NULL, timeMeths, {NULL}, {0},
	(FuncPtrDel)DaoTime_Delete, NULL
};



static void DSPAN_GetYears( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDateSpan *self = (DaoDateSpan*) p[0];
	DaoProcess_PutInteger( proc, self->span.years );
}

static void DSPAN_GetMonths( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDateSpan *self = (DaoDateSpan*) p[0];
	DaoProcess_PutInteger( proc, self->span.months );
}

static void DSPAN_GetDays( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDateSpan *self = (DaoDateSpan*) p[0];
	DaoProcess_PutInteger( proc, self->span.days );
}

static void DSPAN_EQ( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDateSpan *a = (DaoDateSpan*) p[0];
	DaoDateSpan *b = (DaoDateSpan*) p[1];
	DaoProcess_PutBoolean( proc, DDateSpan_Compare( a->span, b->span ) == 0 );
}
static void DSPAN_NE( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDateSpan *a = (DaoDateSpan*) p[0];
	DaoDateSpan *b = (DaoDateSpan*) p[1];
	DaoProcess_PutBoolean( proc, DDateSpan_Compare( a->span, b->span ) != 0 );
}
static void DSPAN_LT( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDateSpan *a = (DaoDateSpan*) p[0];
	DaoDateSpan *b = (DaoDateSpan*) p[1];
	DaoProcess_PutBoolean( proc, DDateSpan_Compare( a->span, b->span ) < 0 );
}
static void DSPAN_LE( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDateSpan *a = (DaoDateSpan*) p[0];
	DaoDateSpan *b = (DaoDateSpan*) p[1];
	DaoProcess_PutBoolean( proc, DDateSpan_Compare( a->span, b->span ) <= 0 );
}

static void DSPAN_ToString( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoDateSpan *self = (DaoDateSpan*) p[0];
	char buf[50];
	int len = 0;
	if( self->span.years ) len += snprintf( buf + len, sizeof(buf) - len, "%iy", self->span.years );
	if( self->span.months ) len += snprintf( buf + len, sizeof(buf) - len, "%im", self->span.months );
	if( self->span.days ) len += snprintf( buf, sizeof(buf), "%id", self->span.days );
	DaoProcess_PutChars( proc, buf );
}


static DaoFuncItem dateSpanMeths[] =
{
	{ DSPAN_GetYears,   ".years(invar self: DateSpan) => int" },
	{ DSPAN_GetMonths,  ".months(invar self: DateSpan) => int" },
	{ DSPAN_GetDays,    ".days(invar self: DateSpan) => int" },

	{ DSPAN_EQ,  "== (invar a: DateSpan, invar b: DateSpan) => bool" },
	{ DSPAN_NE,  "!= (invar a: DateSpan, invar b: DateSpan) => bool" },
	{ DSPAN_LT,  "<  (invar a: DateSpan, invar b: DateSpan) => bool" },
	{ DSPAN_LE,  "<= (invar a: DateSpan, invar b: DateSpan) => bool" },

	{ DSPAN_ToString, "(string)(invar self: DateSpan)" },

	{ NULL, NULL }
};

DaoTypeBase dateSpanTyper =
{
	"DateSpan", NULL, NULL, dateSpanMeths, {NULL}, {0},
	(FuncPtrDel)DaoDateSpan_Delete, NULL
};



static void TSPAN_New( DaoProcess *proc, DaoValue *p[], int N )
{
	dao_integer days = p[0]->xInteger.value;
	dao_integer hours = p[1]->xInteger.value;
	dao_integer minutes = p[2]->xInteger.value;
	double seconds = p[3]->xFloat.value;
	DTimeSpan span;

	if( days < 0 ) goto WrongParam;
	if( hours < 0 || hours >= 24 ) goto WrongParam;
	if( minutes < 0 || minutes >= 60 ) goto WrongParam;
	if( seconds < 0.0 || seconds >= 60.0 ) goto WrongParam;

	span.days = days;
	span.hours = hours;
	span.minutes = minutes;
	span.seconds = seconds;
	DaoProcess_PutTimeSpan( proc, span );
	return;

WrongParam:
	DaoProcess_RaiseError( proc, "Param", "Invalid days" );
}

static void TSPAN_GetDays( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	DaoProcess_PutInteger( proc, self->span.days );
}

static void TSPAN_GetHours( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	DaoProcess_PutInteger( proc, self->span.hours );
}

static void TSPAN_GetMinutes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	DaoProcess_PutInteger( proc, self->span.minutes );
}

static void TSPAN_GetSeconds( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	DaoProcess_PutFloat( proc, self->span.seconds );
}


static void TSPAN_SetDays( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	int value = p[1]->xInteger.value;
	if( value < 0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid days" );
		return;
	}
	self->span.days = value;
}

static void TSPAN_SetHours( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	int value = p[1]->xInteger.value;
	if( value < 0 || value >= 24 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid hours" );
		return;
	}
	self->span.hours = value;
}

static void TSPAN_SetMinutes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	int value = p[1]->xInteger.value;
	if( value < 0 || value >= 60 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid minutes" );
		return;
	}
	self->span.minutes = value;
}

static void TSPAN_SetSeconds( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	dao_float value = p[1]->xFloat.value;
	if( value < 0.0 || value >= 60.0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid seconds" );
		return;
	}
	self->span.seconds = value;
}

static void TSPAN_Plus( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *s1 = (DaoTimeSpan*) p[0];
	DaoTimeSpan *s2 = (DaoTimeSpan*) p[1];
	dao_time_t us1 = DTimeSpan_ToUSeconds( s1->span );
	dao_time_t us2 = DTimeSpan_ToUSeconds( s2->span );
	DTimeSpan res = DTimeSpan_FromUSeconds( us1 + us2 );
	DaoProcess_PutTimeSpan( proc, res );
}
static void TSPAN_Plus2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *s1 = (DaoTimeSpan*) p[0];
	DaoTimeSpan *s2 = (DaoTimeSpan*) p[1];
	dao_time_t us1 = DTimeSpan_ToUSeconds( s1->span );
	dao_time_t us2 = DTimeSpan_ToUSeconds( s2->span );
	s1->span = DTimeSpan_FromUSeconds( us1 + us2 );
}
static void TSPAN_Minus( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *s1 = (DaoTimeSpan*) p[0];
	DaoTimeSpan *s2 = (DaoTimeSpan*) p[1];
	dao_time_t us1 = DTimeSpan_ToUSeconds( s1->span );
	dao_time_t us2 = DTimeSpan_ToUSeconds( s2->span );
	DTimeSpan res = DTimeSpan_FromUSeconds( us1 - us2 );
	DaoProcess_PutTimeSpan( proc, res );
}
static void TSPAN_Minus2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *s1 = (DaoTimeSpan*) p[0];
	DaoTimeSpan *s2 = (DaoTimeSpan*) p[1];
	dao_time_t us1 = DTimeSpan_ToUSeconds( s1->span );
	dao_time_t us2 = DTimeSpan_ToUSeconds( s2->span );
	s1->span = DTimeSpan_FromUSeconds( us1 - us2 );
}

static void TSPAN_EQ( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *a = (DaoTimeSpan*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	DaoProcess_PutBoolean( proc, DTimeSpan_Compare( a->span, b->span ) == 0 );
}
static void TSPAN_NE( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *a = (DaoTimeSpan*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	DaoProcess_PutBoolean( proc, DTimeSpan_Compare( a->span, b->span ) != 0 );
}
static void TSPAN_LT( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *a = (DaoTimeSpan*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	DaoProcess_PutBoolean( proc, DTimeSpan_Compare( a->span, b->span ) < 0 );
}
static void TSPAN_LE( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *a = (DaoTimeSpan*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	DaoProcess_PutBoolean( proc, DTimeSpan_Compare( a->span, b->span ) <= 0 );
}

static void TSPAN_ToString( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	char buf[50];
	int len = 0;
	if ( self->span.days )
		len += snprintf( buf, sizeof(buf), "%id", self->span.days );
	if ( self->span.hours )
		len += snprintf( buf + len, sizeof(buf) - len, "%ih", self->span.hours );
	if ( self->span.minutes )
		len += snprintf( buf + len, sizeof(buf) - len, "%im", self->span.minutes );
	if ( self->span.seconds != 0.0 )
		snprintf( buf + len, sizeof(buf) - len, "%fs", self->span.seconds );
	DaoProcess_PutChars( proc, buf );
}

enum {
	Span_D = 1,
	Span_H = 2,
	Span_M = 4,
	Span_S = 8,
	Span_Ms = 16,
	Span_Us = 32,
};

static void TSPAN_Parse( DaoProcess *proc, DaoValue *p[], int N )
{
	DTimeSpan ts = {0};
	DaoTimeSpan *res = DaoProcess_PutTimeSpan( proc, ts );
	const char *cp, *start = p[0]->xString.value->chars;
	unsigned long num;
	double dnum;
	int mask = 0, fract = 0;
	cp = start;
	while ( 1 ){
		if ( !isdigit( *cp ) )
			goto FormatError;
		for ( ++cp; isdigit( *cp ) || *cp == '.'; ++cp )
			if ( *cp == '.' )
				fract = 1;
		if ( !isalpha( *cp ) )
			goto FormatError;
		if ( *cp == 's' ){ // fractional seconds
			char *pend;
			if ( mask >= Span_S )
				goto FormatError;
			mask |= Span_S;
			dnum = strtod( start, &pend );
			if ( pend != cp || ( ( dnum == HUGE_VAL || dnum == -HUGE_VAL ) && errno == ERANGE ) || dnum < 0 || dnum > 60.0 )
				goto NumericError;
			res->span.seconds = dnum;
		}
		else {
			char *pend;
			num = strtoul( start, &pend, 10 );
			if ( pend != cp || ( num == ULONG_MAX && errno == ERANGE ) )
				goto NumericError;
			if ( strncmp( cp, "ms", 2) == 0 ){
				if ( mask >= Span_Ms || ( ( mask & Span_S ) && fract ) ) goto FormatError;
				if ( num > 1E3 ) goto NumericError;
				mask |= Span_Ms;
				res->span.seconds += num/1E3;
				++cp;
			}
			else if ( strncmp( cp, "us", 2) == 0 ){
				if ( mask >= Span_Us || ( ( mask & Span_S ) && fract ) ) goto FormatError;
				if ( mask == 0 ){
					if ( num > 1E6 ) goto NumericError;
					res->span.seconds = num/1E6;
				}
				else {
					if ( num > 1E3 ) goto NumericError;
					res->span.seconds += num/1E6;
				}
				mask |= Span_Us;
				++cp;
			}
			else if ( *cp == 'd' ){
				if ( mask >= Span_D ) goto FormatError;
				mask |= Span_D;
				res->span.days = num;
			}
			else if ( *cp == 'h' ){
				if ( mask >= Span_H ) goto FormatError;
				if ( num >= 24 ) goto NumericError;
				mask |= Span_H;
				res->span.hours = num;
			}
			else if ( *cp == 'm' ){
				if ( mask >= Span_M ) goto FormatError;
				if ( num >= 60 ) goto NumericError;
				mask |= Span_M;
				res->span.minutes = num;
			}
			else
				goto FormatError;
		}
		for ( ++cp; isspace( *cp ); ++cp );
		if ( !*cp )
			break;
		start = cp;
	}
	return;
FormatError:
	DaoProcess_RaiseError( proc, "Param", "Invalid format" );
	return;
NumericError:
	DaoProcess_RaiseError( proc, "Value", "Invalid number" );
}

static DaoFuncItem timeSpanMeths[] =
{
	{ TSPAN_New,  "TimeSpan( days = 0, hours = 0, minutes = 0, seconds = 0.0 )" },

	{ TSPAN_GetDays,     ".days(invar self: TimeSpan) => int" },
	{ TSPAN_GetHours,    ".hours(invar self: TimeSpan) => int" },
	{ TSPAN_GetMinutes,  ".minutes(invar self: TimeSpan) => int" },
	{ TSPAN_GetSeconds,  ".seconds(invar self: TimeSpan) => float" },

	{ TSPAN_SetDays,     ".days=(self: TimeSpan, value: int)" },
	{ TSPAN_SetHours,    ".hours=(self: TimeSpan, value: int)" },
	{ TSPAN_SetMinutes,  ".minutes=(self: TimeSpan, value: int)" },
	{ TSPAN_SetSeconds,  ".seconds=(self: TimeSpan, value: float)" },

	{ TSPAN_Plus,    "+  (invar a: TimeSpan, invar b: TimeSpan) => TimeSpan" },
	{ TSPAN_Minus,   "-  (invar a: TimeSpan, invar b: TimeSpan) => TimeSpan" },
	{ TSPAN_Plus2,   "+= (self: TimeSpan, invar span: TimeSpan)" },
	{ TSPAN_Minus2,  "-= (self: TimeSpan, invar span: TimeSpan)" },

	{ TSPAN_EQ,  "== (invar a: TimeSpan, invar b: TimeSpan) => bool" },
	{ TSPAN_NE,  "!= (invar a: TimeSpan, invar b: TimeSpan) => bool" },
	{ TSPAN_LT,  "<  (invar a: TimeSpan, invar b: TimeSpan) => bool" },
	{ TSPAN_LE,  "<= (invar a: TimeSpan, invar b: TimeSpan) => bool" },

	{ TSPAN_ToString, "(string)(invar self: TimeSpan)" },

	{ NULL, NULL }
};

DaoTypeBase timeSpanTyper =
{
	"TimeSpan", NULL, NULL, timeSpanMeths, {NULL}, {0},
	(FuncPtrDel)DaoTimeSpan_Delete, NULL
};



static DaoFuncItem timeFuncs[] =
{
	/*! Returns current datetime of the given \a kind */
	{ TIME_Now,  "now(kind: enum<local,utc> = $local) => DateTime" },

	/*! Returns \a kind datetime for a time in microseconds since 200-1-1, 00:00:00 UTC */
	{ TIME_Time,  "fromValue(value: int, kind: enum<local,utc> = $local) => DateTime" },

	/*! Returns local datetime composed of the specified \a year, \a month, \a day, \a hour, \a min and \a sec */
	{ TIME_MakeTime, "make(year: int, month: int, day: int, hour = 0, min = 0, sec = 0.0) => DateTime" },

	/*! Returns datetime parsed from \a value, which may be
	 * - local datetime consisting of date ('YYYY-MM-DD', 'YYYY-MM' or 'MM-DD') and/or time ('HH:MM:SS' or 'HH:MM') separated by ' '
	 * - RFC 3339 datetime returned as UTC */
	{ TIME_Parse,  "parse(value: string) => DateTime" },

	/*! Parses \c TimeSpan from \a value. Examples: '1d 3h 5m', '10m12.34s', '300ms'. Accepted units: d, h, m, s, ms, us.
	 * Seconds may have fractional part, other units must be integer numbers. When seconds are given as a fractional value,
	 * ms and us must not be present */
	{ TSPAN_Parse,  "span(value: string) => TimeSpan" },

	/*! Difference between the specified \a time and \c time.now() */
	{ TIME_Since,  "since(time: DateTime) => TimeSpan" },

	/*! Number of calendar days between \a start and \a end dates */
	{ TIME_DayDiff,  "daysBetween(start: DateTime, end: DateTime) => int" },

	/*! Returns local time zone information (environment variable *TZ*):
	 * -\c dst -- is Daylight Saving Time (DST) used
	 * -\c shift -- shift in seconds from UTC
	 * -\c name -- zone name
	 * -\c dstZone -- DST zone name */
	{ TIME_Zone,  "zone() => tuple<dst: bool, shift: int, name: string, dstZone: string>" },
	{ NULL, NULL }
};


DaoDate* DaoProcess_PutDate( DaoProcess *self, DDate date )
{
	DaoDate *res = (DaoDate*) DaoProcess_PutCpod( self, daox_type_date, sizeof(DaoDate) );

	if( res == NULL ) return NULL;

	if ( !DDate_IsValid( date ) ){
		DaoProcess_RaiseError( self, timeerr, "Invalid date" );
		return NULL;
	}
	res->date = date;
	return res;
}

DaoDate* DaoProcess_NewDate( DaoProcess *self, DDate date )
{
	DaoDate *res = (DaoDate*) DaoProcess_NewCpod( self, daox_type_date, sizeof(DaoDate) );
	if ( !DDate_IsValid( date ) ){
		DaoProcess_RaiseError( self, timeerr, "Invalid date" );
		return NULL;
	}
	res->date = date;
	return res;
}

DaoTime* DaoProcess_PutTime( DaoProcess *self, DTime time, int local )
{
	DaoTime *res = (DaoTime*) DaoProcess_PutCpod( self, daox_type_time, sizeof(DaoTime) );

	if( res == NULL ) return NULL;

	if ( !DTime_IsValid( time ) ){
		DaoProcess_RaiseError( self, timeerr, "Invalid datetime" );
		return NULL;
	}
	res->time = time;
	res->local = local;
	return res;
}

DaoTime* DaoProcess_NewTime( DaoProcess *self, DTime time, int local )
{
	DaoTime *res = (DaoTime*) DaoProcess_NewCpod( self, daox_type_time, sizeof(DaoTime) );
	if ( !DTime_IsValid( time ) ){
		DaoProcess_RaiseError( self, timeerr, "Invalid datetime" );
		return NULL;
	}
	res->time = time;
	res->local = local;
	return res;
}


DaoDateSpan* DaoProcess_PutDateSpan( DaoProcess *self, DDateSpan span )
{
	DaoDateSpan *res = (DaoDateSpan*) DaoProcess_PutCpod( self, daox_type_date_span, sizeof(DaoDateSpan) );

	if( res == NULL ) return NULL;

	res->span = span;
	return res;
}

DaoDateSpan* DaoProcess_NewDateSpan( DaoProcess *self, DDateSpan span )
{
	DaoDateSpan *res = (DaoDateSpan*) DaoProcess_NewCpod( self, daox_type_date_span, sizeof(DaoDateSpan) );
	res->span = span;
	return res;
}


DaoTimeSpan* DaoProcess_PutTimeSpan( DaoProcess *self, DTimeSpan span )
{
	DaoTimeSpan *res = (DaoTimeSpan*) DaoProcess_PutCpod( self, daox_type_time_span, sizeof(DaoTimeSpan) );

	if( res == NULL ) return NULL;

	res->span = span;
	return res;
}

DaoTimeSpan* DaoProcess_NewTimeSpan( DaoProcess *self, DTimeSpan span )
{
	DaoTimeSpan *res = (DaoTimeSpan*) DaoProcess_NewCpod( self, daox_type_time_span, sizeof(DaoTimeSpan) );
	res->span = span;
	return res;
}

#undef DAO_TIME
#undef DAO_TIME_DLL
#define DAO_HAS_TIME
#include"dao_api.h"

DAO_DLL_EXPORT int DaoTime_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *timens = DaoVmSpace_GetNamespace( vmSpace, "time" );
	DTime epoch2000 = { 2000, 1, 1, 0, 0, 0.0 };
	DTime epoch1970 = { 1970, 1, 1, 0, 0, 0.0 };

	DaoNamespace_AddConstValue( ns, "time", (DaoValue*) timens );

	epoch2000_days = DTime_ToJulianDay( epoch2000 );
	epoch2000_useconds = epoch2000_days * 24 * 3600 * 1E6;
	epoch1970_seconds = DTime_ToJulianDay( epoch1970 ) * 24 * 3600;

	daox_type_time = DaoNamespace_WrapType( timens, &timeTyper, DAO_CPOD,0 );
	daox_type_date_span = DaoNamespace_WrapType( timens, &dateSpanTyper, DAO_CPOD,0 );
	daox_type_time_span = DaoNamespace_WrapType( timens, &timeSpanTyper, DAO_CPOD,0 );
	DaoNamespace_WrapFunctions( timens, timeFuncs );

#define DAO_API_INIT
#include"dao_api.h"
	return 0;
}
