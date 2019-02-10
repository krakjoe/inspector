--TEST--
InspectorInstruction getFunction
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

function foo($a, $b) {}

$inspector = 
	new InspectorFunction("foo");

$opline = 
	$inspector->getInstruction();

if ($opline->getFunction() instanceof InspectorFunction) {
	echo "OK";
}
?>
--EXPECT--
OK

