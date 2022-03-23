#include "testmain.h"
#include "../src/tauschema_check.h"
#include "tauschema_device_info_schema.h"

bool test_flater( void )
{
    uint8_t buf[100];   // the message to flaterate
    char errorbuf[500];   // temporary error message

    {
        printf( "\n### Producing message for flaterator to iterate \n\n" );

        tausch_format_buf( buf );
        tausch_iter_t iter_ini = TAUSCH_ITER_INIT(buf,sizeof(buf));
        tausch_iter_t iter = iter_ini;
        uint32_t u32;

        test( !tausch_iter_next( &iter ), LINE( "" ) );
        test( tausch_iter_write_scope( &iter, 1 ), LINE( "" ) );
        test( tausch_iter_enter_scope( &iter ), LINE( "" ) );
        test( !tausch_iter_next( &iter ), LINE( "" ) );
        u32 = 100;
        test( tausch_iter_write( &iter, 8, &u32 ), LINE("") );
        test( !tausch_iter_next( &iter ), LINE( "" ) );
        test( tausch_iter_write_scope( &iter, 3 ), LINE( "" ) );
        test( tausch_iter_enter_scope( &iter ), LINE( "" ) );
        test( !tausch_iter_next( &iter ), LINE( "" ) );
        u32 = 0;
        test( tausch_iter_write( &iter, 2, &u32 ), LINE("") );
        test( !tausch_iter_next( &iter ), LINE( "" ) );
        test( tausch_iter_write( &iter, 1, "thisisablob"), LINE(""));
        test( !tausch_iter_next( &iter ), LINE( "" ) );
        test( tausch_iter_write_end( &iter ), LINE( "" ) );
        test( tausch_iter_exit_scope( &iter ), LINE( "" ) );
        test( !tausch_iter_next( &iter ), LINE( "" ) );
        test( tausch_iter_write_end( &iter ), LINE( "" ) );
        test( tausch_iter_exit_scope( &iter ), LINE( "" ) );
        test( !tausch_iter_next( &iter ), LINE( "" ) );
        HEXCOMP( buf,
            "05,22,04,64,00,00,00,0d,0a,04,00,00,00,00,06,0b,"
                "74,68,69,73,69,73,61,62,6c,6f,62,03,03,07",
            LINE("") );
    }

    {
        printf( "\n### Flaterator tests.\n\n" );

        printf( "   -- Running the method next \n" );
        tausch_schema_t devinfo_schema;
        tausch_flater_t fl;

        tausch_schema_init( &devinfo_schema, tauschema_device_info_flatrows, tauschema_device_info_flatsize );
        tausch_flater_init( &fl, &devinfo_schema, buf, sizeof (buf) );

        tausch_flater_next( &fl );
        test( tausch_flater_tag_n(&fl) == TAUSCH_NAM_DEVICE_INFO_info, LINE( "" ) );

        tausch_flater_next( &fl );
        test( tausch_flater_tag_n(&fl) == TAUSCH_NAM_DEVICE_INFO_, LINE( "" ) );

        printf( "   -- Testing reset \n" );

        tausch_flater_reset( &fl );
        tausch_flater_next( &fl );
        test( tausch_flater_tag_n(&fl) == TAUSCH_NAM_DEVICE_INFO_info, LINE( "" ) );

        printf( "   -- Testing clone \n" );

        tausch_flater_t fc = tausch_flater_clone( &fl );
        tausch_flater_next( &fc );
        test( tausch_flater_tag_n(&fc) == TAUSCH_NAM_DEVICE_INFO_msglen, LINE( "" ) );
        test( tausch_flater_tag_n(&fl) == TAUSCH_NAM_DEVICE_INFO_info, LINE( "" ) );
        tausch_flater_next( &fc );
        test( tausch_flater_tag_n(&fc) == TAUSCH_NAM_DEVICE_INFO_serial, LINE( "" ) );
        tausch_flater_next( &fc );
        test( tausch_flater_tag_n(&fc) == TAUSCH_NAM_DEVICE_INFO_, LINE( "" ) );
        tausch_flater_next( &fc );
        test( tausch_flater_tag_n(&fc) == TAUSCH_NAM_DEVICE_INFO_, LINE( "" ) );

        printf( "   -- Testing go_to \n" );
        fc = tausch_flater_clone( &fl );
        test(
            tausch_flater_tag_n(tausch_flater_go_to(&fc,TAUSCH_NAM_DEVICE_INFO_serial)) == TAUSCH_NAM_DEVICE_INFO_serial,
            LINE( "" ) );
        test( tausch_flater_tag_n(tausch_flater_go_to(&fc,TAUSCH_NAM_DEVICE_INFO_msglen)) == TAUSCH_NAM_DEVICE_INFO_,
            LINE( "" ) );
        fc = tausch_flater_clone( &fl );
        test( tausch_flater_tag_n(
            tausch_flater_next( tausch_flater_go_to( &fc, TAUSCH_NAM_DEVICE_INFO_msglen))
        ) == TAUSCH_NAM_DEVICE_INFO_serial, LINE( "" ) );
        test(
            tausch_flater_tag_n( tausch_flater_go_to( &fc, TAUSCH_NAM_DEVICE_INFO_data)) == TAUSCH_NAM_DEVICE_INFO_data,
            LINE( "" ) );

        printf( "   -- testing read with no argument \n" );

        TAUSCH_BLOB_NEW( stringblob, 100 );

        test( tausch_flater_read( &fc, &stringblob ) == 11, LINE( "" ) );
        STRCOMP( stringblob.buf, "thisisablob", LINE("") );

        printf( "   -- Testing read with name index argument \n" );
        uint32_t u32 = 0;
        test( tausch_flater_read( &fl, &u32, TAUSCH_NAM_DEVICE_INFO_msglen ) == 4, LINE( "" ) );
        test( u32 == 100, LINE( "" ) );
        fc = tausch_flater_clone( &fl );
        u32 = 0;
        test( tausch_flater_read( &fc, &u32, TAUSCH_NAM_DEVICE_INFO_msglen ) == 4, LINE( "" ) );
        test( u32 == 100, LINE( "" ) );
        tausch_flater_go_to( &fc, TAUSCH_NAM_DEVICE_INFO_serial );
        test( tausch_flater_read( &fc, &u32, TAUSCH_NAM_DEVICE_INFO_orig) == 4, LINE( "" ) );
        test( u32 == 0, LINE( "" ) );
        stringblob.buf[0] = 0;
        test( tausch_flater_read( &fc, &stringblob, TAUSCH_NAM_DEVICE_INFO_data ) == 11, LINE( "" ) );
        STRCOMP( stringblob.buf, "thisisablob", LINE("") );

        printf( "   -- Testing read with scoped name index argument \n" );
        uint8_t u8 = 4;
        test( tausch_flater_read( &fl, &u8, TAUSCH_NAM_DEVICE_INFO_serial, TAUSCH_NAM_DEVICE_INFO_orig) == 1,
            LINE( "" ) );
        test( u8 == 0, LINE( "" ) );
        test( tausch_flater_read( &fl, &u8, TAUSCH_NAM_DEVICE_INFO_msglen ) == 1, LINE( "" ) );
        test( u8 == 100, LINE( "" ) );
        stringblob.buf[0] = 0;
        test( tausch_flater_read( &fl, &stringblob, TAUSCH_NAM_DEVICE_INFO_serial, TAUSCH_NAM_DEVICE_INFO_data) == 11,
            LINE( "" ) );
        STRCOMP( stringblob.buf, "thisisablob", LINE("") );

        printf( "   -- Testing overwriting of a value \n" );
        tausch_flater_reset( &fl );
        u8 = 200;
        fc = tausch_flater_clone( &fl );
        test(
            tausch_flater_write( tausch_flater_go_to( &fc, TAUSCH_NAM_DEVICE_INFO_info), TAUSCH_NAM_DEVICE_INFO_msglen,
                &u8 )
                == 4, LINE( "" ) );
        uint16_t u16 = 9000;
        test( tausch_flater_read( &fl, &u16, TAUSCH_NAM_DEVICE_INFO_info, TAUSCH_NAM_DEVICE_INFO_msglen) == 2,
            LINE( "" ) );
        test( u16 == 200, LINE( "" ) );

        printf( "   -- Testing of adding a structure at the end \n" );
        tausch_flater_reset( &fl );
        bool ok = true;
        ok = ok && TAUSCH_FLATER_WRITE_SCOPE( &fl, TAUSCH_NAM_DEVICE_INFO_info )
        {
            uint8_t mslen = 140;
            ok = ok && (tausch_flater_write( sfl, TAUSCH_NAM_DEVICE_INFO_msglen, &mslen ) > 0);
            ok = ok && TAUSCH_FLATER_WRITE_SCOPE( sfl, TAUSCH_NAM_DEVICE_INFO_schurl )
            {
                uint8_t orig = 10;
                ok = ok && (tausch_flater_write( sfl, TAUSCH_NAM_DEVICE_INFO_orig, &orig ) > 0);
                ok = ok && (tausch_flater_write( sfl, TAUSCH_NAM_DEVICE_INFO_data, "http://www.tauria.ee" ) > 0);
                return ok;
            }
            TAUSCH_FLATER_CLOSE_SCOPE;
            return ok;
        }
        TAUSCH_FLATER_CLOSE_SCOPE;
        test( ok, LINE(""));
        test( tausch_flater_tag_n(&fl) == TAUSCH_NAM_DEVICE_INFO_info, LINE( "" ) );
        test( tausch_flater_read(&fl, &u8, TAUSCH_NAM_DEVICE_INFO_msglen) == 1, LINE(""));
        test( u8 == 140, LINE(""));
        test( tausch_flater_read(&fl, &u8, TAUSCH_NAM_DEVICE_INFO_schurl, TAUSCH_NAM_DEVICE_INFO_orig) == 1, LINE(""));
        test( u8 == 10, LINE(""));
        memset( stringblob.buf, 2, stringblob.len );
        test( tausch_flater_read(&fl, &stringblob, TAUSCH_NAM_DEVICE_INFO_schurl, TAUSCH_NAM_DEVICE_INFO_data) == 20, LINE(""));
        STRCOMP( stringblob.buf, "http://www.tauria.ee", LINE("") );
        test( stringblob.buf[20] == 0, LINE(""));
        test( tausch_flater_tag_n( tausch_flater_next( &fl )) == TAUSCH_NAM_DEVICE_INFO_, LINE(""));
        test( fl.iter.scope == 0, LINE(""));

        printf( "   -- Testing of writing empty blob (stuffing) into UTF8 field \n" );
        tausch_flater_reset( &fl );
        tausch_format_buf( buf );
        ok = ok && TAUSCH_FLATER_WRITE_SCOPE( &fl, TAUSCH_NAM_DEVICE_INFO_info )
		{
			tausch_blob_t emptyblob = { .buf = NULL, .len = 5 };
			ok = ok && (tausch_flater_write( sfl, TAUSCH_NAM_DEVICE_INFO_demostring, &emptyblob ) > 0);
			return ok;
		}
        TAUSCH_FLATER_CLOSE_SCOPE;

        test( ok, LINE(""));
        memset( stringblob.buf, 255, stringblob.len );
        test( tausch_flater_read(&fl, &stringblob, TAUSCH_NAM_DEVICE_INFO_demostring) == 5, LINE(""));
        test( stringblob.buf[0] == 0, LINE(""));
    }

    printf( " flater done \n\n");
    return true;
}
