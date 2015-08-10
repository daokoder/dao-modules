/*
// Dao Standard Modules
// http://www.daovm.net
//
// Copyright (c) 2011-2015, Limin Fu
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

#include"dao.h"

#ifndef DAO_TIME_DLL
#ifdef DAO_TIME
#  define DAO_TIME_DLL DAO_DLL_EXPORT
#else
#  define DAO_TIME_DLL DAO_DLL_IMPORT
#endif
#endif


#ifndef __DAO_TIME_H__
#define __DAO_TIME_H__

#include"daoValue.h"
#include<ctype.h>


typedef long long  dao_time_t;
 
typedef struct DTime    DTime;
typedef struct DaoTime  DaoTime;

typedef struct DTimeSpan    DTimeSpan;
typedef struct DaoTimeSpan  DaoTimeSpan;

/*
// Zero month indicates an invalid datetime;
*/
struct DTime
{
	int     year;
	char    month;
	char    day;
	char    hour;
	char    minute;
	double  second;
};

/*
// The TimeSpan includes an anchor year-month-day, which are the start year-month-day
// if the span is computed from two datetimes, otherwise they take a default value of
// 2000-01-01. The anchor year-month-day are included such that the accurate calendar
// days of the span can be computed.
//
// Datetimes are converted to UTC time when computing spans.
// And for time units smaller than a day, the following is assumed:
// 1day=24hours, 1hour=60minutes and 1minute=60seconds.
*/
struct DTimeSpan
{
	int     year;     /* Anchor year,  default: 2000; */
	short   month;    /* Anchor month, default:    1; */
	short   day;      /* Anchor day,   default:    1; */
	int     years;    /* Full years within the span; */
	char    months;   /* Remaining full months within the span; */
	char    days;     /* Remaining days within the span; */
	char    hours;    /* Remaining hours; */
	char    minutes;  /* Remaining minutes; */
	double  seconds;  /* Remaining seconds; */
};

struct DaoTime
{
	DAO_CPOD_COMMON;

	DTime  time;
	short  local;
};

struct DaoTimeSpan
{
	DAO_CPOD_COMMON;

	DTimeSpan  span;
};


#endif

/*
// Note:
// -- Seconds DTime_FromTime()/DTime_ToTime() are counted since 1970-1-1, 00:00:00 UTC;
// -- Other days, seconds and micro-seconds are counted since 2000-1-1, 00:00:00 UTC;
*/
DAO_API( DAO_TIME_DLL, DTime, DTime_Now, (int local) );
DAO_API( DAO_TIME_DLL, DTime, DTime_FromTime, (time_t seconds) );
DAO_API( DAO_TIME_DLL, DTime, DTime_FromSeconds, (dao_time_t seconds) );
DAO_API( DAO_TIME_DLL, DTime, DTime_FromMicroSeconds, (dao_time_t useconds) );
DAO_API( DAO_TIME_DLL, DTime, DTime_FromDay, (int day) );
DAO_API( DAO_TIME_DLL, DTime, DTime_FromJulianDay, (int jday) );
DAO_API( DAO_TIME_DLL, DTime, DTime_LocalToUtc, (DTime local) );
DAO_API( DAO_TIME_DLL, DTime, DTime_UtcToLocal, (DTime utc) );
DAO_API( DAO_TIME_DLL, DTime, DTime_AddSpan, (DTime time, DTimeSpan span) );
DAO_API( DAO_TIME_DLL, DTime, DTime_SubSpan, (DTime time, DTimeSpan span) );
DAO_API( DAO_TIME_DLL, dao_time_t, DTime_ToSeconds, (DTime time) );
DAO_API( DAO_TIME_DLL, dao_time_t, DTime_ToMicroSeconds, (DTime time) );
DAO_API( DAO_TIME_DLL, time_t, DTime_ToTime, (DTime time) );
DAO_API( DAO_TIME_DLL, int, DTime_IsValid, (DTime time) );
DAO_API( DAO_TIME_DLL, void, DTime_ToStructTM, (DTime time, struct tm *parts) );
DAO_API( DAO_TIME_DLL, int, DTime_ToDay, (DTime time) );
DAO_API( DAO_TIME_DLL, int, DTime_ToJulianDay, (DTime time) );
DAO_API( DAO_TIME_DLL, int, DTime_Compare, (DTime first, DTime second) );

DAO_API( DAO_TIME_DLL, DTimeSpan, DTimeSpan_Init, () );
DAO_API( DAO_TIME_DLL, DTimeSpan, DTimeSpan_FromTimeInterval, (DTime start, DTime end) );
DAO_API( DAO_TIME_DLL, DTimeSpan, DTimeSpan_FromUSeconds, (dao_time_t useconds) );
DAO_API( DAO_TIME_DLL, DTime, DTimeSpan_GetStartTime, (DTimeSpan span) );
DAO_API( DAO_TIME_DLL, DTime, DTimeSpan_GetEndTime, (DTimeSpan span) );
DAO_API( DAO_TIME_DLL, int, DTimeSpan_ToDays, (DTimeSpan span) );
DAO_API( DAO_TIME_DLL, dao_time_t, DTimeSpan_ToUSeconds, (DTimeSpan span) );
DAO_API( DAO_TIME_DLL, int, DTimeSpan_IsValid, (DTimeSpan span) );
DAO_API( DAO_TIME_DLL, int, DTimeSpan_Compare, (DTimeSpan first, DTimeSpan second) );

DAO_API( DAO_TIME_DLL, DaoTime*, DaoTime_New, () );
DAO_API( DAO_TIME_DLL, void, DaoTime_Delete, (DaoTime *self) );
DAO_API( DAO_TIME_DLL, int, DaoTime_Now, (DaoTime *self) );
DAO_API( DAO_TIME_DLL, DaoType*, DaoTime_Type, () );

DAO_API( DAO_TIME_DLL, DaoTimeSpan*, DaoTimeSpan_New, () );
DAO_API( DAO_TIME_DLL, void, DaoTimeSpan_Delete, (DaoTimeSpan *self) );
DAO_API( DAO_TIME_DLL, DaoType*, DaoTimeSpan_Type, () );

DAO_API( DAO_TIME_DLL, DaoTime*, DaoProcess_PutTime, (DaoProcess *self, DTime time, int local) );
DAO_API( DAO_TIME_DLL, DaoTime*, DaoProcess_NewTime, (DaoProcess *self, DTime time, int local) );

DAO_API( DAO_TIME_DLL, DaoTimeSpan*, DaoProcess_PutTimeSpan, (DaoProcess *self, DTimeSpan span) );
DAO_API( DAO_TIME_DLL, DaoTimeSpan*, DaoProcess_NewTimeSpan, (DaoProcess *self, DTimeSpan span) );
