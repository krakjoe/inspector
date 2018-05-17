--TEST--
InspectorClass extends \ReflectionClass
--FILE--
<?php
use Inspector\InspectorClass;

$inspector = 
	new InspectorClass(InspectorClass::class);

if ($inspector instanceof \ReflectionClass) {
	echo "OK";
}
?>
--EXPECT--
OK
