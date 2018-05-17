--TEST--
InspectorOperand getWhich
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

if ($op1->getWhich() == InspectorOpline::OP1 &&
    $op2->getWhich() == InspectorOpline::OP2) {
	echo "OK";
}

?>
--EXPECT--
OK
