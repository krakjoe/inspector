--TEST--
InspectorOperand getNumber
--FILE--
<?php
use Inspector\InspectorFunction;

use Inspector\InspectorOperand as Operand;
use Inspector\InspectorInstruction as Instruction;

function foo($a, $b, $c) {
	$d = $a + 10;

	foreach ($c as $k => $v) {
		$g[] = [$k=>$v];
	}

	return $d;
};

$inspector = new InspectorFunction("foo");

$opline = $inspector->findFirstInstruction(Instruction::ZEND_ADD);

$op1 = $opline->getOperand(Operand::OP1);
$op2 = $opline->getOperand(Operand::OP2);
$rv  = $opline->getOperand(Operand::RESULT);

if ($op1->getNumber() == 0 &&
    $op2->getNumber() == 0 &&
    $rv->getNumber() >= 4) {
	echo "OK\n";
}

$opline = $inspector->findFirstInstruction(Instruction::ZEND_FE_RESET_R);
$end    = $inspector->findFirstInstruction(Instruction::ZEND_FE_FREE, $opline->getOffset());

$op1 = $opline->getOperand(Operand::OP1);

if ($op1->getNumber() === 2) {
	echo "OK\n";
}

$op2 = $opline->getOperand(Operand::OP2);

if ($end->getOffset() == $op2->getNumber()) {
	echo "OK\n";
}
?>
--EXPECT--
OK
OK
OK
