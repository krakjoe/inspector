--TEST--
InspectorFunction::findLastInstruction seek
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = new InspectorFunction(function($a, $b){
	return $a + $b;
});

$add = $inspector->findLastInstruction(
	InspectorInstruction::ZEND_ADD, $inspector->getInstructionCount());

if ($add->getOpcode() == InspectorInstruction::ZEND_ADD) {
	echo "OK";
}
?>
--EXPECT--
OK
