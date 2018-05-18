--TEST--
InspectorInstruction getNext
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = 
	new InspectorFunction(function($a) {});

if ($inspector->getInstruction()->getNext() instanceof InspectorInstruction) {
	echo "OK";
}
?>
--EXPECT--
OK
