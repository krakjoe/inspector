--TEST--
InspectorClass extends \ReflectionClass
--FILE--
<?php
use Inspector\InspectorClass;

$inspector = 
	new InspectorClass(InspectorClass::class);

if ($inspector->getName() == "Inspector\\InspectorClass") {
	echo "OK";
}
?>
--EXPECT--
OK
