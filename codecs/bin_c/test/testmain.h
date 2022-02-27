

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

extern size_t count_tests;
extern size_t count_errors;

void test( bool cond, char *msg );
bool bincompare( uint8_t *b1, uint8_t *b2, size_t len );
bool hexcompare( uint8_t *b1, char *b2 );

#define BINCOMP( b1, sb2, emsg )\
    do{ uint8_t *b2 = sb2; test( bincompare(b1,b2,sizeof(sb2)),emsg); }while(0)

#define STRCOMP( b1, sb2, emsg )\
    do{ uint8_t *b2 = (uint8_t*)sb2; test( bincompare(b1,b2,strlen(b2)),emsg); }while(0)

#define HEXCOMP( b1, sb2, emsg )\
    do{ test( hexcompare(b1,sb2),emsg); }while(0)

bool test_buf( void );

void printhex( char *prep, uint8_t *start, uint8_t *end );
