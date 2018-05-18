--TEST--
InspectorFunction::getInstruction returns InspectorInstruction
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = 
	new InspectorFunction(function(){});

if ($inspector->getInstruction() instanceof InspectorInstruction) {
	echo "OK";
}
?>
--EXPECT--
OK
