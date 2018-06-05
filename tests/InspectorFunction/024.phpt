--TEST--
InspectorFunction purge invalid filter element
--FILE--
<?php
use Inspector\InspectorFunction;

function test() {}

InspectorFunction::purge([null]);

if (!function_exists("test")) {
	echo "OK";
}
?>
--EXPECT--
OK
