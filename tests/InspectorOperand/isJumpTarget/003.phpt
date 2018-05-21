--TEST--
InspectorOperand isJumpTarget NEW
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;

$function = function() {
	new stdClass;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_NEW);

if ($opline->getOperand(InspectorOperand::OP2)->isJumpTarget()) {
	echo "OK";
}
?>
--EXPECT--
OK
