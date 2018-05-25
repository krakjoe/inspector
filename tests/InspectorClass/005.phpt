--TEST--
InspectorClass::purge filter
--FILE--
<?php
use Inspector\InspectorClass;

class Test {

}

if (class_exists(Test::class)) {
	InspectorClass::purge(["Test"]);

	if (class_exists(Test::class)) {
		echo "OK";
	}
}
?>
--EXPECT--
OK

