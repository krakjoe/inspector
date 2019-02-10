--TEST--
InspectorFunction::getInstruction returns InspectorInstruction
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

function foo(){};

$inspector = 
	new InspectorFunction("foo");

if ($inspector->getInstruction() instanceof InspectorInstruction) {
	echo "OK";
}
?>
--EXPECT--
OK
