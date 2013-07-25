
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
void test_dao_list( DaoList *list )
{
	printf( "list size = %i\n", (int) list->items.size );
}
