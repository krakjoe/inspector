--TEST--
InspectorOperand getNumber t/var
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$function = function($a, $b) {
	$c = $a + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getOpline(3);

$op1 = $opline->getOperand(InspectorOpline::OP1);
$op2 = $opline->getOperand(InspectorOpline::OP2);

if ($op2->isTemporaryVariable() &&
    $op2->getNumber()) {
	echo "OK";
}
?>
--EXPECT--
OK
