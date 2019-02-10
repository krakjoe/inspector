--TEST--
InspectorInstruction getFlags
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction as Instruction;
use Inspector\InspectorOperand as Operand;

function & foo($thing) {
	if ($thing instanceof Type) {
		return $thing;
	}
}

$inspector = new InspectorFunction("foo");

$opline = $inspector->findFirstInstruction(Instruction::ZEND_RETURN_BY_REF);

$opFlags = $opline->getFlags();

if (($opFlags & Instruction::ZEND_VM_EXT_MASK) == Instruction::ZEND_VM_EXT_SRC) {
	echo "OK\n";
}

$opline = $inspector->findFirstInstruction(Instruction::ZEND_INSTANCEOF);

$op1Flags = $opline->getFlags(Operand::OP1);

if ($op1Flags & Instruction::ZEND_VM_OP_TMPVAR) {
	echo "OK\n";
}

$op2Flags = $opline->getFlags(Operand::OP2);

if ($op2Flags & Instruction::ZEND_VM_OP_CLASS_FETCH) {
	echo "OK\n";
}
?>
--EXPECT--
OK
OK
OK
