
#include"daoType.h"

float test( int p )
{
	printf( "test() is called: %i\n", p );
	return p + 0.55;
}
char* test2( float p )
{
	printf( "test2() is called: %f\n", p );
	return "string from c";
}
float test_string( char *s )
{
	printf( "test_string() is called: %s\n", s );
	return 0.55;
}
void test_array_short( short *array )
{
	printf( "array: %i %i\n", array[0], array[1] );
	array[0] *= 10;
	array[1] *= 100;
}
void test_array_float( float *array )
{
	printf( "array: %f %f\n", array[0], array[1] );
}
void test_array_double( double *array )
{
	printf( "array: %f %f\n", array[0], array[1] );
}
void test_dao_list( DaoList *list )
{
	printf( "list size = %i\n", (int) list->items.size );
}
void test_stream( FILE *fout )
{
	if( fout == NULL ) fout = stdout;
	fprintf( fout, "test_stream: %p\n", fout );
}

void cblas_dgemv(int order, int trans, int m, int n, double alpha, double  *a,
		int lda, double *x, int incx, double beta, double *y, int incy)
{
	int i;
	printf( "%i %i %i %i %f\n", order, trans, m, n, alpha );
	printf( "%p\n", a );
	for(i=0; i<m*n; ++i) printf( "%f\n", a[i] );
	for(i=0; i<n; ++i) printf( "%f\n", y[i] );
}
