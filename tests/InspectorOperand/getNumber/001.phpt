--TEST--
InspectorOperand getNumber cvs
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;
use Inspector\InspectorOperand;

$function = function($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getInstruction(2);

$op1 = $opline->getOperand(InspectorOperand::OP1);
$op2 = $opline->getOperand(InspectorOperand::OP2);

if ($op1->isCompiledVariable() &&
    $op2->isCompiledVariable() &&
    $op1->getNumber() == 0 &&
    $op2->getNumber() == 1) {
	echo "OK";
}
?>
--EXPECT--
OK
