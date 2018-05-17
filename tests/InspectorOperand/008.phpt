--TEST--
InspectorOperand getName
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

if ($op1->getName() == "a") {
	echo "OK";
}
?>
--EXPECT--
OK
