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
 
typedef struct DDate    DDate;
typedef struct DTime    DTime;
typedef struct DaoDate  DaoDate;
typedef struct DaoTime  DaoTime;

typedef struct DDateSpan    DDateSpan;
typedef struct DTimeSpan    DTimeSpan;
typedef struct DaoDateSpan  DaoDateSpan;
typedef struct DaoTimeSpan  DaoTimeSpan;

/*
// Zero month indicates an invalid datetime;
*/
struct DDate
{
	int    year;
	short  month;
	short  day;
};

struct DTime
{
	int     year;
	short   month;
	short   day;
	short   hour;
	short   minute;
	double  second;
};

struct DDateSpan
{
	int    value;
	int    years;
	short  months;
	short  days;
};

struct DTimeSpan
{
	int     days;
	short   hours;
	short   minutes;
	double  seconds;
};

struct DaoDate
{
	DAO_CPOD_COMMON;

	DDate  date;
};


struct DaoTime
{
	DAO_CPOD_COMMON;

	DTime  time;
	short  local;
};

struct DaoDateSpan
{
	DAO_CPOD_COMMON;

	DDateSpan  span;
};

struct DaoTimeSpan
{
	DAO_CPOD_COMMON;

	DTimeSpan  span;
};


#endif

DAO_API( DAO_TIME_DLL, DDate, DDate_Today, (int local) );
DAO_API( DAO_TIME_DLL, DDate, DDate_FromTime, (DTime time) );
DAO_API( DAO_TIME_DLL, DDate, DDate_FromJulianDay, (int jday) );
DAO_API( DAO_TIME_DLL, int, DDate_ToJulianDay, (DDate date) );
DAO_API( DAO_TIME_DLL, DDate, DDate_FromDay, (int day) );
DAO_API( DAO_TIME_DLL, int, DDate_ToDay, (DDate time) );
DAO_API( DAO_TIME_DLL, int, DDate_Compare, (DDate first, DDate second) );

/*
// Note:
// -- Seconds in DTime_FromTime()/DTime_ToTime() are counted since 1970-1-1, 00:00:00 UTC;
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
DAO_API( DAO_TIME_DLL, dao_time_t, DTime_ToSeconds, (DTime time) );
DAO_API( DAO_TIME_DLL, dao_time_t, DTime_ToMicroSeconds, (DTime time) );
DAO_API( DAO_TIME_DLL, time_t, DTime_ToTime, (DTime time) );
DAO_API( DAO_TIME_DLL, int, DTime_IsValid, (DTime time) );
DAO_API( DAO_TIME_DLL, void, DTime_ToStructTM, (DTime time, struct tm *parts) );
DAO_API( DAO_TIME_DLL, int, DTime_ToDay, (DTime time) );
DAO_API( DAO_TIME_DLL, int, DTime_ToJulianDay, (DTime time) );
DAO_API( DAO_TIME_DLL, int, DTime_Compare, (DTime first, DTime second) );

DAO_API( DAO_TIME_DLL, DaoTime*, DaoTime_New, () );
DAO_API( DAO_TIME_DLL, void, DaoTime_Delete, (DaoTime *self) );
DAO_API( DAO_TIME_DLL, int, DaoTime_Now, (DaoTime *self) );
DAO_API( DAO_TIME_DLL, DaoType*, DaoTime_Type, () );

DAO_API( DAO_TIME_DLL, DaoDateSpan*, DaoDateSpan_New, () );
DAO_API( DAO_TIME_DLL, void, DaoDateSpan_Delete, (DaoDateSpan *self) );
DAO_API( DAO_TIME_DLL, DaoType*, DaoDateSpan_Type, () );

DAO_API( DAO_TIME_DLL, DaoTimeSpan*, DaoTimeSpan_New, () );
DAO_API( DAO_TIME_DLL, void, DaoTimeSpan_Delete, (DaoTimeSpan *self) );
DAO_API( DAO_TIME_DLL, DaoType*, DaoTimeSpan_Type, () );

DAO_API( DAO_TIME_DLL, DaoTime*, DaoProcess_PutTime, (DaoProcess *self, DTime time, int local) );
DAO_API( DAO_TIME_DLL, DaoTime*, DaoProcess_NewTime, (DaoProcess *self, DTime time, int local) );

DAO_API( DAO_TIME_DLL, DaoDate*, DaoProcess_PutDate, (DaoProcess *self, DDate date) );
DAO_API( DAO_TIME_DLL, DaoDate*, DaoProcess_NewDate, (DaoProcess *self, DDate date) );

DAO_API( DAO_TIME_DLL, DaoDateSpan*, DaoProcess_PutDateSpan, (DaoProcess *self, DDateSpan span) );
DAO_API( DAO_TIME_DLL, DaoDateSpan*, DaoProcess_NewDateSpan, (DaoProcess *self, DDateSpan span) );

DAO_API( DAO_TIME_DLL, DaoTimeSpan*, DaoProcess_PutTimeSpan, (DaoProcess *self, DTimeSpan span) );
DAO_API( DAO_TIME_DLL, DaoTimeSpan*, DaoProcess_NewTimeSpan, (DaoProcess *self, DTimeSpan span) );
