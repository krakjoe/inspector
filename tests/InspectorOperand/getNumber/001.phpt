--TEST--
InspectorOperand getNumber cvs
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$function = function($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getOpline(2);

$op1 = $opline->getOperand(InspectorOpline::OP1);
$op2 = $opline->getOperand(InspectorOpline::OP2);

if ($op1->isCompiledVariable() &&
    $op2->isCompiledVariable() &&
    $op1->getNumber() == 0 &&
    $op2->getNumber() == 1) {
	echo "OK";
}
?>
--EXPECT--
OK
