--TEST--
InspectorOpline getOperand
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;
use Inspector\InspectorOperand;

$inspector = 
	new InspectorFunction(function($a) {});

if ($inspector->getOpline()->getOperand(InspectorOpline::OP1) instanceof InspectorOperand) {
	echo "OK\n";
}

if ($inspector->getOpline()->getOperand(InspectorOpline::OP2) instanceof InspectorOperand) {
	echo "OK\n";
}

if ($inspector->getOpline()->getOperand(InspectorOpline::RESULT) instanceof InspectorOperand) {
	echo "OK\n";
}

try {
	$inspector->getOpline()->getOperand(42);
} catch (ReflectionException $ex) {
	echo "OK";
}

?>
--EXPECT--
OK
OK
OK
OK
