# TauSchema TLV Codec C

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
	
	if( ! tausch_iter_go_to_tag( &iter, MY_TAG ) )
	{
		// The tag does not exist
		return;
	}

	// Iterator does not change into subscope automatically
	if( ! tausch_iter_enter_scope( &iter ) )
	{
		// The iterator apears not to be a collection
		return;
	}
	
	if( ! tausch_iter_go_to_tag( &iter, MY_SUBTAG ) )
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
	if( ! tausch_iter_read( &iter, &getval ) )
	{
		// the reading of the value failed
		return;
	}
	
	// If you want to overwrite the value in the message
	
	int32_t value = 43;
	if( ! tausch_iter_write( &iter, MY_SUBTAG, &value ) )
	{
		// overwriting failed
		return;
	}
	
	// To exit from the subscope
	
	if( ! tausch_iter_exit_scope( &iter ) )
	{
		// Failed to find the end of the scope
		return;
	}
	
	// To restart looking for another tag
	
	iter = iter_init;   // the easy initialization
	
	if( ! tausch_iter_go_to_tag( &iter, MY_ANOTHER_TAG ) )
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
	
	error = error || tausch_iter_init( &iter, buf, len );   // initialize the iterator
	iter_init = iter;                      // store the value for easy reinitialization
	
	error = error || ! tausch_iter_go_to_tag( &iter, MY_TAG );
	error = error || ! tausch_iter_enter_scope( &iter );
	error = error || ! tausch_iter_go_to_tag( &iter, MY_SUBTAG );
	error = error || ! tausch_iter_is_null( &iter );
	int32_t getval = 0;
	error = error || ! tausch_iter_read( &iter, &getval );
	int32_t value = 43;
	error = error || ! tausch_iter_write( &iter, MY_SUBTAG, &value );
	error = error || ! tausch_iter_exit_scope( &iter );
	
	iter = iter_init;   // the easy initialization
	
	error = error || ! tausch_iter_go_to_tag( &iter, MY_ANOTHER_TAG );
	
	if( ! error )
	{
		// do the response part...
	}
}
```

## LICENSE



Copyright (C) 2020 by Tauria Ltd tauria@tauria.ee

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

