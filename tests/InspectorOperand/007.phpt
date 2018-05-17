--TEST--
InspectorOperand getOpline
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$function = function($b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getOpline(2);

$op1 = $opline->getOperand(InspectorOpline::OP1);

if ($op1->getOpline() == $opline) {
	echo "OK";
}
?>
--EXPECT--
OK
