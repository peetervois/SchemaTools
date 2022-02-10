<?php 

namespace test;

$count_tests = 0;
$count_errors = 0;

function step( bool $condition, string $message )
{
    global $count_tests, $count_errors;
    
    $count_tests += 1;
    if( $condition == true ) return;
    $count_errors += 1;
    echo( "error: " . $message . "\n" );
}

function bincomp( string $b1, string $b2, string $message = "binaries do not match !" )
{
    $l1 = strlen( $b1 );
    $l2 = strlen( $b2 );
    $len = $l1 < $l2 ? $l1 : $l2;
    for( $i=0; $i<$len; $i++ )
    {
        if( $b1[$i] != $b2[$i] )
        {
            step( false, "at $i " . $message );
            return;
        }
    }
    step( true, $message );
}

function summary()
{
    global $count_tests, $count_errors;
    
    echo "\n\n";
    echo "Number of tests performed: ".$count_tests." \n";
    echo "   Number of tests failed: ".$count_errors." \n";
    echo "\n\n";
}

?>