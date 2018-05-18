--TEST--
InspectorInstruction getPrevious
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = 
	new InspectorFunction(function($a) {});

if (!$inspector->getInstruction()->getPrevious()) {
	echo "OK";
}
?>
--EXPECT--
OK
