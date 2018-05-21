--TEST--
InspectorOperand isJumpTarget ANON INHERITED
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;

$function = function() {
	new class() extends other {};
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_DECLARE_ANON_INHERITED_CLASS);

if ($opline->getOperand(InspectorOperand::OP1)->isJumpTarget()) {
	echo "OK";
}
?>
--EXPECT--
OK
