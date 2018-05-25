--TEST--
InspectorClass::purge 
--FILE--
<?php
use Inspector\InspectorClass;

class Test {

}

if (class_exists(Test::class)) {
	InspectorClass::purge();

	if (!class_exists(Test::class)) {
		echo "OK";
	}
}
?>
--EXPECT--
OK

