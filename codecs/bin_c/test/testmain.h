

#include "../tauschema_codec.h"
#include "stdio.h"

extern size_t count_tests;
extern size_t count_errors;

void test( bool cond, char *msg );
bool bincompare( uint8_t *b1, uint8_t *b2, size_t len );

#define BINCOMP( b1, sb2, emsg )\
    do{ uint8_t *b2 = sb2; test( bincompare(b1,b2,sizeof(sb2)),emsg); }while(0)

bool test_buf( void );
