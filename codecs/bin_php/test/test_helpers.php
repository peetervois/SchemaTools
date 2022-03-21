<?php
namespace test;

$count_tests = 0;
$count_errors = 0;

function step(bool $condition, string $message = Null, $applybacktrace = true)
{
    global $count_tests, $count_errors;

    if (is_null($message)) {
        $message = "";
    }

    if ($applybacktrace) {
        $dbg = debug_backtrace();
        $message .= " on line " . $dbg[0]["line"];
    }

    $count_tests += 1;
    if ($condition == true)
        return;
    $count_errors += 1;
    echo ("error: " . $message . "\n");
}

function bincomp(string $b1, string $b2, string $message = "binaries do not match !")
{
    $l1 = strlen($b1);
    $l2 = strlen($b2);
    $len = $l1 < $l2 ? $l1 : $l2;
    $dbg = debug_backtrace();
    for ($i = 0; $i < $len; $i ++) {
        if ($b1[$i] != $b2[$i]) {
            step(false, "at $i " . $message . " on line " . $dbg[0]["line"], false);
            return;
        }
    }
    step(true, $message ." on line " . $dbg[0]["line"], false );
}

function summary()
{
    global $count_tests, $count_errors;

    echo "\n\n";
    echo "Number of tests performed: " . $count_tests . " \n";
    echo "   Number of tests failed: " . $count_errors . " \n";
    echo "\n\n";
}

?>