--TEST--
InspectorOperand getNumber constant
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;
use Inspector\InspectorOperand;

$function = function($b) {
	1 + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getInstruction(1);

$op1 = $opline->getOperand(InspectorOperand::OP1);

if ($op1->isConstant() &&
    $op1->getNumber() == 0) {
	echo "OK";
}
?>
--EXPECT--
OK
