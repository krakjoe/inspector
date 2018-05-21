--TEST--
InspectorOperand getNumber JMP ANON
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;
use Inspector\InspectorOperand;

$function = function($a) {
	new class($a) {};
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_DECLARE_ANON_CLASS);

$op1 = $opline->getOperand(InspectorOperand::OP1);

if ($op1->isJumpTarget() && $op1->getNumber() >= 1) {
	echo "OK";
}
?>
--EXPECT--
OK
