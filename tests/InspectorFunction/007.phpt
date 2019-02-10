--TEST--
InspectorFunction::getInstruction out of bounds
--FILE--
<?php
use Inspector\InspectorFunction;

function foo(){}

$inspector = new InspectorFunction("foo");

try {
	$inspector->getInstruction(-2);
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
