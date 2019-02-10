--TEST--
InspectorMethod extends \ReflectionMethod
--FILE--
<?php
use Inspector\InspectorClass;
use Inspector\InspectorMethod;

$inspector = 
	new InspectorMethod(InspectorClass::class, "getMethod");

if ($inspector->getName() == "getMethod") {
	echo "OK";
}
?>
--EXPECT--
OK
