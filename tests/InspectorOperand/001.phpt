--TEST--
InspectorOperand isUnused
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;
use Inspector\InspectorOperand;

$function = function($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getInstruction(2);

if (!$opline->getOperand(InspectorOperand::OP1)->isUnused()) {
	echo "OK";
}
?>
--EXPECT--
OK
