--TEST--
InspectorOperand getInstruction
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;
use Inspector\InspectorOperand;

$function = function($b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getInstruction(2);

$op1 = $opline->getOperand(InspectorOperand::OP1);

if ($op1->getInstruction() == $opline) {
	echo "OK";
}
?>
--EXPECT--
OK
