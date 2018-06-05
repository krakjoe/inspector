--TEST--
InspectorFunction purge remove unfiltered
--FILE--
<?php
use Inspector\InspectorFunction;

function test() {}

InspectorFunction::purge();

if (!function_exists("test")) {
	echo "OK";
}
?>
--EXPECT--
OK
