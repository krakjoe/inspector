--TEST--
InspectorFunction::getInstruction out of bounds
--FILE--
<?php
use Inspector\InspectorFunction;

$inspector = new InspectorFunction(function(){});

try {
	$inspector->getInstruction(-2);
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
