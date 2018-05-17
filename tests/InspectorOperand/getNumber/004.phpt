--TEST--
InspectorOperand getNumber constant
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$function = function($b) {
	1 + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getOpline(1);

$op1 = $opline->getOperand(InspectorOpline::OP1);

if ($op1->isConstant() &&
    $op1->getNumber() == 0) {
	echo "OK";
}
?>
--EXPECT--
OK
