

#include "testmain.h"
#include "../src/tauschema_codec.h"


static uint8_t testbuf_1[100];



bool test_buf( void )
{
    tausch_iter_t iterini = TAUSCH_ITER_INIT( testbuf_1, sizeof(testbuf_1) );
    char errorbuf[500];   // temporary error message

    tausch_format_buf( testbuf_1 );

    {
        tausch_iter_t iter = iterini;

        printf(" --- Testing of writing u32 with 1 byte tag and 1 byte length. \n");
        uint32_t valu32 = 0x12345678;
        uint8_t expect[] = { 0x0a, 0x04, 0x78, 0x56, 0x34, 0x12 };

        tsch_size_t rv = 0;
        test( !tausch_iter_next( &iter ), LINE("going to eof must return false") );
        rv = tausch_iter_write( &iter, 2, &valu32 );
        test( rv == 4, LINE(" 4 bytes must have been written ") );

        BINCOMP( testbuf_1, expect, LINE("written binary is not right") );

        valu32 = 0x12345678;
        uint8_t expect_0[] = { 0b11000110, 0b10001000, 0b00010001, 0x04, 0x78, 0x56, 0x34, 0x12 };

        printf(" --- Testing of advancing the iterator to write next value\n");
        test( !tausch_iter_next( &iter ), LINE("going to eof must return false") );

        printf(" --- Testing of writin 3byte length of tag \n");
        rv = tausch_iter_write( &iter, 0x11111, &valu32 );
        test( rv == 4, LINE(" 4 bytes must have been written ") );
        BINCOMP( testbuf_1 +6, expect_0, LINE("written binary is not right") );

        printf(" --- Testing of value overwrite \n");
        valu32 = 0x87654321;
        uint8_t expect_1[] = { 0b11000110, 0b10001000, 0b00010001, 0x04, 0x21, 0x43, 0x65, 0x87 };
        rv = tausch_iter_write( &iter, 0x11111, &valu32 );
        test( rv == 4, LINE(" 4 bytes must have been written ") );
        BINCOMP( testbuf_1 +6, expect_1, LINE("written binary is not right") );
        test( ! tausch_iter_is_null( &iter ), LINE("is_null must return false when something is not nullified ") );


        printf(" --- Testing nullification of the value\n");
        uint8_t expect_2[] = { 0b11000100, 0b10001000, 0b00010001, 0x07 };
        rv = tausch_iter_write( &iter, 0x11111, (bool*)NULL );
        test( rv == 1, LINE(" so called 1 bytes must have been written ") );
        BINCOMP( testbuf_1 +6, expect_2, LINE("written binary is not right") );

        printf(" --- Testing the is_null method \n");
        test( tausch_iter_is_null( &iter ), LINE("is_null must return true when something is nullified ") );
        tausch_iter_t tm = iter;
        test( !tausch_iter_next(&tm), LINE("going to eof must return false") );
        test( tausch_iter_write_stuffing(&tm, 5), LINE("writing stuffing to the end of the message failed") );
        test( tm.vlen == 3, LINE("stuffing vlen is wrong") );

        printf(" --- testing of advancing the iterator to next, this time newly created stuffing. \n" );
        test( tausch_iter_next( &iter ), LINE("going to stuffing must return true") );
        uint8_t expect_3[] = { 0x02, 0x03, 0x00, 0x00, 0x00 };
        BINCOMP( testbuf_1 +6+3, expect_3, LINE("written binary is not right") );
        test( 5 == tausch_iter_is_stuffing( &iter ), LINE("tausch_iter_is_stuffing: the amount of stuffing is wrong.") );
    }

    {
        tausch_iter_t iter = iterini;

        printf(" --- Testing of advancing into EOF. \n");
        test( tausch_iter_exit_scope(&iter), LINE("tausch_decode_to_eoscope must return true on EOF.") );
        test( tausch_iter_is_complete(&iter), LINE("the iter shall be complete on top of EOF. ") );
        test( tausch_iter_is_eof( &iter ), LINE("the item shall point to EOF ") );
        test( tausch_iter_exit_scope(&iter), LINE("tausch_decode_to_eoscope must return true on EOF.") );
        test( tausch_iter_is_complete(&iter), LINE("the iter shall be complete on top of EOF. ") );
        test( tausch_iter_is_eof( &iter ), LINE("the item shall point to EOF ") );
    }


    {
        tausch_iter_t iter;
        iter.ebuf = 0;

        printf(" --- Testing start over and runtime initiating the iter \n");
        tausch_iter_init( &iter, testbuf_1, sizeof(testbuf_1) );
        test( tausch_iter_is_ok(&iter), LINE("Initiation of iter went wrong") );

        printf(" --- Decode to stuffing \n");
        test( tausch_iter_go_to_stuffing(&iter), LINE("EOF shall return true when going to stuffing") );
        uint8_t expect_3[] = { 0x02, 0x03, 0x00, 0x00, 0x00 };
        BINCOMP( testbuf_1 +6+3, expect_3, LINE("it is not the stuffing we must have been found.") );
        test( 5 == tausch_iter_is_stuffing( &iter ), LINE("tausch_iter_is_stuffing: the amount of stuffing is wrong.") );

        printf(" --- Testing of decoding to end of buffer \n" );
        test( ! tausch_iter_go_to_tag(&iter, 333), LINE("the tag 333 was found but we shall not have it. ") );

    }

    {
        tausch_iter_t iter = iterini;

        printf(" --- Testing writing of the boolean. \n");
        test( tausch_iter_exit_scope(&iter), LINE("tausch_decode_to_eoscope must return true on EOF.") );
        bool val = false;
        test( tausch_iter_write( &iter, 22, (bool*)NULL ), LINE("writing bool failed") );
        test( (iter.next - iter.idx) == 1, LINE("must have been writing only one byte") );
        test( tausch_iter_read( &iter, &val ), LINE("reading bool failed") );
        test( val == true, LINE("the readout is not true") );
        test( tausch_iter_write( &iter, 22, &val ), LINE("writing true over true must have been passing in tag only mode") );
        val = false;
        test( tausch_iter_write( &iter, 22, &val ), LINE("writing false over true must have been passing in tag only mode") );
        test( tausch_iter_read( &iter, &val ), LINE("reading stuffing as bool must have been passing") );
        test( val == false, LINE("the readout is not false") );
    }

    {
        tausch_iter_t iter = iterini;
        printf(" --- Iterating the buffer one by one at the same scope (root scope). \n");
        test( tausch_iter_next(&iter), LINE("Iterating must have passed") );
        test( iter.tag == 2, LINE("incorrect tag was read out") );
        test( tausch_iter_next(&iter), LINE("Iterating must have passed") );
        test( iter.tag == 0x11111, LINE("incorrect tag was read out") );
        test( tausch_iter_next(&iter), LINE("Iterating must have passed") );
        test( iter.tag == 0, LINE("incorrect tag was read out") );
        test( iter.vlen == 3, LINE("incorrect vlen was read out") );
        test( tausch_iter_next(&iter), LINE("Iterating must have passed") );
        test( iter.tag == 0, LINE("incorrect tag was read out") );
        test( iter.vlen == 0, LINE("incorrect vlen was read out") );
        test( ! tausch_iter_next(&iter), LINE("Iterating must have failed") );
        test( iter.tag == 1, LINE("incorrect tag was read out") );
        test( ! tausch_iter_next(&iter), LINE("Iterating must have passed") );
        test( iter.tag == 1, LINE("incorrect tag was read out") );
        test( tausch_iter_is_eof( &iter ), LINE("iter must point to EOF.") );
    }

    {
        tausch_iter_t iter = iterini;
        printf(" --- Iterating the buffer to the item at the same scope (root scope). \n");
        test( tausch_iter_go_to_tag(&iter, 2), LINE("Iterating must have passed") );
        test( iter.tag == 2, LINE("incorrect tag was read out") );
        iter = iterini;
        test( tausch_iter_go_to_tag(&iter, 0x11111), LINE("Iterating must have passed") );
        test( iter.tag == 0x11111, LINE("incorrect tag was read out") );
        iter = iterini;
        test( tausch_iter_go_to_tag(&iter, 0), LINE("Iterating must have passed") );
        test( iter.tag == 0, LINE("incorrect tag was read out") );
        test( iter.vlen == 3, LINE("incorrect vlen was read out") );
        test( tausch_iter_go_to_tag(&iter, 0), LINE("Iterating must have passed") );
        test( iter.tag == 0, LINE("incorrect tag was read out") );
        test( iter.vlen == 0, LINE("incorrect vlen was read out") );
        iter = iterini;
        test( ! tausch_iter_go_to_tag(&iter,33), LINE("Iterating must have failed") );
        test( iter.tag == 1, LINE("incorrect tag was read out") );
        test( ! tausch_iter_go_to_tag(&iter,2), LINE("Iterating must have passed") );
        test( iter.tag == 1, LINE("incorrect tag was read out") );
        test( tausch_iter_is_eof( &iter ), LINE("iter must point to EOF.") );

        printf(" --- Iterating the buffer to the stuffing but EOF will be found. \n");
        test( tausch_iter_go_to_stuffing(&iter), LINE("EOF must result true when decoding to stuffing") );
        test( tausch_iter_is_eof( &iter ), LINE("iter must point to EOF.") );
    }

    {
        tausch_iter_t iter = iterini;

        printf(" --- Testing writing of the multibyte boolean. \n");
        test( tausch_iter_exit_scope(&iter), LINE("tausch_decode_to_eoscope must return true on EOF.") );
        bool val = false;
        test( tausch_iter_write( &iter, 22, &val ), LINE("writing bool failed") );
        test( (iter.next - iter.idx) == 3, LINE("must have been writing 3 bytes") );
        test( tausch_iter_read( &iter, &val ), LINE("reading bool failed") );
        test( val == false, LINE("the readout is not false") );
        val = true;
        test( tausch_iter_write( &iter, 22, &val ), LINE("writing false over true must have been passing") );
        test( tausch_iter_read( &iter, &val ), LINE("reading bool failed") );
        test( val == true, LINE("the readout is not true") );
    }

    {
        tausch_iter_t iter = iterini;

        printf(" --- Testing writing of typX. \n");
        test( tausch_iter_exit_scope(&iter), LINE("tausch_decode_to_eoscope must return true on EOF.") );

        uint16_t val = 27;
        test( tausch_iter_write( &iter, 35, &val), LINE("writing UINT-16 failed") );
        test( (iter.next - iter.idx) == 5, LINE("must have been writing 5 bytes") );
        test( iter.buf[iter.next] == 7, LINE("EOF must have been pushed.") );
        test( tausch_iter_vlen( &iter) == 2, LINE("the tausch_iter_vlen result is wrong") );
        test( tausch_iter_read( &iter, &val ), LINE("reading UINT-16 failed") );
        test( val == 27, LINE("the read out value is wrong") );
        val = 28;
        test( !tausch_iter_write( &iter,36, &val ), LINE("writing over with another tag must fail") );
        test( tausch_iter_read( &iter, &val ), LINE("reading UINT-16 failed") );
        test( val == 27, LINE("the read out value is wrong") );
        uint32_t val32 = 29;
        test( !tausch_iter_write( &iter,35, &val32 ), LINE("writing over with another value length must fail") );
        test( tausch_iter_read( &iter, &val ), LINE("reading UINT-32 failed") );
        test( val == 27, LINE("the read out value is wrong") );
        uint8_t val8 = 30;
        test( !tausch_iter_write( &iter,35, &val8 ), LINE("writing over with another value length must fail") );
        test( tausch_iter_read( &iter, &val ), LINE("reading UINT-16 failed") );
        test( val == 27, LINE("the read out value is wrong") );

        test( !tausch_iter_next( &iter), LINE("decode next must have return false because of EOF") );
        test( tausch_iter_write_typX( &iter,40,NULL,9 ) == 9, LINE("writing 9 filled bytes did not succeed quite") );

        uint8_t blob_buf_1[20];
        tausch_blob_t blob_1 = { .buf = blob_buf_1, .len = sizeof(blob_buf_1) };
        test( tausch_iter_read( &iter, &blob_1) == 9, LINE("reading 9 filled bytes did not succeed quite") );
        test( tausch_iter_write( &iter, 40, &blob_1) == 9, LINE("overwriting blob with bigger data must write as much as space in message") );
        test( !tausch_iter_next( &iter ), LINE("advancing to eof must return false") );
        test( tausch_iter_write( &iter, 40, &blob_1) == 20, LINE("writing blob failed") );
        test( !tausch_iter_next( &iter ), LINE("advancing to eof must return false") );
        test( tausch_iter_write( &iter, 40, (tausch_blob_t*)NULL ) == 0, LINE("writing null blob must fail, use bool instead") ); // FIXME:

        printf(" --- Testing of writing UTF8 \n" );
        test( !tausch_iter_next( &iter ), LINE("advancing to eof must return false") );
        test( tausch_iter_write( &iter, 41, "The utf8 äöüš"), LINE("writing of utf8 failed") );
        HEXCOMP( &iter.buf[iter.idx],
            "a6,01,11,54,68,65,20,75,74,66,38,20,c3,a4,c3,b6,c3,bc,c5,a1,07",
            LINE("different message") );
        iter = iterini;
        test( tausch_iter_go_to_tag( &iter, 40), LINE("finding blob first item failed") );
        HEXCOMP( &iter.buf[iter.idx],
            "a2,01,09,00,00,00,00,00,00,00,00,00",
            LINE("different message") );
        test( tausch_iter_go_to_tag( &iter, 40), LINE("finding blob second item failed") );
        //printhex( ">", iter.idx, iter.next );
        HEXCOMP( &iter.buf[iter.idx],
            "a2,01,14,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00",
            LINE("different message") );
        test( tausch_iter_vlen( &iter ) == 20, LINE("the length of the item is wrong") );
        //printhex( ">", iter.idx, iter.next );
        test( tausch_iter_write( &iter, 40, "äöüš second"), "overwriting blob with utf8 failed" );
        HEXCOMP( &iter.buf[iter.idx],
            "a2,01,0f,c3,a4,c3,b6,c3,bc,c5,a1,20,73,65,63,6f,6e,64",
            LINE("different message") );
        test( tausch_iter_write_typX( &iter, 40, NULL, iter.vlen), LINE("overwriting with 0x00 failed") );
        //printhex( ">", iter.idx, iter.next );
        HEXCOMP( &iter.buf[iter.idx],
            "a2,01,0f,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00",
            LINE("different message") );
        test( tausch_iter_write( &iter, 40, NULL), LINE("overwriting with 0x00 failed") );
        //printhex( ">", iter.idx, iter.next );
        HEXCOMP( &iter.buf[iter.idx],
            "a2,01,0f,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00",
            LINE("different message") );

    }

    {
        tausch_iter_t iter = iterini;

        printf(" --- Testing writing of typX null. \n");
        test( tausch_iter_exit_scope(&iter), LINE("tausch_decode_to_eoscope must return true on EOF.") );
        test( tausch_iter_write( &iter, 32, NULL), LINE("writing null failed") );
        test( (iter.next - iter.idx) == 2, LINE("must have been writing 2 bytes") );
        test( tausch_iter_is_null( &iter), LINE("iter must be as null") );
        test( tausch_iter_vlen(&iter) == 0, LINE("iter value len must be 0") );
        test( ! tausch_iter_next( &iter), LINE("iterating to EOC must fail") );
        test( tausch_iter_vlen(&iter) == 0, LINE("iter at EOF vlen must be 0") );

    }

    {
        tausch_iter_t iter = iterini;
        uint32_t val = 10;

        printf(" --- Testing writing of scoped data. \n");
        test( tausch_iter_exit_scope(&iter), LINE("tausch_decode_to_eoscope must return true on EOF.") );
        test( tausch_iter_write_scope(&iter, 100), LINE("writing of scope failed") );
        test( tausch_iter_enter_scope(&iter), LINE("entering to the scope failed") );
        test( !tausch_iter_next(&iter), LINE("advancing to eof must return false") );
        test( tausch_iter_write(&iter, 1, &val), LINE("writing value failed") );
        test( !tausch_iter_next(&iter), LINE("advancing to eof must return false") );
        test( tausch_iter_write_end(&iter), LINE("writing end failed") );
        test( !tausch_iter_next(&iter), LINE("advancing to next must returng false and stay to end") );
        test( tausch_iter_is_end(&iter), LINE("iter must stay to the end") );
        test( !tausch_iter_is_eof(&iter), LINE("iter must not be eof but end") );
        test( iter.buf[iter.next] == 7, LINE("the eof must have been pushed into message") );
        test( tausch_iter_exit_scope(&iter), LINE("exiting the scope failed") );
        test( !tausch_iter_next(&iter), LINE("advancing to eof must return false") );
        test( tausch_iter_is_eof(&iter), LINE("iter must be eof now") );
        test( tausch_iter_write(&iter, 101, &val), LINE("writing value failed") );

        iter = iterini;
        test( tausch_iter_go_to_tag( &iter, 100), LINE("finding tag 100 failed") );
        test( tausch_iter_enter_scope( &iter ), LINE("entering into scope failed") );
        test( tausch_iter_go_to_tag( &iter, 1), LINE("finding tag 100/1 failed") );
        test( tausch_iter_exit_scope( &iter ), LINE("finding end failed") );
        test( iter.buf[iter.idx] != 0x03, LINE("iter must not be at the END tag") );
        test( tausch_iter_next(&iter), LINE("advancing to next failed") );
        test( iter.tag == 101, LINE("the tag after the scope must be now") );
        test( !tausch_iter_next(&iter), LINE("advancing to odf must return false") );
        test( tausch_iter_write( &iter, 2, "a") > 0, LINE("writing string tag failed") );
        test( !tausch_iter_next(&iter), LINE("advancing to eof must return false") );
        test( ! tausch_iter_write_scope(&iter, 3), LINE("FIXME: writing of scope at the end of buffer must fail") );

        iter = iterini;
        test( tausch_iter_go_to_tag( &iter, 100), LINE("finding tag 100 failed") );
        test( tausch_iter_enter_scope(&iter), LINE("entering into scope failed") );
        test( ! tausch_iter_go_to_tag( &iter, 2), LINE("finding tag 100/2 must fail") );

        iter = iterini;
        test( tausch_iter_go_to_tag( &iter, 100), LINE("finding tag 100 failed") );
        test( tausch_iter_enter_scope(&iter), LINE("entering into scope failed") );
        test( ! tausch_iter_go_to_stuffing( &iter), LINE("finding stuffing must fail") );
        test( tausch_iter_exit_scope(&iter), LINE("decoding to eoscope failed") );
    }

    {
        printf(" --- Testing over the edge decoding. \n");
        uint8_t buf[] = {0b1010,20,0,0,0,};
        tausch_iter_t iter = TAUSCH_ITER_INIT( buf, sizeof(buf) );
        test( ! tausch_iter_next(&iter), LINE("over the edge decode next failed") );

    }

    {
        tausch_iter_t iter = iterini;
        iterini = iter; // to avoid compiler warning

#if 0 // Testing of linking failure
        char mem[10];
        tausch_iter_read( &iter, mem );
#endif

    }

    return true;
}

