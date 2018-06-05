--TEST--
InspectorFunction purge filter
--FILE--
<?php
use Inspector\InspectorFunction;

function test() {}

InspectorFunction::purge(["test"]);

if (function_exists("test")) {
	echo "OK";
}
?>
--EXPECT--
OK
