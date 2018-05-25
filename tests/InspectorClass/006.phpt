--TEST--
InspectorClass::purge filter
--FILE--
<?php
use Inspector\InspectorClass;
use Inspector\InspectorMethod;

class Test {

}

class Other {

}

if (class_exists(Test::class)) {
	InspectorClass::purge(["Other"]);

	if (class_exists(Other::class) && !class_exists(Test::class)) {
		echo "OK";
	}
}
?>
--EXPECT--
OK

