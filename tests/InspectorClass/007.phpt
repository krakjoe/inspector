--TEST--
InspectorClass::purge ignore nulls
--FILE--
<?php
use Inspector\InspectorClass;
use Inspector\InspectorMethod;

class Test {

}

if (class_exists(Test::class)) {
	InspectorClass::purge([null]);

	if (!class_exists(Test::class)) {
		echo "OK";
	}
}
?>
--EXPECT--
OK

