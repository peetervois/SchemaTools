

#include "testmain.h"

size_t count_tests = 0;
size_t count_errors = 0;

int main( void )
{
    test_buf();
    test_flater();

    printf("\n\n");
    printf("Number of tests performed: %ld \n", count_tests );
    printf("   Number of tests failed: %ld \n", count_errors );
    printf("\n\n");
    return 0;
}

char details[20] = { 0 };

bool bincompare( uint8_t *b1, uint8_t *b2, size_t len )
{
    for( size_t i=0; i<len; i++ )
    {
        if( b1[i] != b2[i] )
        {
            snprintf( details, 20, " [%ld]", i );
            return false;
        }
    }
    return true;
}

bool hexcompare( uint8_t *b1, char *b2 )
{
    size_t i = 0;
    while( *b2 != 0  )
    {
        unsigned long db2;
        char *end = 0;
        db2 = strtoul( b2, &end, 16 );
        if( (*end != 0) && (b2 == end) )
        {
            b2++;
            continue;
        }
        b2 = end;
        if( b1[i] != (db2 & 0xFF) )
        {
            snprintf( details, 20, " [%ld]", i );
            return false;
        }
        i++;
    }
    return true;
}


void test( bool cond, char *msg )
{
    count_tests += 1;
    if( !cond )
    {
        count_errors += 1;
        printf( "error: %s %s\n", msg, details );
    }
    details[0] = 0;
}

void printhex( char *prep, uint8_t *start, uint8_t *end )
{
    printf( "%s ", prep );

    while( start < end )
    {
        printf("%02hhx,", *(start++) );
    }
    printf("\n");
}
