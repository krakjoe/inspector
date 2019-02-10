--TEST--
InspectorFunction::getInstruction invalid parameters
--FILE--
<?php
use Inspector\InspectorFunction;

function foo(){}

$inspector = new InspectorFunction("foo");

try {
	$inspector->getInstruction("string");
} catch (TypeError $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
