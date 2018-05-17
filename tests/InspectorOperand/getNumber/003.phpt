--TEST--
InspectorOperand getNumber variable
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$function = function($a, $b) {
	return $c = $a + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getOpline(4);

$op1 = $opline->getOperand(InspectorOpline::OP1);

if ($op1->isVariable() &&
    $op1->getNumber() == 3) {
	echo "OK";
}
?>
--EXPECT--
OK
