--TEST--
InspectorOperand isJumpTarget ANON
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;

$function = function() {
	new class() {};
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_DECLARE_ANON_CLASS);

if ($opline->getOperand(InspectorOperand::OP1)->isJumpTarget()) {
	echo "OK";
}
?>
--EXPECT--
OK
