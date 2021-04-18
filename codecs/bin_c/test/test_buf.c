

#include "testmain.h"

static uint8_t testbuf_1[100];

bool test_buf( void )
{
    tausch_iter_t iter = TAUSCH_ITER_INIT( testbuf_1, sizeof(testbuf_1) );

    {
        printf(" --- Testing of writing u32 with 1 byte tag and 1 byte length. \n");
        uint32_t valu32 = 0x12345678;
        uint8_t expect[] = { 0x0a, 0x04, 0x78, 0x56, 0x34, 0x12 };

        size_t rv = tausch_write( &iter, 2, &valu32 );
        test( rv == 4, " 4 bytes must have been written " );

        BINCOMP( testbuf_1, expect, "written binary is not right" );
    }

    {
        uint32_t valu32 = 0x12345678;
        uint8_t expect[] = { 0b11000110, 0b10001000, 0b00010001, 0x04, 0x78, 0x56, 0x34, 0x12 };

        printf(" --- Testing of advancing the iterator to write next value\n");
        test( tausch_write_next( &iter ), "write_next failed" );

        printf(" --- Testing of writin 3byte length of tag \n");
        size_t rv = tausch_write( &iter, 0x11111, &valu32 );
        test( rv == 4, " 4 bytes must have been written " );
        BINCOMP( testbuf_1 +6, expect, "written binary is not right" );

        printf(" --- Testing of value overwrite \n");
        valu32 = 0x87654321;
        uint8_t expect_1[] = { 0b11000110, 0b10001000, 0b00010001, 0x04, 0x21, 0x43, 0x65, 0x87 };
        rv = tausch_write( &iter, 0x11111, &valu32 );
        test( rv == 4, " 4 bytes must have been written " );
        BINCOMP( testbuf_1 +6, expect_1, "written binary is not right" );

        printf(" --- Testing nullification of the value\n");
        uint8_t expect_2[] = { 0b11000100, 0b10001000, 0b00010001, 0x02, 0x03, 0x00, 0x00, 0x00 };
        rv = tausch_write( &iter, 0x11111, NULL );
        test( rv == 1, " so called 1 bytes must have been written " );
        BINCOMP( testbuf_1 +6, expect_2, "written binary is not right" );

        printf(" --- testing of advancing the iterator to next, this time newly created stuffing. \n" );
        test( tausch_decode_next( &iter ), "decode_next failed" );
        uint8_t expect_3[] = { 0x02, 0x03, 0x00, 0x00, 0x00 };
        BINCOMP( testbuf_1 +6+3, expect_3, "written binary is not right" );
    }


    return true;
}

