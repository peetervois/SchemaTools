# TauSchema Codec C

TauSchema codec for C language does parse, modify and compose TLV binary message. The message is as flexible as JSON.

## Usage

The low level interface does use Iterators to navigate through the binary. It is possible to overwrite values, append values to
the end of message, clear the TLV elements by overwriting them with "Stuffing" (a special TLV whose tag id is 0).

```c
#include "tauschema_codec.h"

uint8_t messagebuf[100];

void handle( uint8_t *buf, size_t len )
{
	tausch_iter_t iter, iter_init;

	// Initialize the iterator
	
	tausch_iter_init( &iter, buf, len );   // initialize the iterator
	iter_init = iter;                      // store the value for easy reinitialization
	
	// Decode the iterator to the element that we
	// are interested of. Search for the root level tag.
	
	if( ! tausch_decode_to_tag( &iter, MY_TAG ) )
	{
		// The tag does not exist
		return;
	}
	
	// If MY_TAG is collection, then iterator entered
	// directly into beginning of the subscope of the MY_TAG.
	// Now decode the iterator to the MY_TAG.MY_SUBTAG
	
	if( ! tausch_decode_to_tag( &iter, MY_SUBTAG ) )
	{
		// The tag does not exist
		return;
	}
	
	if( tausch_iter_is_null( &iter ) )
	{
		// The item is provided but does not contain any data
		return;
	}
	
	int32_t getval = 0;
	if( ! tausch_read( &iter, &getval ) )
	{
		// the reading of the value failed
		return;
	}
	
	// If you want to overwrite the value in the message
	
	int32_t value = 43;
	if( ! tausch_write( &iter, MY_SUBTAG, &value ) )
	{
		// overwriting failed
		return;
	}
	
	// To exit from the subscope
	
	if( ! tausch_decode_to_end( &iter ) )
	{
		// Failed to find the end of the scope
		return;
	}
	
	// To restart looking for another tag
	
	iter = iter_init;   // the easy initialization
	
	if( ! tausch_decode_to_tag( &iter, MY_ANOTHER_TAG ) )
	{
		// The tag does not exist
		return;
	}
}

```

The same above in much dense coding style:

```C
void handle( uint8_t *buf, size_t len )
{
	tausch_iter_t iter, iter_init;
	bool error = false;

	// Initialize the iterator
	
	error ||= tausch_iter_init( &iter, buf, len );   // initialize the iterator
	iter_init = iter;                      // store the value for easy reinitialization
	
	error ||= ! tausch_decode_to_tag( &iter, MY_TAG ) );
	error ||= ! tausch_decode_to_tag( &iter, MY_SUBTAG ) );
	error ||= ! tausch_iter_is_null( &iter ) );
	int32_t getval = 0;
	error ||= ! tausch_read( &iter, &getval ) );
	int32_t value = 43;
	error ||= ! tausch_write( &iter, MY_SUBTAG, &value ) );
	error ||= ! tausch_decode_to_end( &iter ) );
	
	iter = iter_init;   // the easy initialization
	
	error ||= ! tausch_decode_to_tag( &iter, MY_ANOTHER_TAG ) );
	
	if( ! error )
	{
		// do the response part...
	}
}
```


## Binary Format

The binary message format is as follows:

```
MESSAGE := TLV [,TLV [, ... ] ]

TLV := VLUTAG [,VLULENGTH [,VALUE] ]

VLUTAG := VLUINT( TXLC )
VLULENTH := VLUINT( LENGTH )
VALUE := BYTE [,BYTE [,...] ]

TXLC := {TAG[31:2],L[1],C[1]}
VLUINT( X ) := {N[7]=(X>2^7-1),X[6:0]}[, {N[7]=(X>2^(2*7)-1,X[13:7]}[, ..] ]
```

Message contains sequence of TLV elements. Each TLV element must contain Tag value TXLC, 
where X is the tag of object, L flag defines if the VLULENGTH item follows. When the 
LENGTH is nonzero, then the value bytes follow as much as the length specifies. The flag
C indicates wether the tag is collection.

## Collections, values

| X>0 |  L  |  C  | Alias | Description  |
| --- | --- | --- | ---   | ---  |
|  0  |  0  |  0  | T000  | Stuffing byte; use this to add just a single byte placeholder |
|  0  |  1  |  0  | T010  | Treat as another way of providing stuffing bytes, Length field must follow, Value field may follow if length is not 0. Keep the value field 0 for security. |
|  X  |  0  |  0  | TX00  | Boolean value “X” = True or some other element does have zero length content but is provided. In case of boolean if not present then the boolean value is False, the length of Value is 0 Bytes, no value field follows. Also "null" values may be formatted this way.  |
|  X  |  1  |  0  | TX10  | Any Tag ID X with data; The Length is encoded as following VLUINT field to the Tag VLUINT field, Value field is encoded when the Length is >0. TX10, 0x00 represents boolean value “X” = False or the item other than boolean does have zero length content but is provided.     |
|  0  |  0  |  1  | T001  | Start of Schema messaging collection. No Length field follows, no Value field follows, at some point must be matched with T011 to end the collection. Information about the connected device and schema will be transferred using TLV specified in this standard.     |
|  X  |  0  |  1  | TX01  | Start of collection or variadic array X; No Length field follows, no Value field follows, at some point must be matched with T011 to end the collection or variadic array     |
|  0  |  1  |  1  | T011  | End of collection or variadic array.    |
|  X  |  1  |  1  | TX11  | End of Message / End of File (EOF)    |


## Schema and Device info TLV

To use the binary messages, Schema needs to be known for all parties. The schema may be transferred and information about the device can be transferred with the special Schema interface. The device info messaging collection is opened with tag T001 (0x01) and closed with tag T011 (0x03)

```
#
# Schema for exchange of the device information and interface Schema.
# This is the standardized way of TaTLV, the schema here belongs to
# the content of tag 0, which may not be specified in schema file.
#
 
slice : COLLECTION
  #
  # Slice collection is used for transferring large data piece wize.
  #
  orig : UINT-32 = 2  # The origin of the blob from the full data, 
                      #   count as 0 if omitted.
  size : UINT-16 = 3  # Response total number of bytes in requested
                      #   elements. If size is not requested then
                      #   it is not returned
  data : BLOB = 1     # The data slice returned. On request the
                      #   value field is stuffed with zeroes.
                      #   On response it is filled with data.
slice : END
 
 
schrow : COLLECTION
  #
  # The schema row in request and response.
  # Some fields are required {req}.
  #
  item : UINT-16 = 1   # {req} Item number in the binary.
  name : slice = 2     # {req} UTF8, Item name in the code 
                       #   and json.
  desc : slice = 3     # UTF8, Description of the item for 
                       #   engineer to read.
  type : UINT-8 = 4    # {req} Type number of the item.
  sub  : UINT-16 = 5   # {req} Flat tree index to first subitem 
                       #   of this item (COLLECTION and VARIADIC).
  next : UINT-16 = 6   # {req} Flat tree index to next item at the 
                       #   same scope.
  idx  : UINT-16 = 7   # Index of the row, on request and 
                       #   response, if omitted consider as 0.
schrow : END
 
 
info : COLLECTION        
  #
  # Description of the device information.
  #
  name : slice = 1       # UTF8, Device name.
  version : slice = 2    # UTF8, Device version.
  serial : slice = 3     # UTF8, Device serial code.
  vendor : slice = 4     # UTF8, Device vendor information.
  schtxt : slice = 5     # UTF8, The schema as text file.
  schurl : slice = 6     # UTF8, Link to the schema URL.
  schbin : VARIADIC = 7  # The array of schema flat tree.
    .schrow = 1          # Row description
  schbin : END  
info : END
 
 
get : info = 1           
  #
  # Ask for the device information. Provide the value items 
  # with content all bytes 0x00. Into response the data may
  # be filled on site.
  #
 
 
ret : info = 2     
  #      
  # Returned response to the get request. Contains only
  # requested items if exist. The response may be filled
  # on site. When the item does not exist, then the field
  # may be replaced with stuffing T0?0. When blobs contain
  # less data than field has, then remainder is turned into
  # stuffing with T0?0.
  #
```

## LICENSE



Copyright (C) 2020 by Tauria Ltd tauria@tauria.ee

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

