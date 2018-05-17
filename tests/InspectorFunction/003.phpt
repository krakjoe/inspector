--TEST--
InspectorClass::getMethod
--FILE--
<?php
use Inspector\InspectorClass;

class Test {}

$inspector = 
	new InspectorClass(Test::class);

try {
	$inspector->getMethod("method");
} catch (\ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
