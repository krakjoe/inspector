--TEST--
InspectorInstruction getOperand
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;

$inspector = 
	new InspectorFunction(function($a) {});

if ($inspector->getInstruction()->getOperand(InspectorOperand::OP1) instanceof InspectorOperand) {
	echo "OK\n";
}

if ($inspector->getInstruction()->getOperand(InspectorOperand::OP2) instanceof InspectorOperand) {
	echo "OK\n";
}

if ($inspector->getInstruction()->getOperand(InspectorOperand::RESULT) instanceof InspectorOperand) {
	echo "OK\n";
}

try {
	$inspector->getInstruction()->getOperand(42);
} catch (ReflectionException $ex) {
	echo "OK";
}

?>
--EXPECT--
OK
OK
OK
OK
