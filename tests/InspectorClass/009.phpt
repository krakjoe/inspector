--TEST--
InspectorClass::getMethods purge invalid parameter
--FILE--
<?php
use Inspector\InspectorClass;

try {
	InspectorClass::purge("string");
} catch(TypeError $ex) {
	echo "OK";
}

?>
--EXPECT--
OK
