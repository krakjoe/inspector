--TEST--
InspectorOperand getNumber JMP ANON INHERITED
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;
use Inspector\InspectorOperand;

$function = function($a) {
	new class($a) extends super {};
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_DECLARE_ANON_INHERITED_CLASS);

$op1 = $opline->getOperand(InspectorOperand::OP1);

if ($op1->isJumpTarget() && $op1->getNumber() == 3) {
	echo "OK";
} else {
	var_dump($op1->getNumber(), $opline->getOffset());
}
?>
--EXPECT--
OK
