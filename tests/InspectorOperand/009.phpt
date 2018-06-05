--TEST--
InspectorOperand isJumpTarget
--FILE--
<?php
use Inspector\InspectorFunction;

use Inspector\InspectorOperand as Operand;
use Inspector\InspectorInstruction as Instruction;

$function = function($thing) {
	foreach ($thing as $k => $v) {}
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(Instruction::ZEND_FE_RESET_R);

$op2  = $opline->getOperand(Operand::OP2);

if ($op2->isJumpTarget()) {
	echo "OK\n";
}

$op1 = $opline->getOperand(Operand::OP1);

if (!$op1->isJumpTarget()) {
	echo "OK\n";
}
?>
--EXPECT--
OK
OK
