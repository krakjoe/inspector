--TEST--
InspectorOperand getNumber NEW
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;
use Inspector\InspectorOperand;

$function = function($a) {
	return new super($a);
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_NEW);

$op2 = $opline->getOperand(InspectorOperand::OP2);

if ($op2->isJumpTarget() && $op2->getNumber()) {
	echo "OK";
}
?>
--EXPECT--
OK
