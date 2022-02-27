<?php

include_once '../src/tauschema_check.php';
include_once 'test_helpers.php';
include_once 'device_info_schema.php';

$bufsize = 100;

echo " ## Producing message for flaterator to iterate \n";

$iter = new tauschema\Iterator( $bufsize );

//test\step( , __LINE__ );

test\step( ! $iter->next() );
test\step( $iter->write_scope( 1 ) );
test\step( $iter->enter_scope() );
test\step( ! $iter->next() );
test\step( $iter->write( 8, 100, "UINT-32") );
test\step( ! $iter->next() );
test\step( $iter->write_scope( 3 ) );
test\step( $iter->enter_scope() );
test\step( ! $iter->next() );
test\step( $iter->write( 2, 0, "UINT-32") );
test\step( ! $iter->next(), __LINE__ );
test\step( $iter->write( 1, "thisisablob", "BLOB") );
test\step( ! $iter->next() );
test\step( $iter->write_end() );
test\step( $iter->exit_scope() );
test\step( ! $iter->next() );
test\step( $iter->write_end() );
test\step( $iter->exit_scope() );
test\step( ! $iter->next() );
//echo bin2hex( substr( $iter->message(), 0, 100) ) . "\n";
test\bincomp( $iter->message(), "\x05\x22\x04\x64\x00\x00\x00\x0d\x0a\x04\x00\x00\x00\x00\x06\x0b\x74\x68\x69\x73\x69\x73\x61\x62\x6c\x6f\x62\x03\x03\x07" );

echo " ## Flaterator tests \n";

echo "   -- Running the method next \n";
$fl = new tauschema\Flaterator( tauschema_flattree\device_info::$schema, $iter->message() );
$fl->next();
test\step( $fl->tag_s() == "info" );
$fl->next();
test\step( $fl->tag_s() == "" );

echo "   -- Testing reset \n";
$fl->reset();
//var_dump( $fl );
$fl->next();
//echo "tag_s=" . $fl->tag_s() . "\n";
test\step( $fl->tag_s() == "info" );

echo "   -- Testing clone \n";
$fc = $fl->clone();
$fc->next();
//echo "tag_s=" . $fc->tag_s() . "\n";
test\step( $fc->tag_s() == "msglen" );
test\step( $fl->tag_s() == "info" );
$fc->next();
test\step( $fc->tag_s() == "serial" );
$fc->next();
test\step( $fc->tag_s() == "" );
$fc->next();
test\step( $fc->tag_s() == "" );

echo "   -- Testing go_to \n"; 
$fc = $fl->clone();
test\step( $fc->go_to("serial")->tag_s() == "serial" );
//echo "tag_s=" . $fc->tag_s() . "\n";
test\step( $fc->go_to("msglen")->tag_s() == "" );
$fc = $fl->clone();
test\step( $fc->go_to("msglen")->next()->tag_s() == "serial" );
test\step( $fc->go_to("data")->tag_s() == "data" );

echo "   -- testing read with no argument \n";
test\step( $fc->read() == "thisisablob" );


echo "   -- Testing read with string argument \n";
test\step( $fl->read("msglen") == 100 );
$fc = $fl->clone();
test\step( $fc->read("msglen") == 100 );
//echo "tag_s=" . $fc->tag_s() . "\n";
$fc->go_to("serial");
test\step( $fc->read("orig") == 0 );
test\step( $fc->read("data") == "thisisablob" );
test\step( $fc->clone()->read("data") == "thisisablob" );

echo "   -- Testing read with scoped string argument \n";
test\step( $fl->read("serial.orig") == 0 );
test\step( $fl->read("serial.data") == "thisisablob" );
test\step( $fl->clone()->read("serial.data") == "thisisablob" );

echo "   -- Testing overwriting of a value \n";
$fl->reset();
test\step( $fl->write( 200, "info.msglen" ) );
test\step( $fl->read("info.msglen") == 200 );

//echo bin2hex( substr( $iter->message(), 0, 100) ) . "\n";
echo "   -- Testing of adding a structure at the end \n";
$fl->reset();
test\step( $fl->write( array( "msglen"=> 140, "schurl"=>array("orig"=>10,"data"=>"http://www.tauria.ee")), "info" ) );
test\step( $fl->next()->tag_s() == "info" );
test\step( $fl->next()->tag_s() == "info" );
//echo bin2hex( substr( $iter->message(), 0, 100) ) . "\n";
test\step( $fl->read("msglen") == 140 );
test\step( $fl->read("schurl.orig") == 10 );
test\step( $fl->read("schurl.data") == "http://www.tauria.ee" );

test\summary();

?>
