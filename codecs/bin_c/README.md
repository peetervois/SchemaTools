# TauSchema TLV Codec C

TauSchema codec for C language does parse, modify and compose TLV binary message. The message is as flexible as JSON.

## Usage

TLV reading and writing does have two level interface. 

Level 0 interface does read and write raw TLV without any knowledge of the of the data schema. 
The develper needs to keep track of the TLV structure, data types e.t.c.
Although the interface does have powerful functions, we do not describe it here.
You can read about the interface from the header file "tauschema_codec.h".

Level 1 interface does read and write the TLV and keeps track the TLV message against the data schema.
The functions are more powerful, helping to perform number data type conversions, 
opening and closing the collections and variadics, verifies that the collection consists of only unique
tags, e.t.c. The main reason is to protect the application software against data objects that
are not descirbed by the schema.


``` C

#include "tauschema_check.h"
#include "tauschema_device_info_schema.h"

const char vendor[]= "Tauria Ltd";
const char serial[]= "abcdefghijk";
const char device[]= "tauschema-demo-app";
const uint8_t max_msgsize = 64;

/**
 * This is server message handler. Client has sent a message
 * and has requested some data, we need to fill in the values.
 */
void handle( uint8_t *buf, size_t len )
{
	// The flat tree schema model is generated into ROM memory.
	// The tausch_schema_t does track the parts of the file.
	// Also dynamic loading of the schema is possible.
	tausch_schema_t devinfo_schema;
	
	tausch_schema_init( &devinfo_schema, tauschema_device_info_flatrows, tauschema_device_info_flatsize );

	// The iterator of the flat tree is called flaterator.
	// At the same time, the flaterator holds an TLV message iterator
	// for reading and writing the message.
	tausch_flater_t fl;
	
	tausch_flater_init( &fl, &devinfo_schema, buf, sizeof (buf) );
	
	tsch_size_t tag_n = 0;
	while( tausch_flater_tag_n( tausch_flater_next( &fl ) ) != TAUSCH_NAM_DEVICE_INFO_ )
	{
		switch( tausch_flater_tag_n( &fl ) )
		{
			case TAUSCH_NAM_DEVICE_INFO_info:
			{	
				tausch_flater_t fc = tausch_flater_clone( &fl );
				while( tausch_flater_tag_n( tausch_flater_next( &fc ) ) != TAUSCH_NAM_DEVICE_INFO_ )
				{
					// Reduce source code depth with special handler, also it is repeated handler
					// for data class slice.
					// slice : COLLECTION
					//    orig : UINT-32 = 1
					//    data : BLOB = 2
					// : END
					void slice_txt_handler( tausch_flater_t *fc, char *text )
					{
							uint16_t idx = 0;
							uint16_t len = strnlen( text, max_msgsize );
							tausch_flater_t fcc = tausch_flater_clone( &fc );
							
							tausch_flater_read( &fcc, &idx, TAUSCH_NAM_DEVICE_INFO_orig );
							if( idx > len ) idx = len;
							tausch_flater_write( &fcc, TAUSCH_NAM_DEVICE_INFO_data, (char*)&text[idx] );
					};
					//
					// Here fill in the requested data fields
					//
					switch( tausch_flater_tag_n( &fc ) )
					{
						case TAUSCH_NAM_DEVICE_INFO_msglen:
							tausch_flater_write( &fc, TAUSCH_NAM_DEVICE_INFO_msglen, &max_msgsize );
							break;
						case TAUSCH_NAM_DEVICE_INFO_vendor:
							slice_txt_handler( &fc, vendor );
							break;
						case TAUSCH_NAM_DEVICE_INFO_serial:
							slice_txt_handler( &fc, serial );
							break;
						case TAUSCH_NAM_DEVICE_INFO_name:
							slice_txt_handler( &fc, device );
							break;
						default:
							// we do not support this field, for avoiding confusion, we will erase it
							// from the response message. It will be replaced with free memory field
							// with no particular meaning.
							tausch_flater_erase( &fc );
							break;
					}
				}
				break;
			}
			default:
				// we do not support this field, for avoiding confusion, we will erase it
				// from the response message. It will be replaced with free memory field
				// with no particular meaning.
				tausch_flater_erase( &fc );
				break;
		}
	}
}
```

And the client side does first compose the message like this:

``` C
#include "tauschema_check.h"
#include "tauschema_device_info_schema.h"

void info_request( uint8_t *buf, size_t len )
{
	tausch_schema_t devinfo_schema;
	tausch_schema_init( &devinfo_schema, tauschema_device_info_flatrows, tauschema_device_info_flatsize );

	tausch_flater_t fl;
	tausch_flater_init( &fl, &devinfo_schema, buf, sizeof (buf) );

	bool ok = TAUSCH_FLATER_WRITE_SCOPE( &fl, TAUSCH_NAM_DEVICE_INFO_info )
	{
		ok = ok && (tausch_flater_write( sfl, TAUSCH_NAM_DEVICE_INFO_msglen, (uint32_t*)NULL ) > 0);
		if( schema_url_is_partial() )
		{ 
			ok = ok && TAUSCH_FLATER_WRITE_SCOPE( sfl, TAUSCH_NAM_DEVICE_INFO_schurl )
			{
				uint8_t orig = get_missing_schema_url_origin();
				tausch_blob_t emptyblob = { .buf = NULL, .len = 16 };
				ok = ok && (tausch_flater_write( sfl, TAUSCH_NAM_DEVICE_INFO_orig, &orig ) > 0);
				ok = ok && (tausch_flater_write( sfl, TAUSCH_NAM_DEVICE_INFO_data, &emptyblob ) > 0);
				return ok;
			}
			TAUSCH_FLATER_CLOSE_SCOPE;
		}
		return ok;
	}
	TAUSCH_FLATER_CLOSE_SCOPE;
	
	if( !ok )
	{
		// while(1); // debug trap
	}
}


```

## LICENSE



Copyright (C) 2020 by Tauria Ltd tauria@tauria.ee

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

