--TEST--
InspectorInstruction getPrevious
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

function foo($a) {}

$inspector = new InspectorFunction("foo");

if ($inspector->getInstruction()->getNext()->getPrevious() instanceof InspectorInstruction) {
	echo "OK";
}
?>
--EXPECT--
OK
