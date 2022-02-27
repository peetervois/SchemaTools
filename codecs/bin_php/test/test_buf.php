<?php

include_once '../src/tauschema_codec.php';
include_once 'test_helpers.php';

$bufsize = 10;

echo " ## Testing constructors\n";

echo "   -- Producing empty buffer\n";
$iter = new tauschema\Iterator( $bufsize );
test\bincomp( $iter->message(), "\x07", "Must start with 0x07" );
test\step( strlen($iter->message()) == $bufsize, "Buffer is with wrong size." );

echo "   -- Starting iterator from message\n";
$buffer = "\x07.........";
$iter = new tauschema\Iterator( $buffer );
test\bincomp( $iter->message(), "\x07", "Must start with 0x07" );
test\step( strlen($iter->message()) == 10, "Buffer is with wrong size." );

echo "   -- Cloning iterator\n";
$cpy = $iter->clone();
$buffer[1] = chr(0);
test\bincomp( $cpy->message(), "\x07\x00" );
$iter->copy_from( $cpy );
$buffer[2] = chr(0);
test\bincomp( $cpy->message(), "\x07\x00\x00." );
test\bincomp( $iter->message(), "\x07\x00\x00." );

echo " ## Testing of writing TLV entries \n";

echo "   -- Testing of writing u32 with 1 byte tag and 1 byte length. \n";
$bufsize = 100;
$iter = new tauschema\Iterator( $bufsize );
$iter_ini = $iter->clone(); // iterator that has not parsed anything yet
$iter->next(); // we need to parse first thing

test\step( $iter->is_eof(), "Iter must be end of file");
$val_u32 = 0x12345678;
$expect = "\x0a\x04\x78\x56\x34\x12";
$rv = $iter->write( 2, $val_u32, "UINT-32" );
test\step( $rv == 4, "4 bytes must have been written, but was " .$rv );
test\bincomp( $iter->message(), $expect . "\x07" );
//echo bin2hex( $iter->message() ) . "\n";

echo "   -- Testing of advancing the iterator to write next value\n";
test\step( ! $iter->next(), "next must fail at the end of file" );

echo "   -- Testing of writing 3byte length of tag \n";
$val_u32 = 0x87654321;
$expect = "\x0a\x04\x78\x56\x34\x12";
$expect .= "\xc6\x88\x11\x04\x21\x43\x65\x87";
$rv = $iter->write( 0x11111, $val_u32, "UINT-32" );
test\step( $rv==4, "4 bytes must have been written" );
test\bincomp( $iter->message(), $expect . "\x07" );
//echo bin2hex( $iter->message() ) . "\n";
test\step( ! $iter->is_null(), "is_null must return false when something is not nullified" );

echo "   -- Testing nullification of the value\n";
$expect = "\x0a\x04\x78\x56\x34\x12";
$expect .= "\xc4\x88\x11\x02\x03\x00\x00\x00";
$rv = $iter->write( 0x11111 );
test\step( $rv == 1, "so called 1 bytes must have been written, but was $rv" );
test\bincomp( $iter->message(), $expect . "\x07" );
//echo bin2hex( $iter->message() ) . "\n";

echo "   -- Testing the is_null method \n";
test\step( $iter->is_null(), "is_null must return true when something is nullified " );

echo "   -- Testing of advancing the iterator to next, this time newly created stuffing. \n";
test\step( $iter->next(), "next failed" );
test\step( 5 == $iter->is_stuffing(), "iter_is_stuffing: the amount of stuffing is wrong.");

// FIXME: test writing of different types aswell

echo " ## Testing of advancing into EOF. \n";

echo "   -- decode to end reaches EOF.\n";
$iter->copy_from( $iter_ini );
test\step( $iter->next(), "decode to next failed.");
test\step( $iter->tag() == 2, "the very first tag must have been found.");
test\step( $iter->exit_scope(), "decode_to_eoscope must return true on EOF.");
test\step( ! $iter->is_complete(), "the iter shall not be complete. ");
test\step( $iter->is_eof(), "the item shall point to EOF");
test\step( $iter->exit_scope(), "decode_to_eoscope must return true on EOF second attempt.");
test\step( ! $iter->is_complete(), "the iter shall not be complete also after second attempt. ");
test\step( $iter->is_eof(), "the item shall point to EOF also after second attempt.");

echo "   -- Decode tgo stuffing\n";
$iter->copy_from( $iter_ini );
test\step( $iter->go_to_stuffing(), "stuffing was not found but it is there");
test\bincomp( substr( $iter->message(), $iter->idx(), 5), "\x02\x03\x00\x00\x00", "it is not the stuffing we must have been found." );
test\step( 5 == $iter->is_stuffing(), "iter_is_stuffing: the amount of stuffing is wrong." );

echo "   -- Testing of decoding to end of buffer by searching nonexisting tag \n";
test\step( ! $iter->go_to_tag( 333 ), "1- the tag 333 was found but we shall not have it." );
test\step( $iter->is_eof(), "the item shall point to EOF");
$iter->copy_from( $iter_ini );
test\step( ! $iter->go_to_tag( 333 ), "2- the tag 333 was found but we shall not have it." );
test\step( $iter->is_eof(), "the item shall point to EOF");

echo " ## Testing of singlebyte booleans. \n";

echo "   -- Testing writing of the boolean. \n";
$iter->copy_from( $iter_ini );
test\step( $iter->exit_scope(), "tausch_decode_to_eoscope must return true on EOF.");
$val_bool = false;
test\step( $iter->write( 22, NULL, "BOOL" ), "writing bool as tag only failed" );
test\bincomp( substr( $iter->message(), $iter->idx(), 2), "\x58\x07","must have been writing only one byte");
//echo bin2hex( substr( $iter->message(), $iter->idx(), 4) ) . "\n";
test\step( $iter->read( $val_bool, "BOOL"), "reading boolean failed.");
test\step( $val_bool == true, "the readout is not true" );
test\step( $iter->write( 22, $val_bool, "BOOL"), "writing true over true must have been passing in tag only mode." );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 10) ) . "\n";
test\bincomp( substr( $iter->message(), $iter->idx(), 2), "\x58\x07","must have been writing only one byte");
$val_bool = false;
test\step( $iter->write( 22, $val_bool, "BOOL"), "writing false over true must have been passing in tag only mode." );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 10) ) . "\n";
test\bincomp( substr( $iter->message(), $iter->idx(), 2), "\x00\x07","must have been writing only one byte");
$val_bool = true;
test\step( $iter->read( $val_bool, "BOOL"), "reading stuffing as bool must have been passing" );
test\step( $val_bool == false, "the readout is not false" );

echo " ## Testing of iterations.\n";

echo "   -- Iterating the buffer one by one at the same scope (root scope). \n";
$iter->copy_from( $iter_ini );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->next(), "Iterating must have passed");
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->tag() == 2, "incorrect tag was read out: 2 vs ".$iter->tag() );
test\step( $iter->next(), "Iterating must have passed");
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->tag() == 0x11111, "incorrect tag was read out: 0x11111 vs ".$iter->tag() );
test\step( $iter->next(), "Iterating must have passed");
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->tag() == 0, "incorrect tag was read out: 0 vs ".$iter->tag() );
test\step( $iter->vlen() == 3, "incorrect vlen was read out: 3 vs ".$iter->vlen() );
test\step( $iter->next(), "Iterating must have passed");
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->tag() == 0, "incorrect tag was read out: 0 vs ".$iter->tag() );
test\step( $iter->vlen() == 0, "incorrect vlen was read out: 0 vs ".$iter->vlen() );
test\step( ! $iter->next(), "Iterating must have failed");
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->tag() == 1, "incorrect tag was read out: -1 vs ".$iter->tag() );
test\step( ! $iter->next(), "Iterating must have failed");
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->tag() == 1, "incorrect tag was read out: -1 vs ".$iter->tag() );
test\step( $iter->is_eof(), "the item shall point to EOF");

echo "   -- Iterating the buffer to the stuffing but EOF will be found. \n";
test\step( $iter->go_to_stuffing(), "EOF must result true when decoding to stuffing");
test\step( $iter->is_eof(), "the item shall point to EOF");

echo " ## Testing multibyte boolean\n";

echo "   -- Writing multibyte false\n";
test\step( $iter->exit_scope(), "decode_to_eoscope must return true on EOF.");
$val_bool = false;
test\step( $iter->write( 22, $val_bool, "BOOL" ), "writing bool failed" );
test\bincomp( substr( $iter->message(), $iter->idx(), 4), "\x5a\x01\x00\x07", "must have been writing 3 bytes" );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 4) ) . "\n";
$val_bool = true;
test\step( $iter->read( $val_bool, "BOOL" ), "reading failed" );
test\step( $val_bool == false, "wrong value was read out, waiting false." );

echo "   -- Overwriting to true\n";
$val_bool = true;
test\step( $iter->write( 22, $val_bool, "BOOL"), "writing false over true must have been passing");
$val_bool = false;
test\step( $iter->read( $val_bool, "BOOL" ), "reading failed" );
test\step( $val_bool == true, "wrong value was read out, waiting true." );
test\bincomp( substr( $iter->message(), $iter->idx(), 4), "\x5a\x01\x01\x07", "must have been writing 3 bytes" );

echo " ## Testing writing of various types\n";

echo "   -- UINT-16\n";
$iter->copy_from($iter_ini);
test\step( $iter->exit_scope(), "decode_to_eoscope must return true on EOF.");
$val_u16 = 27;
test\step( $iter->write(35, $val_u16, "UINT-16"), "writing UINT-16 failed" );
test\bincomp( substr( $iter->message(), $iter->idx(), 6), "\x8e\x01\x02\x1b\x00\x07", "must have been writing 5 bytes" );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 6) ) . "\n";
test\step( $iter->vlen() == 2, "the tausch_iter_vlen result is wrong");
$val_u16 = 0;
test\step( $iter->read( $val_u16, "UINT-16" ), "1; reading UINT-16 failed" );
test\step( $val_u16 == 27, "1; the read out value is wrong, expect 27 but $val_u16" );
$val_u16 = 28;
test\step( ! $iter->write(36, $val_u16, "UINT-16"), "2; writing over with another tag value must fail" );
test\step( $iter->is_ok(), "2; iterator is broken" );
test\step( $iter->read( $val_u16, "UINT-16" ), "2; reading UINT-16 failed" );
test\step( $val_u16 == 27, "2; the read out value is wrong, expect 27 but $val_u16" );
$val_u32 = 29;
test\step( ! $iter->write(35, $val_u32, "UINT-32"), "3; writing over with another value length must fail" );
test\step( $iter->is_ok(), "3; iterator is broken" );
test\step( $iter->read( $val_u16, "UINT-16" ), "3; reading UINT-16 failed" );
test\step( $val_u16 == 27, "3; the read out value is wrong, expect 27 but $val_u16" );
$val_u8 = 30;
test\step( ! $iter->write(35, $val_u32, "UINT-8"), "4; writing over with another value length must fail" );
test\step( $iter->is_ok(), "4; iterator is broken" );
test\step( $iter->read( $val_u16, "UINT-16" ), "4; reading UINT-16 failed" );
test\step( $val_u16 == 27, "4; the read out value is wrong, expect 27 but $val_u16" );

echo "   -- BLOB\n";
test\step( !$iter->next(), "decode next must have been failed because of EOF");
test\step( $iter->write( 40, 9, "BLOB"), "writing 9 filled bytes (0) did not succeed");
test\bincomp( substr( $iter->message(), $iter->idx(), 15), "\xa2\x01\x09\x00\x00\x00\x00\x00\x00\x00\x00\x00\x07", "must have been writing 3 bytes" );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 15) ) . "\n";
$blob_20 = str_repeat(chr(0), 20);
test\step( $iter->read( $blob_20, "BLOB") == 9, "reading 9 filled bytes did not succeed");
$blob_20 = str_repeat(chr(0), 20);
test\step( ! $iter->write( 40, $blob_20, "BLOB"), "overwriting blob with bigger data must fail");
test\step( ! $iter->next(), "iterating to EOF must return false");
test\step( $iter->write( 40, $blob_20, "BLOB"), "writing blob failed, we must be able to write same tag again.");
test\step( ! $iter->next(), "iterating to EOF must return false");
test\step( $iter->write(40, NULL, "BLOB"), "writing null blob failed");
//echo bin2hex( substr( $iter->message(), $iter->idx(), 15) ) . "\n";

echo "   -- UTF-8\n";
test\step( ! $iter->next(), "iterating to EOF must return false");
test\step( $iter->write( 41, "The utf8 äöüš", "UTF-8") == 17, "writing of utf8 failed" );
test\bincomp( substr( $iter->message(), $iter->idx(), 25), "\xa6\x01\x11\x54\x68\x65\x20\x75\x74\x66\x38\x20\xc3\xa4\xc3\xb6\xc3\xbc\xc5\xa1\x07", "written utf8 differs" );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 25) ) . "\n";
$iter->copy_from( $iter_ini );
test\step( $iter->go_to_tag(40), "finding first blob item failed" );
test\step( $iter->go_to_tag(40), "finding second blob item failed" );
test\step( $iter->vlen() == 20, "the length of the item is wrong" );
test\step( $iter->write( 40, "äöüš second", "UTF-8"), "overwriting blob with utf8 failed" );
test\bincomp( substr( $iter->message(), $iter->idx(), 25), "\xa2\x01\x14\xc3\xa4\xc3\xb6\xc3\xbc\xc5\xa1\x20\x73\x65\x63\x6f\x6e\x64\x00\x00\x00\x00\x00", "written utf8 differs" );
test\step( $iter->write( 40, NULL, "ZERO"), "overwriting with with 0x00 failed" );
test\step( $iter->write( 40, NULL), "turning the item to true + stuffing, ie NULL value and stuff the remainder");
test\step( $iter->is_null(), "Iterator must be null" );
test\step( $iter->tag() == 40, "the tag must be 40");
test\step( $iter->vlen() == 0, "the value length must be 0");
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->go_to_tag(41), "finding UTF-8 item failed");
test\step( $iter->erase(), "erasing of UTF-8 item failed" );
test\step( $iter->is_stuffing() == 20, "iter must be stuffing with length 20" );
test\step( $iter->vlen() == 18, "vlen must be 18 but is ".$iter->vlen());
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\bincomp( substr( $iter->message(), $iter->idx(), 30), "\x02\x12\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x07" );

echo "   -- typX null.\n";
test\step( $iter->exit_scope(), "decode_to_eoscope must return true on EOF." );
test\step( $iter->write( 32, NULL), "writing null failed" );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\bincomp( substr( $iter->message(), $iter->idx(), 30), "\x80\x01\x07" );
test\step( $iter->is_null(), "iter must be as null" );
test\step( $iter->vlen() == 0, "iter value len must be 0" );
test\step( ! $iter->next(), "iterating out of EOF must fail" );
test\step( $iter->vlen() == 0, "iter value len must be 0" );

echo " ## Scoped data tests\n";

echo "   -- writing a scope with one variable\n";
test\step( $iter->exit_scope(), "decode_to_eoscope must return true on EOF." );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->write_scope(100), "writing scope failed" );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->enter_scope(), "entering scope has failed" );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( ! $iter->next(), "iterating to EOF must return false" );
test\step( $iter->write( 1, $val_u32, "UINT-32"), "writing uint32 failed" );
test\step( ! $iter->next(), "iterating to EOF must return false" );
test\step( $iter->write_end(), "writing end failed" );
test\step( $iter->exit_scope(), "exiting from scope failed" );
test\step( ! $iter->next(), "iterating to EOF must return false" );
test\step( $iter->write( 101, $val_u32, "UINT-32"), "writing uint32 failed" );

echo "   -- traversing scoped structure\n";
$iter->copy_from( $iter_ini );
test\step( $iter->go_to_tag(100), "finding tag 100 failed" );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\bincomp( substr( $iter->message(), $iter->idx(), 30), "\x91\x03\x06\x04\x1d\x00\x00\x00\x03\x96\x03\x04\x1d\x00\x00\x00\x07" );
test\step( $iter->enter_scope(), "entering into scope failed ");
test\step( $iter->go_to_tag(1), "finding tag 100/1 failed" );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->exit_scope(), "finding end failed" );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->next(), "advancing to next failed" );
//echo bin2hex( substr( $iter->message(), $iter->idx(), 30) ) . "\n";
test\step( $iter->tag() == 101, "iter must be at the element after the end of scope" );
test\step( ! $iter->next(), "iterating to EOF must return false" );
test\step( $iter->write( 2 ), "writing null tag failed" );
test\step( ! $iter->next(), "iterating to EOF must return false" );
test\step( $iter->buff_free() == 0, "Assumption failed that the buffer is full");
test\step( ! $iter->write_scope(3), "writing scope at the end of buffer must fail" );

echo "   -- looking for nonexisting subtag\n";
$iter->copy_from( $iter_ini );
test\step( $iter->go_to_tag(100), "finding tag 100 failed" );
test\step( $iter->go_to_tag(101), "finding tag 101 failed" );
//echo " tag is " . $iter->tag() . " \n";
test\step( $iter->tag() == 101, "Iter shall stop on next element after eos" );
$iter->copy_from( $iter_ini );
test\step( $iter->go_to_tag(100), "finding tag 100 failed" );
test\step( $iter->enter_scope(), "entering into scope failed ");
test\step( ! $iter->go_to_tag(101), "finding tag 101 must fail" );
test\step( ! $iter->go_to_tag(101), "finding tag 101 must fail second time too" );
test\step( $iter->is_end(), "iter must be end");
test\step( ! $iter->is_eof(), "iter must not be end of file");
test\step( ! $iter->next(), "iter next must fail" );
test\step( $iter->exit_scope(), "iter exit scope must pass");
test\step( $iter->next(), "iter next must pass" );
test\step( $iter->tag() == 101, "the iterated tag is not 101");

echo "   -- looking for stuffing in subscope\n";
$iter->copy_from( $iter_ini );
//test\step( $iter->go_to_tag(100), "finding tag 100 failed" );
test\step( $iter->enter_scope(100), "entering into next scope 100 failed ");
test\step( ! $iter->go_to_stuffing(), "finding stuffing must fail" );
test\step( $iter->is_end(), "iter must be end");
test\step( ! $iter->is_eof(), "iter must not be end of file");
test\step( ! $iter->next(), "iter next must fail" );
test\step( $iter->exit_scope(), "iter exit scope must pass");
test\step( $iter->next(), "iter next must pass" );
test\step( $iter->tag() == 101, "the iterated tag is not 101");

echo " ## Various \n";
echo "   -- decoding over buffer edge\n";
$buffer_over = "\x0a\x14\x00\x00\x00";
$over = new tauschema\Iterator($buffer_over);
test\step( ! $over->next(), "1; over the edge decode must fail" );
// throws test\step( $over->tag() == -1, "1; tag must be -1" );
$buffer_over = "\x0a\x04\x00\x00\x00\x07";
$over = new tauschema\Iterator($buffer_over);
test\step( $over->next(), "2; the first item shall be ok" );
test\step( $over->tag() == 2, "2; tag must be 2" );
test\step( ! $over->next(), "2; over the edge decode must fail" );
// throws test\step( $over->tag() == -1, "2; tag must be -1" );



test\summary();

?>