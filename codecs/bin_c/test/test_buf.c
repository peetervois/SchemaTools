

#include "testmain.h"

static uint8_t testbuf_1[100];

bool test_buf( void )
{
    tausch_iter_t iterini = TAUSCH_ITER_INIT( testbuf_1, sizeof(testbuf_1) );

    tausch_format_buf( testbuf_1 );

    {
        tausch_iter_t iter = iterini;

        printf(" --- Testing of writing u32 with 1 byte tag and 1 byte length. \n");
        uint32_t valu32 = 0x12345678;
        uint8_t expect[] = { 0x0a, 0x04, 0x78, 0x56, 0x34, 0x12 };

        size_t rv = tausch_write( &iter, 2, &valu32 );
        test( rv == 4, " 4 bytes must have been written " );

        BINCOMP( testbuf_1, expect, "written binary is not right" );

        valu32 = 0x12345678;
        uint8_t expect_0[] = { 0b11000110, 0b10001000, 0b00010001, 0x04, 0x78, 0x56, 0x34, 0x12 };

        printf(" --- Testing of advancing the iterator to write next value\n");
        test( tausch_write_next( &iter ), "write_next failed" );

        printf(" --- Testing of writin 3byte length of tag \n");
        rv = tausch_write( &iter, 0x11111, &valu32 );
        test( rv == 4, " 4 bytes must have been written " );
        BINCOMP( testbuf_1 +6, expect_0, "written binary is not right" );

        printf(" --- Testing of value overwrite \n");
        valu32 = 0x87654321;
        uint8_t expect_1[] = { 0b11000110, 0b10001000, 0b00010001, 0x04, 0x21, 0x43, 0x65, 0x87 };
        rv = tausch_write( &iter, 0x11111, &valu32 );
        test( rv == 4, " 4 bytes must have been written " );
        BINCOMP( testbuf_1 +6, expect_1, "written binary is not right" );
        test( ! tausch_iter_is_null( &iter ), "is_null must return false when something is not nullified ");


        printf(" --- Testing nullification of the value\n");
        uint8_t expect_2[] = { 0b11000100, 0b10001000, 0b00010001, 0x02, 0x03, 0x00, 0x00, 0x00 };
        rv = tausch_write( &iter, 0x11111, NULL );
        test( rv == 1, " so called 1 bytes must have been written " );
        BINCOMP( testbuf_1 +6, expect_2, "written binary is not right" );

        printf(" --- Testing the is_null method \n");
        test( tausch_iter_is_null( &iter ), "is_null must return true when something is nullified ");

        printf(" --- testing of advancing the iterator to next, this time newly created stuffing. \n" );
        test( tausch_decode_next( &iter ), "decode_next failed" );
        uint8_t expect_3[] = { 0x02, 0x03, 0x00, 0x00, 0x00 };
        BINCOMP( testbuf_1 +6+3, expect_3, "written binary is not right" );
        test( 5 == tausch_iter_is_stuffing( &iter ), "tausch_iter_is_stuffing: the amount of stuffing is wrong.");

    }

    {
        tausch_iter_t iter = iterini;

        printf(" --- Testing of advancing into EOF. \n");
        test( tausch_decode_to_eoscope(&iter),"tausch_decode_to_eoscope must return true on EOF.");
    }


    {
        tausch_iter_t iter;
        iter.ebuf = NULL;

        printf(" --- Testing start over and runtime initiating the iter \n");
        tausch_iter_init( &iter, testbuf_1, sizeof(testbuf_1) );
        test( tausch_iter_is_ok(&iter), "Initiation of iter went wrong");

        printf(" --- Decode to stuffing \n");
        test( tausch_decode_to_stuffing(&iter), "stuffing was not found but it is there");
        uint8_t expect_3[] = { 0x02, 0x03, 0x00, 0x00, 0x00 };
        BINCOMP( testbuf_1 +6+3, expect_3, "it is not the stuffing we must have been found." );
        test( 5 == tausch_iter_is_stuffing( &iter ), "tausch_iter_is_stuffing: the amount of stuffing is wrong.");

        printf(" --- Testing of decoding to end of buffer \n" );
        test( ! tausch_decode_to_tag(&iter, 333), "the tag 333 was found but we shall not have it. ");

    }

    {
        tausch_iter_t iter = iterini;

        printf(" --- Testing writing of the boolean. \n");
        test( tausch_decode_to_eoscope(&iter),"tausch_decode_to_eoscope must return true on EOF.");
        bool val = false;
        test( tausch_write( &iter, 22, (bool*)NULL ), "writing bool failed" );
        test( (iter.next - iter.idx) == 1, "must have been writing only one byte" );
        test( tausch_buf_is_eof( iter.next ), "EOF must have been pushed.");
        test( tausch_read( &iter, &val ), "reading bool failed" );
        test( val == true, "the readout is not true" );
        test( tausch_write( &iter, 22, &val ), "writing true over true must have been passing in tag only mode" );
        val = false;
        test( tausch_write( &iter, 22, &val ), "writing false over true must have been passing in tag only mode" );
        test( tausch_read( &iter, &val ), "reading stuffing as bool must have been passing" );
        test( val == false, "the readout is not false" );


    }

    {
        tausch_iter_t iter = iterini;
        iterini = iter; // to avoid compiler warning

#if 0 // Testing of linking failure
        char mem[10];
        tausch_read( &iter, mem );
#endif

    }

    return true;
}

