--TEST--
InspectorMethod extends \ReflectionMethod
--FILE--
<?php
use Inspector\InspectorClass;
use Inspector\InspectorMethod;

$inspector = 
	new InspectorMethod(InspectorClass::class, "getMethod");

if ($inspector instanceof \ReflectionMethod) {
	echo "OK";
}
?>
--EXPECT--
OK
