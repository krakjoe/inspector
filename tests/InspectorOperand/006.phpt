--TEST--
InspectorOperand getValue constant
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;
use Inspector\InspectorOperand;

function foo($b) {
	1 + $b;
};

$inspector = new InspectorFunction("foo");

$opline = $inspector->getInstruction(1);

$op1 = $opline->getOperand(InspectorOperand::OP1);

if ($op1->getValue() == 1) {
	echo "OK";
}
?>
--EXPECT--
OK
