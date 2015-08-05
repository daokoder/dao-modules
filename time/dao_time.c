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
static DaoType *daox_type_time = NULL;
static DaoType *daox_type_span = NULL;

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

static int DaysInMonth( int y, int m )
{
	const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if ( m > 12 )
		return 0;
	return ( LeapYear( y ) && m == 2 )? 29 : days[m - 1];
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
	res.year   = systime.wYear;
	res.month  = systime.wMonth;
	res.day    = systime.wDay;
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
	res.year   = parts.tm_year + 1900;
	res.month  = parts.tm_mon + 1;
	res.day    = parts.tm_mday;
	res.hour   = parts.tm_hour;
	res.minute = parts.tm_min;
	res.second = parts.tm_sec + now.tv_usec / 1.0E6;
#endif

	return res;
}


DTime DTime_FromJulianDay( int jday )
{
	DTime time = {0,0,0,0,0,0.0};
	int F = jday + 1401 + (((4*jday + 274277) / 146097) * 3) / 4 - 38;
	int E = 4*F + 3;
	int G = (E%1461) / 4;
	int H = 5*G + 2;
	time.day = (H%153) / 5 + 1;
	time.month = ((H/153 + 2) % 12) + 1;
	time.year = (E/1461) - 4716 + (12 + 2 - time.month) / 12;
	return time;
}
int DTime_ToJulianDay( DTime time )
{
	int a = FloorDiv(14 - time.month, 12);
	int year = time.year + 4800 - a;
	int month = time.month + 12*a - 3;
	int day = time.day + FloorDiv( 153*month + 2, 5 ) + 365*year;
	day += FloorDiv( year, 4 ) - FloorDiv( year, 100 ) + FloorDiv( year, 400 );
	return day - 32045;
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
	dao_time_t seconds2 = seconds + epoch2000_useconds / 1000000;
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
	return jday*24*3600 + time.hour*3600 + time.minute*60 + (dao_time_t)time.second - epoch2000_useconds / 1E6;
}

DTime DTime_FromMicroSeconds( dao_time_t useconds )
{
	dao_time_t seconds = (useconds + epoch2000_useconds) / 1000000;
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
	parts->tm_year = time.year - 1900;
	parts->tm_mon  = time.month - 1;
	parts->tm_mday = time.day;
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
	res.year = gmt.wYear;
	res.month = gmt.wMonth;
	res.day = gmt.wDay;
	res.hour = gmt.wHour;
	res.minute = gmt.wMinute;
	res.second = gmt.wSecond;
#else
	time_t t;
	struct tm ts = {0};
	DTime_ToStructTM( local, &ts );
	t = mktime( &ts );
	gmtime_r( &t, &ts );
	res.year = ts.tm_year + 1900;
	res.month = ts.tm_mon + 1;
	res.day = ts.tm_mday;
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
	res.year = loc.wYear;
	res.month = loc.wMonth;
	res.day = loc.wDay;
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
	localtime_r( &t, &ts );
	res.year = ts.tm_year + 1900;
	res.month = ts.tm_mon + 1;
	res.day = ts.tm_mday;
	res.hour = ts.tm_hour;
	res.minute = ts.tm_min;
	res.second = ts.tm_sec;
#endif
	res.second += fract;
	return res;
}

DTime DTime_AddSpan( DTime time, DTimeSpan span )
{
	dao_time_t useconds1 = DTime_ToMicroSeconds( time );
	dao_time_t useconds2 = DTimeSpan_ToUSeconds( span );
	return DTime_FromMicroSeconds( useconds1 + useconds2 );
}

int DTime_Compare( DTime first, DTime second )
{
	int msa, msb;
	if( first.year != second.year ) return first.year < second.year ? -1 : 1;
	if( first.month != second.month ) return first.month < second.month ? -1 : 1;
	if( first.day != second.day ) return first.day < second.day ? -1 : 1;
	if( first.hour != second.hour ) return first.hour < second.hour ? -1 : 1;
	if( first.minute != second.minute ) return first.minute < second.minute ? -1 : 1;
	msa = (int)(first.second*1E6);
	msb = (int)(second.second*1E6);
	if( msa != msb ) return msa < msb ? -1 : 1;
	return 0;
}

int DTime_IsValid( DTime time )
{
	return time.year > 0 && time.month > 0 && time.month < 13 && time.day > 0 &&
			time.day <= DaysInMonth( time.year, time.month ) && time.hour >= 0 &&
			time.hour < 24 && time.minute >= 0 && time.minute < 60 &&
			time.second >= 0.0 && time.second < 60.0;
}



DTimeSpan DTimeSpan_Init()
{
	DTimeSpan span = {0};
	span.year = 2000;
	span.month = 1;
	span.day = 1;
	span.nyday = 1;
	return span;
}
DTimeSpan DTimeSpan_FromTimeInterval( DTime start, DTime end )
{
	DTimeSpan span = DTimeSpan_Init();
	int same = 1;
	int mdays = DaysInMonth( start.year, start.month );
	int midStart = start.month > 1 || start.day > 1 || start.hour > 0 || start.minute > 0;
	midStart |= (int)(start.second*1E6) > 0;
	
	span.year = start.year;
	span.month = start.month;
	span.day = start.day;
	span.nyday = midStart == 0;
	if( DTime_Compare( start, end ) > 0 ) return span;

	span.years = end.year - start.year;
	if( span.nyday == 0 ){
		if( span.years > 0 ) span.years -= 1;
	}
	same &= start.year == end.year;
	if( same ){
		span.months = end.month - start.month;
		if( span.months ) span.months -= 1;
	}else{
		span.months = 12 - start.month;
		span.months += end.month - 1;
	}
	same &= start.month == end.month;
	if( same ){
		span.days = end.day - start.day;
		if( span.days ) span.days -= 1;
	}else{
		span.days = mdays - start.day;
		span.days += end.day - 1;
	}
	same &= start.day == end.day;
	if( same ){
		span.hours = end.hour - start.hour;
		if( span.hours ) span.hours -= 1;
	}else{
		span.hours = 23 - start.hour;
		span.hours += end.hour;
	}
	same &= start.hour == end.hour;
	if( same ){
		span.minutes = end.minute - start.minute;
		if( span.minutes ) span.minutes -= 1;
	}else{
		span.minutes = 59 - start.minute;
		span.minutes += end.minute;
	}
	same &= start.minute == end.minute;
	if( same ){
		span.seconds = end.second - start.second;
	}else{
		span.seconds = 60.0 - start.second;
		span.seconds += end.second;
	}
	/*
	// Now:
	// -- years are full years from Jan1 to Dec31;
	// -- months are full months from the first days to the last days of the months;
	// -- days are full days from 00:00:00 to the 00:00:00 of the next day;
	// -- hours are full hours from 00:00 to the 00:00 of the next hour;
	// -- minutes are full hours from 00 to the 00 of the next hour;
	// -- seconds are the sum of the trailing seconds of the minute in the start time
	//    and the leading seconds of the minute in the end time;
	//
	// The following will handle the carry over:
	*/
	if( span.seconds >= 60.0 ){
		span.seconds -= 60.0;
		span.minutes += 1;
	}
	if( span.minutes >= 60 ){
		span.minutes -= 60;
		span.hours += 1;
	}
	if( span.hours >= 24 ){
		span.hours -= 24;
		span.days += 1;
	}
	if( span.days >= mdays ){
		span.days -= mdays;
		span.months += 1;
	}
	if( span.months >= 12 ){
		span.months -= 12;
		span.years += 1;
	}
	return span;
}
int DTimeSpan_ToDays( DTimeSpan span )
{
	int years = span.years;
	int months = span.months;
	int days = span.days;
	int mdays = DaysInMonth( span.year, span.month );
	int i, month, year;

	/* Reverse the carray-over down to days: */
	if( span.nyday == 0 ){
		/* A carray-over year (12 full months from two partial years): */
		if( months < 12 - span.month ){
			years -= 1;
			months += 12;
		}
		/* A carray-over month (mdays full days from two partial months): */
		if( days < (mdays - span.day) ){
			months -= 1;
			days += mdays;
		}
	}
	/* Days of full years: */
	for(i=0; i<years; ++i){
		year = span.year + i + (span.nyday == 0);
		days += 365 + LeapYear( year );
	}
	month = months;
	if( span.nyday == 0 ){
		/* Days of the full months in the starting partial year: */
		for(i=span.month+1; i<=12; ++i){
			days += DaysInMonth( span.year, i );
		}
		month = months - (12 - span.month);
	}
	year = span.year + years + (span.nyday == 0);
	/* Days of the full months in the ending partial year: */
	for(i=0; i<month; ++i){
		days += DaysInMonth( year, i + 1 );
	}
	//printf( "%i: %i\n", __LINE__, days );
	return days;
}
DTimeSpan DTimeSpan_FromUSeconds( dao_time_t useconds )
{
	DTime epoch = DTime_FromMicroSeconds( 0 );
	DTime time  = DTime_FromMicroSeconds( useconds );
	if( useconds < 0 ) return DTimeSpan_FromTimeInterval( time, epoch );
	return DTimeSpan_FromTimeInterval( epoch, time );
}
dao_time_t DTimeSpan_ToUSeconds( DTimeSpan span )
{
	dao_time_t useconds = span.seconds * 1E6;
	useconds += DTimeSpan_ToDays( span ) * 24 * 3600 * 1E6;
	useconds += (span.hours * 3600 + span.minutes * 60) * 1E6;
	return useconds;
}
int DTimeSpan_IsValid( DTimeSpan span )
{
	DTimeSpan span2;
	DTime end, start = {2000,1,1,0,0,0.0};

	start.year = span.year;
	start.month = span.month;
	start.day = span.day;

	end = DTime_AddSpan( start, span );
	span2 = DTimeSpan_FromTimeInterval( start, end );

	return DTimeSpan_Compare( span, span2 ) == 0;
}
int DTimeSpan_Compare( DTimeSpan first, DTimeSpan second )
{
	int msa, msb;
	if( first.years  != second.years  ) return first.years  < second.years  ? -1 : 1;
	if( first.months != second.months ) return first.months < second.months ? -1 : 1;
	if( first.days   != second.days   ) return first.days   < second.days   ? -1 : 1;
	if( first.hours  != second.hours  ) return first.hours  < second.hours  ? -1 : 1;
	if( first.minutes != second.minutes ) return first.minutes < second.minutes ? -1 : 1;
	msa = (int)(first.seconds*1E6);
	msb = (int)(second.seconds*1E6);
	if( msa != msb ) return msa < msb ? -1 : 1;
	return 0;
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
	return self->time.month;
}

DTime DaoTime_Get( DaoTime *self, int local )
{
	if( self->local == local ) return self->time;
	if( local ) return DTime_UtcToLocal( self->time );
	return DTime_LocalToUtc( self->time );
}

DaoType* DaoTime_Type()
{
	return daox_type_time;
}



DaoTimeSpan* DaoTimeSpan_New()
{
	DaoTimeSpan *self = (DaoTimeSpan*) DaoCpod_New( daox_type_span, sizeof(DaoTimeSpan) );
	return self;
}
void DaoTimeSpan_Delete( DaoTimeSpan *self )
{
	DaoCpod_Delete( (DaoCpod*) self );
}
DaoType* DaoTimeSpan_Type()
{
	return daox_type_span;
}



static void TIME_Now( DaoProcess *proc, DaoValue *p[], int N )
{
	int local = p[0]->xEnum.value == 0;
	DTime time = DTime_Now( local );
	DaoTime *self = DaoProcess_PutTime( proc, time, local );
	if ( time.month == 0 ){
		DaoProcess_RaiseError( proc, timeerr, "Failed to get current datetime" );
		return;
	}
}

static void TIME_Time( DaoProcess *proc, DaoValue *p[], int N )
{
	DTime time = DTime_FromMicroSeconds( p[0]->xFloat.value*1E6 );
	DaoProcess_PutTime( proc, time, 0 );
	if( time.month == 0 ){
		DaoProcess_RaiseError( proc, timeerr, "Invalid datetime" );
		return;
	}
}

double TruncateSeconds( double seconds )
{
	dao_time_t fract = seconds*1E6;
	return fract/1E6;
}

static void TIME_MakeTime( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self;
	struct tm parts;
	DTime time;
	int i;
	for ( i = 0; i < 5; ++i )
		if ( p[i]->xInteger.value < 0 ){
			DaoProcess_RaiseError( proc, timeerr, "Invalid datetime" );
			return;
		}

	time.year   = p[0]->xInteger.value;
	time.month  = p[1]->xInteger.value;
	time.day    = p[2]->xInteger.value;
	time.hour   = p[3]->xInteger.value;
	time.minute = p[4]->xInteger.value;
	time.second = TruncateSeconds( p[5]->xFloat.value );

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
	res.year = ( cp[0] - '0' )*1000 + ( cp[1] - '0' )*100 + ( cp[2] - '0' )*10 + ( cp[3] - '0' );
	cp += 5;

	if ( !isdigit(cp[0]) || !isdigit(cp[1]) || cp[2] != '-' )
		return inv_time;
	res.month = ( cp[0] - '0' )*10 + ( cp[1] - '0' );
	if ( res.month == 0 || res.month > 12 )
		return inv_time;
	cp += 3;

	if ( !isdigit(cp[0]) || !isdigit(cp[1]) || cp[2] != 'T' )
		return inv_time;
	res.day = ( cp[0] - '0' )*10 + ( cp[1] - '0' );
	if ( res.day == 0 || res.day > DaysInMonth( res.year, res.month ) )
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
		res.second = TruncateSeconds( num );
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

DTime ParseSimpleTime( DaoProcess *proc, DString *str, int *failed )
{
	DTime inv_time = {0};
	DTime time = {0};
	DString *sdate = NULL, *stime = NULL;
	DString *sfrac = NULL;
	daoint pos;

	*failed = 0;
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
			time.year = GetNum( sdate->chars, 4 );
			time.month = GetNum( sdate->chars + 5, 2 );
			time.day = GetNum( sdate->chars + 8, 2 );
		} else if ( bits == ToBits( "00-00", 5 ) ){ /* MM-DD */
			DTime now = DTime_Now( 1 );
			if( now.month == 0 ){
				DaoProcess_RaiseError( proc, timeerr, "Failed to get current datetime" );
				*failed = 1;
				return inv_time;
			}
			time.year = now.year;
			time.month = GetNum( sdate->chars, 2 );
			time.day = GetNum( sdate->chars + 3, 2 );
		} else {
			goto Error;
		}
	}
	else {
		time = DTime_Now( 1 );
		if( time.month == 0 ){
			DaoProcess_RaiseError( proc, timeerr, "Failed to get current datetime" );
			*failed = 1;
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
		time.second += TruncateSeconds( strtod( sfrac->chars, NULL ) );
	}
	goto Clean;
Error:
	time = inv_time;
Clean:
	if( str ) DString_Delete( str );
	if( sdate ) DString_Delete( sdate );
	if( stime ) DString_Delete( stime );
	if( sfrac ) DString_Delete( sfrac );
	return DTime_IsValid( time )? time : inv_time;
}

static void TIME_Parse( DaoProcess *proc, DaoValue *p[], int N )
{
	DString *str = p[0]->xString.value;
	int local = 0, failed;
	DTime t = ParseRfc3339Time( str );

	if ( !t.month ){
		local = 1;
		t = ParseSimpleTime( proc, str, &failed );

		if ( !t.month && !failed ){
			DaoProcess_RaiseError( proc, "Time", "Unsupported datetime format or invalid datetime" );
			return;
		}
	}
	DaoProcess_PutTime( proc, t, local );
}

static void TIME_Value( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutFloat( proc, DTime_ToMicroSeconds( self->local? DTime_LocalToUtc( self->time ) : self->time ) / 1.0E6 );
}

static void TIME_Type( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutEnum( proc, self->local? "local" : "utc" );
}

static void TIME_Convert( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];

	if( self->local != (p[1]->xEnum.value == 0) )
		DaoProcess_PutTime( proc, p[1]->xEnum.value == 0? DTime_UtcToLocal( self->time ) : DTime_LocalToUtc( self->time ), !self->local );
}

static void TIME_Second( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutFloat( proc, self->time.second );
}

static void TIME_Minute( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutInteger( proc, self->time.minute );
}

static void TIME_Hour( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutInteger( proc, self->time.hour );
}

static void TIME_Day( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutInteger( proc, self->time.day );
}

static void TIME_Month( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutInteger( proc, self->time.month );
}

static void TIME_Year( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DaoProcess_PutInteger( proc, self->time.year );
}

static void TIME_SetSecond( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	dao_float value = p[1]->xFloat.value;
	if( value < 0.0 || value >= 60.0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid seconds" );
		return;
	}
	self->time.second = TruncateSeconds( value );
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
	int days = DaysInMonth( self->time.year, self->time.month );
	if( value < 1 || value > days ){
		DaoProcess_RaiseError( proc, "Param", "Invalid day" );
		return;
	}
	self->time.day = value;
}

static void TIME_SetMonth( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	int value = p[1]->xInteger.value;
	if( value < 1 || value > 12 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid month" );
		return;
	}
	self->time.month = value;
}

static void TIME_SetYear( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	int value = p[1]->xInteger.value;
	if ( value < 0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid year" );
		return;
	}
	if ( !LeapYear( value ) && self->time.month == 2 && self->time.day == 29 ){
		DaoProcess_RaiseError( proc, "Param", "Specified year results in invalid datetime" );
		return;
	}
	self->time.year = value;
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
	nyday.month = 1;
	nyday.day = 1;
	DaoProcess_PutInteger( proc, DTime_ToJulianDay( self->time ) - DTime_ToJulianDay( nyday ) + 1 );
}

static void TIME_Zone( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTuple *res = DaoProcess_PutTuple( proc, 4 );
	tzset();
	res->values[0]->xBoolean.value = daylight > 0;
	res->values[1]->xInteger.value = -timezone;
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
				sprintf( p1, "%i", self->time.year );
				break;
			case 'y' :
				sprintf( p1, "%i", self->time.year );
				p2 += 2;
				break;
			case 'M' :
			case 'm' :
				if( ! addStringFromMap( key, S, sym, "month", self->time.month-1 ) ){
					sprintf( p1, "%i", self->time.month );
					if( ch=='M' && p1[1]==0 ) p2 = buf; /* padding 0; */
				}else p2 = NULL;
				break;
			case 'D' :
			case 'd' :
				if( ! addStringFromMap( key, S, sym, "date", self->time.day ) ){
					sprintf( p1, "%i", self->time.day );
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

static void TIME_Days( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	if ( p[1]->xEnum.value == 0 )
		DaoProcess_PutInteger( proc, DaysInMonth( self->time.year, self->time.month ) );
	else
		DaoProcess_PutInteger( proc, LeapYear( self->time.year )? 366 : 365 );
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

static void TIME_Add( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DTime restime = self->time;
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
	if ( years || months ){
		restime.year += years;
		restime.year += months/12;
		restime.month += months%12;
		if ( restime.month > 12 ){
			restime.year += 1;
			restime.month -= 12;
		} else if ( restime.month < 1 ){
			restime.year--;
			restime.month = 12 - restime.month;
		}
		d = DaysInMonth( restime.year, restime.month );
		if ( restime.day > d ){
			days += restime.day - d;
			restime.day = d;
		}
	}
	if ( days ){
		int jday = DTime_ToJulianDay( restime ) + days;
		restime = DTime_FromJulianDay( jday );
	}
	self->time = restime;
}
static void TIME_Plus( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	DTime res = DTime_AddSpan( a->time, b->span );
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
	DTime atime = DaoTime_Get( a, 0 );
	DTime btime = DaoTime_Get( b, 0 );
	if( DTime_Compare( atime, btime ) < 0 ){
		DaoProcess_RaiseError( proc, "Param", "Subtracting leading to negative result" );
		return;
	}
	DaoProcess_PutTimeSpan( proc, DTimeSpan_FromTimeInterval( btime, atime ) );
}

static void TIME_Since( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *self = (DaoTime*) p[0];
	DTime then = DaoTime_Get( self, 0 );
	DTime now = DTime_Now( 0 );
	if( DTime_Compare( now, then ) < 0 ){
		DaoProcess_RaiseError( proc, "Param", "Subtracting leading to negative result" );
		return;
	}
	DaoProcess_PutTimeSpan( proc, DTimeSpan_FromTimeInterval( then, now ) );
}

static void TIME_DayDiff( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTime *a = (DaoTime*) p[0];
	DaoTime *b = (DaoTime*) p[1];
	DTime atime = a->time;
	DTime btime = b->time;
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
	/*! \c Returns the number of seconds elapsed since 2000-1-1, 00:00:00 UTC */
	{ TIME_Value,   ".value(invar self: DateTime) => float" },
	{ TIME_Value,   "(float)(invar self: DateTime)" },

	/*! Returns datetime kind with regard to the time zone: UTC or local */
	{ TIME_Type,    ".kind(invar self: DateTime) => enum<local,utc>" },

	/*! Converts datetime to the given \a kind */
	{ TIME_Convert, "as(self: DateTime, kind: enum<local,utc>) => DateTime" },

	/*! Specific datetime part */
	{ TIME_Second,  ".second(invar self: DateTime) => float" },
	{ TIME_Minute,  ".minute(invar self: DateTime) => int" },
	{ TIME_Hour,    ".hour(invar self: DateTime) => int" },
	{ TIME_Day,     ".day(invar self: DateTime) => int" },
	{ TIME_Month,   ".month(invar self: DateTime) => int" },
	{ TIME_Year,    ".year(invar self: DateTime) => int" },

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

	/*! Returns the number of day in the month or year of the given datetime depending on the \a period parameter */
	{ TIME_Days,  "daysIn(invar self: DateTime, period: enum<month,year>) => int" },

	/*! Returns new datetime obtained by adding the specified number of years, months or days (provided as named values) */
	{ TIME_Add,	 "add(self: DateTime, ...: tuple<enum<years,months,days>,int>)" },

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


static int SPAN_SetFields( DTimeSpan *span, DaoProcess *proc, DaoValue *p[], int N )
{
	int i;

	for(i = 0; i < N; i++ ){
		DaoEnum *en = (DaoEnum*) p[i]->xTuple.values[0];
		if ( strstr( en->etype->name->chars, "second" ) != NULL ){
			span->seconds = TruncateSeconds( p[i]->xTuple.values[1]->xFloat.value ); //sec
		}else{
			dao_integer val = p[i]->xTuple.values[1]->xInteger.value;
			switch ( p[i]->xTuple.values[0]->xEnum.value ){
			case 0:  span->years   = val; break; // year
			case 1:  span->months  = val; break; // month
			case 2:  span->days    = val; break; // day
			case 3:  span->hours   = val; break; // hour
			case 4:  span->minutes = val; break; // min
			default: break;
			}
		}
	}
	if ( !DTimeSpan_IsValid( *span )){
		DaoProcess_RaiseError( proc, timeerr, "Invalid time span" );
		return 0;
	}
	return 1;
}

static void SPAN_New( DaoProcess *proc, DaoValue *p[], int N )
{
	DTimeSpan span = DTimeSpan_Init();
	if( SPAN_SetFields( & span, proc, p, N ) == 0 ) return;
	DaoProcess_PutTimeSpan( proc, span );
}

static void SPAN_Set( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	if( SPAN_SetFields( & self->span, proc, p+1, N-1 ) == 0 ) return;
}

static void SPAN_GetYears( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	DaoProcess_PutInteger( proc, self->span.years );
}

static void SPAN_GetMonths( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	DaoProcess_PutInteger( proc, self->span.months );
}

static void SPAN_GetDays( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	DaoProcess_PutInteger( proc, self->span.days );
}

static void SPAN_GetHours( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	DaoProcess_PutInteger( proc, self->span.hours );
}

static void SPAN_GetMinutes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	DaoProcess_PutInteger( proc, self->span.minutes );
}

static void SPAN_GetSeconds( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	DaoProcess_PutFloat( proc, self->span.seconds );
}


static void SPAN_SetDays( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	int value = p[1]->xInteger.value;
	if( value < 0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid days" );
		return;
	}
	self->span.days = value;
}

static void SPAN_SetHours( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	int value = p[1]->xInteger.value;
	if( value < 0 || value >= 24 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid hours" );
		return;
	}
	self->span.hours = value;
}

static void SPAN_SetMinutes( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	int value = p[1]->xInteger.value;
	if( value < 0 || value >= 60 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid minutes" );
		return;
	}
	self->span.minutes = value;
}

static void SPAN_SetSeconds( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	dao_float value = p[1]->xFloat.value;
	if( value < 0.0 || value >= 60.0 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid seconds" );
		return;
	}
	self->span.seconds = value;
}

static void SPAN_Plus( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *s1 = (DaoTimeSpan*) p[0];
	DaoTimeSpan *s2 = (DaoTimeSpan*) p[1];
	dao_time_t us1 = DTimeSpan_ToUSeconds( s1->span );
	dao_time_t us2 = DTimeSpan_ToUSeconds( s2->span );
	DTimeSpan res = DTimeSpan_FromUSeconds( us1 + us2 );
	DaoProcess_PutTimeSpan( proc, res );
}
static void SPAN_Plus2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *s1 = (DaoTimeSpan*) p[0];
	DaoTimeSpan *s2 = (DaoTimeSpan*) p[1];
	dao_time_t us1 = DTimeSpan_ToUSeconds( s1->span );
	dao_time_t us2 = DTimeSpan_ToUSeconds( s2->span );
	s1->span = DTimeSpan_FromUSeconds( us1 + us2 );
}
static void SPAN_Minus( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *s1 = (DaoTimeSpan*) p[0];
	DaoTimeSpan *s2 = (DaoTimeSpan*) p[1];
	dao_time_t us1 = DTimeSpan_ToUSeconds( s1->span );
	dao_time_t us2 = DTimeSpan_ToUSeconds( s2->span );
	DTimeSpan res = DTimeSpan_FromUSeconds( us1 - us2 );
	DaoProcess_PutTimeSpan( proc, res );
}
static void SPAN_Minus2( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *s1 = (DaoTimeSpan*) p[0];
	DaoTimeSpan *s2 = (DaoTimeSpan*) p[1];
	dao_time_t us1 = DTimeSpan_ToUSeconds( s1->span );
	dao_time_t us2 = DTimeSpan_ToUSeconds( s2->span );
	s1->span = DTimeSpan_FromUSeconds( us1 - us2 );
}

static void SPAN_EQ( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *a = (DaoTimeSpan*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	DaoProcess_PutBoolean( proc, DTimeSpan_Compare( a->span, b->span ) == 0 );
}
static void SPAN_NE( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *a = (DaoTimeSpan*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	DaoProcess_PutBoolean( proc, DTimeSpan_Compare( a->span, b->span ) != 0 );
}
static void SPAN_LT( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *a = (DaoTimeSpan*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	DaoProcess_PutBoolean( proc, DTimeSpan_Compare( a->span, b->span ) < 0 );
}
static void SPAN_LE( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *a = (DaoTimeSpan*) p[0];
	DaoTimeSpan *b = (DaoTimeSpan*) p[1];
	DaoProcess_PutBoolean( proc, DTimeSpan_Compare( a->span, b->span ) <= 0 );
}

static void SPAN_ToString( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoTimeSpan *self = (DaoTimeSpan*) p[0];
	char buf[100];
	int len = 0;
	if ( self->span.years )
		len += snprintf( buf, sizeof(buf), "%iy", self->span.years );
	if ( self->span.months )
		len += snprintf( buf + len, sizeof(buf) - len, "%imo", self->span.months );
	if ( self->span.days )
		len += snprintf( buf + len, sizeof(buf) - len, "%id", self->span.days );
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

static void SPAN_Parse( DaoProcess *proc, DaoValue *p[], int N )
{
	DTimeSpan ts = DTimeSpan_Init();
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

static DaoFuncItem spanMeths[] =
{
	{ SPAN_New, "TimeSpan( ...: tuple<enum<years,months,days,hours,minutes>,int> | tuple<enum<seconds>,float>)" },

	{ SPAN_Set, "set( self: TimeSpan, ...: tuple<enum<years,months,days,hours,minutes>,int> | tuple<enum<seconds>,float>)" },

	{ SPAN_GetYears,    ".years(invar self: TimeSpan) => int" },
	{ SPAN_GetMonths,   ".months(invar self: TimeSpan) => int" },
	{ SPAN_GetDays,     ".days(invar self: TimeSpan) => int" },
	{ SPAN_GetHours,    ".hours(invar self: TimeSpan) => int" },
	{ SPAN_GetMinutes,  ".minutes(invar self: TimeSpan) => int" },
	{ SPAN_GetSeconds,  ".seconds(invar self: TimeSpan) => float" },

#if 0
	{ SPAN_SetDays,     ".days=(self: TimeSpan, value: int)" },
	{ SPAN_SetHours,    ".hours=(self: TimeSpan, value: int)" },
	{ SPAN_SetMinutes,  ".minutes=(self: TimeSpan, value: int)" },
	{ SPAN_SetSeconds,  ".seconds=(self: TimeSpan, value: float)" },
#endif

	{ SPAN_Plus,    "+  (invar a: TimeSpan, invar b: TimeSpan) => TimeSpan" },
	{ SPAN_Minus,   "-  (invar a: TimeSpan, invar b: TimeSpan) => TimeSpan" },
	{ SPAN_Plus2,   "+= (self: TimeSpan, invar span: TimeSpan)" },
	{ SPAN_Minus2,  "-= (self: TimeSpan, invar span: TimeSpan)" },

	{ SPAN_EQ,  "== (invar a: TimeSpan, invar b: TimeSpan) => bool" },
	{ SPAN_NE,  "!= (invar a: TimeSpan, invar b: TimeSpan) => bool" },
	{ SPAN_LT,  "<  (invar a: TimeSpan, invar b: TimeSpan) => bool" },
	{ SPAN_LE,  "<= (invar a: TimeSpan, invar b: TimeSpan) => bool" },

	{ SPAN_ToString, "(string)(invar self: TimeSpan)" },

	{ NULL, NULL }
};

DaoTypeBase spanTyper =
{
	"TimeSpan", NULL, NULL, spanMeths, {NULL}, {0},
	(FuncPtrDel)DaoTimeSpan_Delete, NULL
};


static DaoFuncItem timeFuncs[] =
{
	/*! Returns current datetime of the given \a kind */
	{ TIME_Now,  "now(kind: enum<local,utc> = $local) => DateTime" },

	/*! Returns UTC datetime for given the number of seconds elapsed since 2000-1-1, 00:00:00 UTC */
	{ TIME_Time,  "fromValue(value: float) => DateTime" },

	/*! Returns local datetime composed of the specified \a year, \a month, \a day, \a hour, \a min and \a sec */
	{ TIME_MakeTime, "make(year: int, month: int, day: int, hour = 0, min = 0, sec = 0.0) => DateTime" },

	/*! Returns datetime parsed from \a value, which may be
	 * - local datetime consisting of date ('YYYY-MM-DD' or 'MM-DD') and/or time ('HH:MM:SS' or 'HH:MM') separated by ' '
	 * - RFC 3339 datetime returned as UTC */
	{ TIME_Parse,  "parse(value: string) => DateTime" },

	/*! Parses \c TimeSpan from \a value. Examples: '1d 3h 5m', '10m12.34s', '300ms'. Accepted units: d, h, m, s, ms, us.
	 * Seconds may have fractional part, other units must be integer numbers. When seconds are given as a fractional value,
	 * ms and us must not be present */
	{ SPAN_Parse,  "span(value: string) => TimeSpan" },

	/*! Difference between the specified \a time and \c time.now() */
	{ TIME_Since,  "since(time: DateTime) => TimeSpan" },

	/*! Number of calendar days between \a start and \a end dates */
	{ TIME_DayDiff,  "daysBetween(start: DateTime, end: DateTime) => int" },

	/*! Returns local time zone information (environment variable *TZ*):
	 * -\c dst -- is Daylight Saving Time (DST) used
	 * -\c offset -- offset in seconds from UTC
	 * -\c name -- zone name
	 * -\c dstZone -- DST zone name */
	{ TIME_Zone,  "zone() => tuple<dst: bool, offset: int, name: string, dstZone: string>" },
	{ NULL, NULL }
};

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


DaoTimeSpan* DaoProcess_PutTimeSpan( DaoProcess *self, DTimeSpan span )
{
	DaoTimeSpan *res = (DaoTimeSpan*) DaoProcess_PutCpod( self, daox_type_span, sizeof(DaoTimeSpan) );

	if( res == NULL ) return NULL;

	res->span = span;
	return res;
}

DaoTimeSpan* DaoProcess_NewTimeSpan( DaoProcess *self, DTimeSpan span )
{
	DaoTimeSpan *res = (DaoTimeSpan*) DaoProcess_NewCpod( self, daox_type_span, sizeof(DaoTimeSpan) );
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
	daox_type_span = DaoNamespace_WrapType( timens, &spanTyper, DAO_CPOD,0 );
	DaoNamespace_WrapFunctions( timens, timeFuncs );

#define DAO_API_INIT
#include"dao_api.h"
	return 0;
}
