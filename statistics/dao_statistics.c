/*
// Dao Virtual Machine
// http://www.daovm.net
//
// Copyright (c) 2006-2014, Limin Fu
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

#include <math.h>
#include "daoNumtype.h"
#include "daoValue.h"

double Mean( DaoArray *arr )
{
	daoint size = DaoArray_GetWorkSize( arr );
	daoint start = DaoArray_GetWorkStart( arr );
	daoint len = DaoArray_GetWorkIntervalSize( arr );
	daoint step = DaoArray_GetWorkStep( arr );
	daoint i, j;
	double sum = 0;
	arr = DaoArray_GetWorkArray( arr );
	if ( !size )
		return 0;
	switch ( arr->etype ){
	case DAO_INTEGER:
		for( i = 0; i < size; i++ ){
			j = start + (i / len) * step + (i % len);
			sum += arr->data.i[j];
		}
		break;
	case DAO_FLOAT:
		for( i = 0; i < size; i++ ){
			j = start + (i / len) * step + (i % len);
			sum += arr->data.f[j];
		}
		break;
	case DAO_DOUBLE:
		for( i = 0; i < size; i++ ){
			j = start + (i / len) * step + (i % len);
			sum += arr->data.d[j];
		}
		break;
	}
	return sum/arr->size;
}

double Variance( DaoArray *arr, double mean, int sample )
{
	daoint size = DaoArray_GetWorkSize( arr );
	daoint start = DaoArray_GetWorkStart( arr );
	daoint len = DaoArray_GetWorkIntervalSize( arr );
	daoint step = DaoArray_GetWorkStep( arr );
	daoint i;
	double sum = 0;
	arr = DaoArray_GetWorkArray( arr );
	if ( !size || ( sample && size == 1 ) )
		return 0;
	switch ( arr->etype ){
	case DAO_INTEGER:
		for( i = 0; i < size; i++ ){
			daoint j = start + (i / len) * step + (i % len);
			double x = arr->data.i[j];
			sum += ( x - mean )*( x - mean );
		}
		break;
	case DAO_FLOAT:
		for( i = 0; i < size; i++ ){
			daoint j = start + (i / len) * step + (i % len);
			double x = arr->data.f[j];
			sum += ( x - mean )*( x - mean );
		}
		break;
	case DAO_DOUBLE:
		for( i = 0; i < size; i++ ){
			daoint j = start + (i / len) * step + (i % len);
			double x = arr->data.d[j];
			sum += ( x - mean )*( x - mean );
		}
		break;
	}
	return sum/( size - ( sample? 1 : 0 ) );
}

double CorrelationPearson( DaoArray *arr1, DaoArray *arr2, double mean1, double mean2 )
{
	daoint size1 = DaoArray_GetWorkSize( arr1 );
	daoint start1 = DaoArray_GetWorkStart( arr1 );
	daoint len1 = DaoArray_GetWorkIntervalSize( arr1 );
	daoint step1 = DaoArray_GetWorkStep( arr1 );
	daoint size2 = DaoArray_GetWorkSize( arr2 );
	daoint start2 = DaoArray_GetWorkStart( arr2 );
	daoint len2 = DaoArray_GetWorkIntervalSize( arr2 );
	daoint step2 = DaoArray_GetWorkStep( arr2 );
	daoint i;
	double sum = 0, dev1, dev2;
	arr1 = DaoArray_GetWorkArray( arr1 );
	arr2 = DaoArray_GetWorkArray( arr2 );
	if ( size1 != size2 )
		return 0;
	if ( !size1 || arr1->etype != arr2->etype )
		return 0;
	dev1 = sqrt( Variance( arr1, mean1, 0 ) );
	dev2 = sqrt( Variance( arr2, mean2, 0 ) );
	switch ( arr1->etype ){
	case DAO_INTEGER:
		for( i = 0; i < size1; i++ ){
			daoint j1 = start1 + (i / len1) * step1 + (i % len1);
			daoint j2 = start2 + (i / len2) * step2 + (i % len2);
			double x = arr1->data.i[j1];
			double y = arr2->data.i[j2];
			sum += ( x - mean1 )*( y - mean2 );
		}
		break;
	case DAO_FLOAT:
		for( i = 0; i < size1; i++ ){
			daoint j1 = start1 + (i / len1) * step1 + (i % len1);
			daoint j2 = start2 + (i / len2) * step2 + (i % len2);
			double x = arr1->data.f[j1];
			double y = arr2->data.f[j2];
			sum += ( x - mean1 )*( y - mean2 );
		}
		break;
	case DAO_DOUBLE:
		for( i = 0; i < size1; i++ ){
			daoint j1 = start1 + (i / len1) * step1 + (i % len1);
			daoint j2 = start2 + (i / len2) * step2 + (i % len2);
			double x = arr1->data.d[j1];
			double y = arr2->data.d[j2];
			sum += ( x - mean1 )*( y - mean2 );
		}
		break;
	}
	return sum/size1/dev1/dev2;
}

int CompareDoubles( const void *x, const void *y )
{
	return *(double*)x > *(double*)y;
}

void CopyToDouble( DaoArray *src, DaoArray *dest )
{
	daoint size = DaoArray_GetWorkSize( src );
	daoint start = DaoArray_GetWorkStart( src );
	daoint len = DaoArray_GetWorkIntervalSize( src );
	daoint step = DaoArray_GetWorkStep( src );
	daoint i, j;
	src = DaoArray_GetWorkArray( src );
	DaoArray_ResizeVector( dest, size );
	switch ( src->etype ){
	case DAO_INTEGER:
		for( i = 0; i < size; i++ ){
			j = start + (i / len) * step + (i % len);
			dest->data.d[i] = src->data.i[j];
		}
		break;
	case DAO_FLOAT:
		for( i = 0; i < size; i++ ){
			j = start + (i / len) * step + (i % len);
			dest->data.d[i] = src->data.f[j];
		}
		break;
	case DAO_DOUBLE:
		for( i = 0; i < size; i++ ){
			j = start + (i / len) * step + (i % len);
			dest->data.d[i] = src->data.d[j];
		}
		break;
	}
}

void ValuesToRanks( DaoArray *arr )
{
	daoint i;
	if ( arr->etype != DAO_DOUBLE )
		return;
	for ( i = 0; i < arr->size; ){
		daoint j, count = 1, rank = i + 1;
		for ( j = i + 1; j < arr->size && arr->data.d[i] == arr->data.d[j]; j++ ){
			rank += j + 1;
			count++;
		}
		for ( j = i; j < i + count; j++ )
			arr->data.d[j] = rank/(double)count;
		i += count;
	}
}

double CorrelationSpearman( DaoArray *arr1, DaoArray *arr2 )
{
	daoint size = arr1->size;
	DaoArray *ranks1, *ranks2;
	double res;
	if ( arr2->size != size )
		return 0;
	if ( !size || arr1->etype != arr2->etype )
		return 0;
	ranks1 = DaoArray_New( DAO_DOUBLE );
	ranks2 = DaoArray_New( DAO_DOUBLE );
	CopyToDouble( arr1, ranks1 );
	CopyToDouble( arr2, ranks2 );
	qsort( ranks1->data.p, size, sizeof(double), CompareDoubles );
	qsort( ranks2->data.p, size, sizeof(double), CompareDoubles );
	ValuesToRanks( ranks1 );
	ValuesToRanks( ranks2 );
	res = CorrelationPearson( ranks1, ranks2, Mean( ranks1 ), Mean( ranks2 ) );
	DaoArray_Delete( ranks1 );
	DaoArray_Delete( ranks2 );
	return res;
}

double Moment( DaoArray *arr, int kurtosis, double mean )
{
	daoint size = DaoArray_GetWorkSize( arr );
	daoint start = DaoArray_GetWorkStart( arr );
	daoint len = DaoArray_GetWorkIntervalSize( arr );
	daoint step = DaoArray_GetWorkStep( arr );
	daoint i;
	arr = DaoArray_GetWorkArray( arr );
	double sum = 0, dev, pw = kurtosis? 4 : 3;
	if ( !size )
		return 0;
	dev = pow( Variance( arr, mean, 0 ), 0.5 );
	switch ( arr->etype ){
	case DAO_INTEGER:
		for( i = 0; i < size; i++ ){
			daoint j = start + (i / len) * step + (i % len);
			daoint x = arr->data.i[j];
			sum += pow( ( x - mean )/dev, pw );
		}
		break;
	case DAO_FLOAT:
		for( i = 0; i < size; i++ ){
			daoint j = start + (i / len) * step + (i % len);
			daoint x = arr->data.f[j];
			sum += pow( ( x - mean )/dev, pw );
		}
		break;
	case DAO_DOUBLE:
		for( i = 0; i < size; i++ ){
			daoint j = start + (i / len) * step + (i % len);
			daoint x = arr->data.d[j];
			sum += pow( ( x - mean )/dev, pw );
		}
		break;
	}
	return sum/size - ( kurtosis? 3 : 0 );
}

static void DaoStat_Mean( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr = &p[0]->xArray;
	if ( !DaoArray_GetWorkSize( arr ) ){
		DaoProcess_RaiseError( proc, "Value", "Empty sample" );
		return;
	}
	DaoProcess_PutDouble( proc, Mean( arr ) );
}

static void DaoStat_Variance( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr = &p[0]->xArray;
	double mean;
	int sample = 1;
	if ( N == 2 && p[1]->type == DAO_ENUM )
		sample = p[1]->xEnum.value == 0;
	else if ( N == 3 && p[2]->type == DAO_ENUM )
		sample = p[2]->xEnum.value == 0;
	if ( !DaoArray_GetWorkSize( arr ) ){
		DaoProcess_RaiseError( proc, "Value", "Empty sample" );
		return;
	}
	if ( sample && DaoArray_GetWorkSize( arr ) == 1 ){
		DaoProcess_RaiseError( proc, "Value", "Calculating sample variance on a single-element sample" );
		return;
	}
	if ( N > 1 && p[1]->type == DAO_DOUBLE )
		mean =  p[1]->xDouble.value;
	else
		mean = Mean( arr );
	DaoProcess_PutDouble( proc, Variance( arr, mean, sample ) );
}

static void DaoStat_Percentile( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr = &p[0]->xArray;
	daoint i, j, count;
	double perc = N == 1? 50.0 : p[1]->xDouble.value;
	int odd;
	DaoArray_Sliced( arr );
	if ( !arr->size ){
		DaoProcess_RaiseError( proc, "Value", "Empty sample" );
		return;
	}
	if ( perc <= 0 || perc >= 100 ){
		DaoProcess_RaiseError( proc, "Param", "Invalid percentage" );
		return;
	}
	if ( N == 1 ){
		count = arr->size/2 + 1;
		odd = arr->size%2? 1 : 0;
	}
	else {
		count = round( arr->size*( perc/100 ) );
		if ( !count ){
			DaoProcess_RaiseError( proc, "Param", "Percentage too low for the given quantity of data" );
			return;
		}
		odd = count%2? 1 : 0;
		if ( !odd ){
			if ( count < arr->size )
				count++;
			else {
				DaoProcess_RaiseError( proc, "Param", "Percentage too high for the given quantity of data" );
				return;
			}
		}
	}
	switch ( arr->etype ){
	case DAO_INTEGER:
		for ( i = 0; i < count; i++ ){
			daoint min = arr->data.i[i];
			daoint index = i;
			for ( j = i + 1; j < arr->size; j++ )
				if ( arr->data.i[j] < min ){
					min = arr->data.i[j];
					index = j;
				}
			arr->data.i[i] = arr->data.i[index];
			arr->data.i[index] = min;
		}
		DaoProcess_PutDouble( proc, odd? (double)arr->data.i[count - 1] : ( (double)arr->data.i[count - 1] + arr->data.i[count - 2] )/2 );
		break;
	case DAO_FLOAT:
		for ( i = 0; i < count; i++ ){
			float min = arr->data.f[i];
			daoint index = i;
			for ( j = i + 1; j < arr->size; j++ )
				if ( arr->data.f[j] < min ){
					min = arr->data.f[j];
					index = j;
				}
			arr->data.f[i] = arr->data.f[index];
			arr->data.f[index] = min;
		}
		DaoProcess_PutDouble( proc, odd? (double)arr->data.f[count - 1] : ( (double)arr->data.f[count - 1] + arr->data.f[count - 2] )/2 );
		break;
	case DAO_DOUBLE:
		for ( i = 0; i < count; i++ ){
			double min = arr->data.d[i];
			daoint index = i;
			for ( j = i + 1; j < arr->size; j++ )
				if ( arr->data.d[j] < min ){
					min = arr->data.d[j];
					index = j;
				}
			arr->data.d[i] = arr->data.d[index];
			arr->data.d[index] = min;
		}
		DaoProcess_PutDouble( proc, odd? arr->data.d[count - 1] : ( arr->data.d[count - 1] + arr->data.d[count - 2] )/2 );
		break;
	}
}

static void DaoStat_Mode( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr = &p[0]->xArray;
	daoint i, freq = 0;
	DMap *hash;
	DNode *node;
	char key[sizeof(void*)*2], value[sizeof(void*)*2];
	void *pkey = &key;
	daoint *pval = (daoint*)&value;
	memset( pkey, 0, sizeof(key) );
	memset( pval, 0, sizeof(value) );
	DaoArray_Sliced( arr );
	if ( !arr->size ){
		DaoProcess_RaiseError( proc, "Value", "Empty sample" );
		return;
	}
	hash = DHash_New( DAO_DATA_VOID2, DAO_DATA_VOID2 );
	for ( i = 0; i < arr->size; i++ ){
		switch ( arr->etype ){
		case DAO_INTEGER:	*(daoint*)pkey = arr->data.i[i]; break;
		case DAO_FLOAT:		*(float*)pkey = arr->data.f[i]; break;
		case DAO_DOUBLE:	*(double*)pkey = arr->data.d[i]; break;
		}
		node = DMap_Find( hash, pkey );
		if ( node )
			*pval = *(daoint*)node->value.pVoid + 1;
		else
			*pval = 1;
		DMap_Insert( hash, pkey, pval );
	}
	for ( node = DMap_First( hash ); node; node = DMap_Next( hash, node ) )
		if ( *(daoint*)node->value.pVoid > freq ){
			freq = *(daoint*)node->value.pVoid;
			memmove( pkey, node->key.pVoid, sizeof(key) );
		}
	switch ( arr->etype ){
	case DAO_INTEGER:	DaoProcess_PutInteger( proc, *(daoint*)pkey ); break;
	case DAO_FLOAT:		DaoProcess_PutFloat( proc, *(float*)pkey ); break;
	case DAO_DOUBLE:	DaoProcess_PutDouble( proc, *(double*)pkey ); break;
	}
	DMap_Delete( hash );
}

static void DaoStat_MinMax( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr = &p[0]->xArray;
	daoint size = DaoArray_GetWorkSize( arr );
	daoint start = DaoArray_GetWorkStart( arr );
	daoint len = DaoArray_GetWorkIntervalSize( arr );
	daoint step = DaoArray_GetWorkStep( arr );
	daoint i, j;
	DaoTuple *tup;
	arr = DaoArray_GetWorkArray( arr );
	if ( !size ){
		DaoProcess_RaiseError( proc, "Value", "Empty sample" );
		return;
	}
	tup = DaoProcess_PutTuple( proc, 2 );
	switch ( arr->etype ){
	case DAO_INTEGER:
		if ( 1 ){
			daoint min, max;
			min = max = arr->data.i[start];
			for ( i = 0; i < size; i++ ){
				j = start + (i / len) * step + (i % len);
				if ( arr->data.i[j] < min )
					min = arr->data.i[j];
				else if ( arr->data.i[j] > max )
					max = arr->data.i[j];
			}
			tup->values[0]->xInteger.value = min;
			tup->values[1]->xInteger.value = max;
		}
		break;
	case DAO_FLOAT:
		if ( 1 ){
			float min, max;
			min = max = arr->data.f[start];
			for ( i = 0; i < size; i++ ){
				j = start + (i / len) * step + (i % len);
				if ( arr->data.f[j] < min )
					min = arr->data.f[j];
				else if ( arr->data.f[j] > max )
					max = arr->data.f[j];
			}
			tup->values[0]->xFloat.value = min;
			tup->values[1]->xFloat.value = max;
		}
		break;
	case DAO_DOUBLE:
		if ( 1 ){
			double min, max;
			min = max = arr->data.d[start];
			for ( i = 0; i < arr->size; i++ ){
				j = start + (i / len) * step + (i % len);
				if ( arr->data.d[j] < min )
					min = arr->data.d[j];
				else if ( arr->data.d[j] > max )
					max = arr->data.d[j];
			}
			tup->values[0]->xDouble.value = min;
			tup->values[1]->xDouble.value = max;
		}
		break;
	}
}

static void DaoStat_Distribution( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr = &p[0]->xArray;
	daoint size = DaoArray_GetWorkSize( arr );
	daoint start = DaoArray_GetWorkStart( arr );
	daoint len = DaoArray_GetWorkIntervalSize( arr );
	daoint step = DaoArray_GetWorkStep( arr );
	daoint i;
	DaoMap *hash = DaoProcess_PutMap( proc, 1 );
	arr = DaoArray_GetWorkArray( arr );
	if ( !size ){
		DaoProcess_RaiseError( proc, "Value", "Empty sample" );
		return;
	}
	switch ( arr->etype ){
	case DAO_INTEGER:
		for ( i = 0; i < size; i++ ){
			daoint j = start + (i / len) * step + (i % len);
			DaoInteger key = {DAO_INTEGER, 0, 0, 0, 0, arr->data.i[j]};
			DaoInteger val = {DAO_INTEGER, 0, 0, 0, 0, 1};
			DaoValue *value = DaoMap_GetValue( hash, (DaoValue*)&key );
			if ( value )
				val.value = value->xInteger.value + 1;
			DaoMap_Insert( hash, (DaoValue*)&key, (DaoValue*)&val );
		}
		break;
	case DAO_FLOAT:
		for ( i = 0; i < size; i++ ){
			daoint j = start + (i / len) * step + (i % len);
			DaoFloat key = {DAO_FLOAT, 0, 0, 0, 0, arr->data.f[j]};
			DaoInteger val = {DAO_INTEGER, 0, 0, 0, 0, 1};
			DaoValue *value = DaoMap_GetValue( hash, (DaoValue*)&key );
			if ( value )
				val.value = value->xFloat.value + 1;
			DaoMap_Insert( hash, (DaoValue*)&key, (DaoValue*)&val );
		}
		break;
	case DAO_DOUBLE:
		for ( i = 0; i < size; i++ ){
			daoint j = start + (i / len) * step + (i % len);
			DaoDouble key = {DAO_DOUBLE, 0, 0, 0, 0, arr->data.d[j]};
			DaoInteger val = {DAO_INTEGER, 0, 0, 0, 0, 1};
			DaoValue *value = DaoMap_GetValue( hash, (DaoValue*)&key );
			if ( value )
				val.value = value->xInteger.value + 1;
			DaoMap_Insert( hash, (DaoValue*)&key, (DaoValue*)&val );
		}
		break;
	}
}

static void DaoStat_DistribGroup( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr = &p[0]->xArray;
	daoint size = DaoArray_GetWorkSize( arr );
	daoint start = DaoArray_GetWorkStart( arr );
	daoint len = DaoArray_GetWorkIntervalSize( arr );
	daoint step = DaoArray_GetWorkStep( arr );
	daoint i;
	double stval = p[2]->xDouble.value, interval = p[1]->xDouble.value;
	DaoMap *hash = DaoProcess_PutMap( proc, 1 );
	arr = DaoArray_GetWorkArray( arr );
	if ( !size ){
		DaoProcess_RaiseError( proc, "Value", "Empty sample" );
		return;
	}
	if ( interval == 0 ){
		DaoProcess_RaiseError( proc, "Param", "Zero interval" );
		return;
	}
	for ( i = 0; i < size; i++ ){
		daoint j = start + (i / len) * step + (i % len);
		DaoInteger key = {DAO_INTEGER, 0, 0, 0, 0, 0};
		DaoInteger val = {DAO_INTEGER, 0, 0, 0, 0, 1};
		DaoValue *value;
		double item;
		switch ( arr->etype ){
		case DAO_INTEGER:	item = arr->data.i[j]; break;
		case DAO_FLOAT:		item = arr->data.f[j]; break;
		case DAO_DOUBLE:	item = arr->data.d[j]; break;
		}
		if ( item >= stval ){
			key.value = ( item - stval )/interval;
			value = DaoMap_GetValue( hash, (DaoValue*)&key );
			if ( value )
				val.value = value->xInteger.value + 1;
			DaoMap_Insert( hash, (DaoValue*)&key, (DaoValue*)&val );
		}
	}
}

static void DaoStat_Correlation( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr1 = &p[0]->xArray;
	DaoArray *arr2 = &p[1]->xArray;
	int pearson = p[2]->xEnum.value == 0;
	if ( !DaoArray_GetWorkSize( arr1 ) || !DaoArray_GetWorkSize( arr2 ) ){
		DaoProcess_RaiseError( proc, "Value", "Empty sample" );
		return;
	}
	if ( DaoArray_GetWorkSize( arr1 ) != DaoArray_GetWorkSize( arr2 ) ){
		DaoProcess_RaiseError( proc, "Value", "Calculating correlation on samples of different size" );
		return;
	}
	if ( pearson ){
		double mean1, mean2;
		if ( N == 5 ){
			mean1 = p[3]->xDouble.value;
			mean2 = p[4]->xDouble.value;
		}
		else {
			mean1 = Mean( arr1 );
			mean2 = Mean( arr2 );
		}
		DaoProcess_PutDouble( proc, CorrelationPearson( arr1, arr2, mean1, mean2 ));
	}
	else
		DaoProcess_PutDouble( proc, CorrelationSpearman( arr1, arr2 ));
}

static void DaoStat_Kurtosis( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr = &p[0]->xArray;
	double mean;
	if ( !DaoArray_GetWorkSize( arr ) ){
		DaoProcess_RaiseError( proc, "Value", "Empty sample" );
		return;
	}
	if ( N == 2 )
		mean = p[1]->xDouble.value;
	else
		mean = Mean( arr );
	DaoProcess_PutDouble( proc, Moment( arr, 1, mean ) );
}

static void DaoStat_Skewness( DaoProcess *proc, DaoValue *p[], int N )
{
	DaoArray *arr = &p[0]->xArray;
	double mean;
	if ( !DaoArray_GetWorkSize( arr ) ){
		DaoProcess_RaiseError( proc, "Value", "Empty sample" );
		return;
	}
	if ( N == 2 )
		mean = p[1]->xDouble.value;
	else
		mean = Mean( arr );
	DaoProcess_PutDouble( proc, Moment( arr, 0, mean ) );
}

static DaoFuncItem statMeths[] =
{
	/*! Returns arithmetic mean of \a data.
	 * E[X] = Σx / N */
	{ DaoStat_Mean,			"mean(invar data: array<@T<int|float|double>>) => double" },

	/*! Returns variance of \a data (measure of spread) of the given \a kind. Uses \a mean if it is given
	 * Sample:		σ²[X] = Σ(x - E[X]) / (N - 1)
	 * Population:	σ²[X] = Σ(x - E[X]) / N */
	{ DaoStat_Variance,		"variance(invar data: array<@T<int|float|double>>, kind: enum<sample,population> = $sample) => double" },
	{ DaoStat_Variance,		"variance(invar data: array<@T<int|float|double>>, mean: double, kind: enum<sample,population> = $sample) => double" },

	/*! Returns median (middle value) of \a data while partially sorting it. If \a data size is even, the mean of two middle values is returned */
	{ DaoStat_Percentile,	"median(data: array<@T<int|float|double>>) => double" },

	/*! Returns percentile \a percentage (the value below which the given percentage of sample values fall) of \a data while partially sorting it.
	 * \a percentage must be in range (0; 100) */
	{ DaoStat_Percentile,	"percentile(data: array<@T<int|float|double>>, percentage: double) => double" },

	/*! Returns mode (most common value) of \a data */
	{ DaoStat_Mode,			"mode(invar data: array<@T<int|float|double>>) => @T" },

	/*! Returns minimum and maximum value in \a data */
	{ DaoStat_MinMax,		"range(invar data: array<@T<int|float|double>>) => tuple<min: @T, max: @T>" },

	/*! Returns distribution of values in \a data in the form \c value => \c frequency, where \c value is a single unique value and \c frequency
	 * is the number of its appearances in \a data */
	{ DaoStat_Distribution,	"distribution(invar data: array<@T<int|float|double>>) => map<@T,int>" },

	/*! Returns values of \a data grouped into ranges of width \a interval starting from \a start. The result is in the form \c index => \c frequency
	 * corresponding to the ranges present in the sample. \c index identifies the range, it is equal to integer number of intervals
	 * \a interval between \a start and the beginning of the particular range; the exact range boundaries are
	 * [\a start + \c floor(\c index / \a interval); \a start + \c floor(\c index / \a interval) + \a interval).
	 * \c frequency is the number of values which fall in the range. The values lesser then \a start are not included in the resulting statistics */
	{ DaoStat_DistribGroup,	"distribution(invar data: array<@T<int|float|double>>, interval: double, start = 0.0) => map<int,int>" },

	/*! Returns correlation \a coefficient between \a data1 and \a data2. Pearson coefficient measures linear dependence,
	 * Spearman's rank coefficient measures monotonic dependence. If \a mean1 and \a mean2 are given, they are used for calculating Pearson
	 * coefficient.
	 * \note \a self and \a other must be of equal size
	 * Pearson:			r[X,Y] = E[(X - E[X])(Y - E[Y])] / σ[X]σ[Y]
	 * Spearman's rank:	ρ[X,Y] = r(Xrank, Yrank) */
	{ DaoStat_Correlation,	"correlation(invar data1: array<@T<int|float|double>>, "
										"invar data2: array<@T>, coefficient: enum<pearson,spearman>) => double" },
	{ DaoStat_Correlation,	"correlation(invar data1: array<@T<int|float|double>>, "
										"invar data2: array<@T>, coefficient: enum<pearson,spearman>, mean1: double, mean2: double) => double" },

	/*! Returns skewness (measure of asymmetry) of \a data. Uses \a mean if it is given.
	 * γ1[X] = E[((x - E[X]) / σ)^3] */
	{ DaoStat_Skewness,		"skewness(invar data: array<@T<int|float|double>>) => double" },
	{ DaoStat_Skewness,		"skewness(invar data: array<@T<int|float|double>>, mean: double) => double" },

	/*! Returns kurtosis (measure of "peakedness"). Uses \a mean if it is given
	 * γ2[X] = E[((x - E[X]) / σ)^4] - 3 */
	{ DaoStat_Kurtosis,		"kurtosis(invar data: array<@T<int|float|double>>) => double" },
	{ DaoStat_Kurtosis,		"kurtosis(invar data: array<@T<int|float|double>>, mean: double) => double" },
	{ NULL, NULL }
};

DAO_DLL int DaoStatistics_OnLoad( DaoVmSpace *vmSpace, DaoNamespace *ns )
{
	DaoNamespace *statns;
	statns = DaoVmSpace_GetNamespace( vmSpace, "stat" );
	DaoNamespace_AddConstValue( ns, "stat", (DaoValue*)statns );
	DaoNamespace_WrapFunctions( statns, statMeths );
	return 0;
}
