--TEST--
InspectorInstruction getFunction
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = 
	new InspectorFunction(function($a, $b) {});

$opline = 
	$inspector->getInstruction();

if ($opline->getFunction() instanceof InspectorFunction) {
	echo "OK";
}
?>
--EXPECT--
OK

