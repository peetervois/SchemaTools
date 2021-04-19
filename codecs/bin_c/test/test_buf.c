

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
        test( tausch_decode_to_end(&iter),"tausch_decode_to_eoscope must return true on EOF.");
        test( ! tausch_iter_is_complete(&iter), "the iter shall not be complete. ");
        test( tausch_buf_is_eof( iter.idx ), "the item shall point to EOF ");
        test( tausch_decode_to_end(&iter),"tausch_decode_to_eoscope must return true on EOF.");
        test( ! tausch_iter_is_complete(&iter), "the iter shall not be complete. ");
        test( tausch_buf_is_eof( iter.idx ), "the item shall point to EOF ");
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
        test( tausch_decode_to_end(&iter),"tausch_decode_to_eoscope must return true on EOF.");
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
        printf(" --- Iterating the buffer one by one at the same scope (root scope). \n");
        test( tausch_decode_to_next(&iter), "Iterating must have passed" );
        test( iter.tag == 2, "incorrect tag was read out" );
        test( tausch_decode_to_next(&iter), "Iterating must have passed" );
        test( iter.tag == 0x11111, "incorrect tag was read out" );
        test( tausch_decode_to_next(&iter), "Iterating must have passed" );
        test( iter.tag == 0, "incorrect tag was read out" );
        test( iter.vlen == 3, "incorrect vlen was read out" );
        test( tausch_decode_to_next(&iter), "Iterating must have passed" );
        test( iter.tag == 0, "incorrect tag was read out" );
        test( iter.vlen == 0, "incorrect vlen was read out" );
        test( ! tausch_decode_to_next(&iter), "Iterating must have failed" );
        test( iter.tag == ~0, "incorrect tag was read out" );
        test( ! tausch_decode_to_next(&iter), "Iterating must have passed" );
        test( iter.tag == ~0, "incorrect tag was read out" );
        test( tausch_buf_is_eof( iter.idx ), "iter must point to EOF.");
    }

    {
        tausch_iter_t iter = iterini;
        printf(" --- Iterating the buffer to the item at the same scope (root scope). \n");
        test( tausch_decode_to_tag(&iter, 2), "Iterating must have passed" );
        test( iter.tag == 2, "incorrect tag was read out" );
        iter = iterini;
        test( tausch_decode_to_tag(&iter, 0x11111), "Iterating must have passed" );
        test( iter.tag == 0x11111, "incorrect tag was read out" );
        iter = iterini;
        test( tausch_decode_to_tag(&iter, 0), "Iterating must have passed" );
        test( iter.tag == 0, "incorrect tag was read out" );
        test( iter.vlen == 3, "incorrect vlen was read out" );
        test( tausch_decode_to_tag(&iter, 0), "Iterating must have passed" );
        test( iter.tag == 0, "incorrect tag was read out" );
        test( iter.vlen == 0, "incorrect vlen was read out" );
        iter = iterini;
        test( ! tausch_decode_to_tag(&iter,33), "Iterating must have failed" );
        test( iter.tag == ~0, "incorrect tag was read out" );
        test( ! tausch_decode_to_tag(&iter,2), "Iterating must have passed" );
        test( iter.tag == ~0, "incorrect tag was read out" );
        test( tausch_buf_is_eof( iter.idx ), "iter must point to EOF.");

        printf(" --- Iterating the buffer to the stuffing but EOF will be found. \n");
        test( tausch_decode_to_stuffing(&iter), "EOF must result true when decoding to stuffing");
        test( tausch_buf_is_eof( iter.idx ), "iter must point to EOF.");
    }

    {
        tausch_iter_t iter = iterini;

        printf(" --- Testing writing of the multibyte boolean. \n");
        test( tausch_decode_to_end(&iter),"tausch_decode_to_eoscope must return true on EOF.");
        bool val = false;
        test( tausch_write( &iter, 22, &val ), "writing bool failed" );
        test( (iter.next - iter.idx) == 3, "must have been writing 3 bytes" );
        test( tausch_buf_is_eof( iter.next ), "EOF must have been pushed.");
        test( tausch_read( &iter, &val ), "reading bool failed" );
        test( val == false, "the readout is not false" );
        val = true;
        test( tausch_write( &iter, 22, &val ), "writing false over true must have been passing" );
        test( tausch_read( &iter, &val ), "reading bool failed" );
        test( val == true, "the readout is not true" );
    }

    {
        tausch_iter_t iter = iterini;

        printf(" --- Testing writing of typX. \n");
        test( tausch_decode_to_end(&iter),"tausch_decode_to_eoscope must return true on EOF.");

        uint16_t val = 27;
        test( tausch_write( &iter, 35, &val), "writing UINT-16 failed" );
        test( (iter.next - iter.idx) == 5, "must have been writing 5 bytes" );
        test( tausch_buf_is_eof( iter.next ), "EOF must have been pushed.");
        test( tausch_iter_vlen( &iter) == 2, "the tausch_iter_vlen result is wrong" );
        test( tausch_read( &iter, &val ), "reading UINT-16 failed");
        test( val == 27, "the read out value is wrong" );
        val = 28;
        test( !tausch_write( &iter,36, &val ), "writing over with another tag must fail");
        test( tausch_read( &iter, &val ), "reading UINT-16 failed");
        test( val == 27, "the read out value is wrong" );
        uint32_t val32 = 29;
        test( !tausch_write( &iter,35, &val32 ), "writing over with another value length must fail");
        test( tausch_read( &iter, &val ), "reading UINT-16 failed");
        test( val == 27, "the read out value is wrong" );
        uint8_t val8 = 30;
        test( !tausch_write( &iter,35, &val8 ), "writing over with another value length must fail");
        test( tausch_read( &iter, &val ), "reading UINT-16 failed");
        test( val == 27, "the read out value is wrong" );

        test( !tausch_decode_next( &iter), "decode next must have been failed because of EOF" );
        test( tausch_write_typX( &iter,40,NULL,9 ) == 9, "writing 9 filled bytes did not succeed quite" );

        uint8_t blob_buf_1[20];
        tausch_blob_t blob_1 = { .buf = blob_buf_1, .len = sizeof(blob_buf_1) };
        test( tausch_read( &iter, &blob_1) == 9, "reading 9 filled bytes did not succeed quite" );
        test( ! tausch_write( &iter, 40, &blob_1), "overwriting blob with bigger data must fail" );
        test( tausch_write_next( &iter ), "preparing to write next failed" );
        test( tausch_write( &iter, 40, &blob_1), "writing blob failed");
        test( tausch_write_next( &iter ), "preparing to write next failed" );
        test( tausch_write( &iter, 40, (tausch_blob_t*)NULL ), "writing null blob failed");

        printf(" --- Testing of writing UTF8 \n" );
        test( tausch_write_next( &iter ), "preparing to write next failed" );
        test( tausch_write( &iter, 41, "The utf8 äöüš"), "writing of utf8 failed" );
        iter = iterini;
        test( tausch_decode_to_tag( &iter, 40), "finding blob first item failed" );
        test( tausch_decode_to_tag( &iter, 40), "finding blob second item failed" );
        test( tausch_iter_vlen( &iter ) == 20, "the length of the item is wrong" );
        test( tausch_write( &iter, 40, "äöüš second"), "overwriting blobw with utf8 failed" );
        test( tausch_write_typX( &iter, 40, NULL, iter.vlen), "overwriting with 0x00 failed" );
        test( tausch_write( &iter, 40, NULL), "turning the item to stuffing" );

    }

    {
        tausch_iter_t iter = iterini;

        printf(" --- Testing writing of typX null. \n");
        test( tausch_decode_to_end(&iter),"tausch_decode_to_eoscope must return true on EOF.");
        test( tausch_write( &iter, 32, NULL), "writing null failed" );
        test( (iter.next - iter.idx) == 2, "must have been writing 5 bytes" );
        test( tausch_iter_is_null( &iter), "iter must be as null" );
        test( tausch_iter_vlen(&iter) == 0, "iter value len must be 0" );
        test( ! tausch_decode_next( &iter), "iterating to EOC must fail" );
        test( tausch_iter_vlen(&iter) == 0, "iter at EOF vlen must be 0" );

    }

    {
        tausch_iter_t iter = iterini;
        uint32_t val = 10;

        printf(" --- Testing writing of scoped data. \n");
        test( tausch_decode_to_end(&iter),"tausch_decode_to_eoscope must return true on EOF.");
        test( tausch_write_scope(&iter, 100), "writing of scope failed" );
        test( tausch_write_next(&iter), "advancing to next failed" );
        test( tausch_write(&iter, 1, &val), "writing value failed" );
        test( tausch_write_next(&iter), "advancing to next failed" );
        test( tausch_write_end(&iter), "writing end failed" );
        test( tausch_write_next(&iter), "advancing to next failed" );
        test( tausch_write(&iter, 101, &val), "writing value failed" );

        iter = iterini;
        test( tausch_decode_to_tag( &iter, 100), "finding tag 100 failed" );
        test( tausch_decode_to_tag( &iter, 1), "finding tag 100/1 failed" );
        test( tausch_decode_to_end( &iter ), "finding end failed" );
        test( iter.idx[0] != 0x03, "iter must not be at the END tag" );
        test( tausch_write_next(&iter), "advancing to next failed" );
        test( tausch_write( &iter, 2, NULL), "writing null tag failed" );
        test( tausch_write_next(&iter), "advancing to next failed" );
        test( ! tausch_write_scope(&iter, 3), "writing of scope at the end of buffer must fail" );

        iter = iterini;
        test( tausch_decode_to_tag( &iter, 100), "finding tag 100 failed" );
        test( ! tausch_decode_to_tag( &iter, 2), "finding tag 100/2 must fail" );

        iter = iterini;
        test( tausch_decode_to_tag( &iter, 100), "finding tag 100 failed" );
        test( ! tausch_decode_to_stuffing( &iter), "finding stuffing must fail" );
        test( tausch_decode_to_end(&iter), "decoding to eoscope failed" );
        test( tausch_decode_vluint(&iter) == 7, "in test the EOF must have been found" );
        test( tausch_decode_vluint(&iter) == ~0, "over the edge test failed" );
    }

    {
        printf(" --- Testing over the edge decoding. \n");
        uint8_t buf[] = {0b1010,20,0,0,0,};
        tausch_iter_t iter = TAUSCH_ITER_INIT( buf, sizeof(buf) );
        test( ! tausch_decode_next(&iter), "over the edge decode next failed" );

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

