# TauSchema TLV Codec for PHP

TauSchema codec for PHP language does parse, modify and compose TLV binary message. The message is as flexible as JSON.

## Usage

The low level interface does use Iterators to navigate through the binary. It is possible to overwrite values, append values to
the end of message, clear the TLV elements by overwriting them with "Stuffing" (a special TLV whose tag id is 0).

``` php
<?php

include_once 'path/to/tauschema_codec.php';

function handle( string &$buf )
{
	$iter_ini = new tauschema\Iterator( $buf );
	$iter = $iter_ini->clone();
	$error = ! $iter_>is_ok();
	
	$error = $error || ! $iter->enter_scope( MY_TAG );
	$error = $error || ! $iter->go_to_tag( MY_SUBTAG );
	$error = $error || ! $iter->is_null();
	$getval = 0;
	$error = $error || ! $iter->read( $getval, "SINT-32" );
	$value = 43;
	$error = $error || ! $iter->write( MY_SUBTAG, $value, "SINT-32" );
	$error = $error || ! $iter->exit_scope();
	
	$iter->copy_from( $iter_ini );   // the easy initialization
	
	$error = $error || ! $iter->go_to_tag( MY_ANOTHER_TAG );
	
	if( ! $error )
	{
		// do the response part...
	}
}

?>

```