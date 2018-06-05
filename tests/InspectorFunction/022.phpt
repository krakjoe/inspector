--TEST--
InspectorFunction purge keep closures
--FILE--
<?php
use Inspector\InspectorFunction;

$closure = function(){
	echo "OK";
};

InspectorFunction::purge();

$closure();
?>
--EXPECT--
OK
