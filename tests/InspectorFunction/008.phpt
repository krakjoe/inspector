--TEST--
InspectorFunction::getEntryInstruction no entry
--FILE--
<?php
use Inspector\InspectorFunction;

function foo(){}

$inspector = new InspectorFunction("foo");

try {
	$inspector->getEntryInstruction();
} catch (ReflectionException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
