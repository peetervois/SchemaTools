

#include "testmain.h"

size_t count_tests = 0;
size_t count_errors = 0;

int main( void )
{
    test_buf();

    printf("\n\n");
    printf("Number of tests performed: %ld \n", count_tests );
    printf("   Number of tests failed: %ld \n", count_errors );
    printf("\n\n");
    return 0;
}

bool bincompare( uint8_t *b1, uint8_t *b2, size_t len )
{
    for( size_t i=0; i<len; i++ )
    {
        if( b1[i] != b2[i] ) return false;
    }
    return true;
}

void test( bool cond, char *msg )
{
    count_tests += 1;
    if( cond ) return;
    count_errors += 1;
    printf( "error: %s\n", msg );
}
