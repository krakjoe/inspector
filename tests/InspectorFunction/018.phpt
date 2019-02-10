--TEST--
InspectorFunction::findLastInstruction seek
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

function foo($a, $b){
	return $a + $b;
}

$inspector = new InspectorFunction("foo");

$add = $inspector->findLastInstruction(
	InspectorInstruction::ZEND_ADD, $inspector->getInstructionCount());

if ($add->getOpcode() == InspectorInstruction::ZEND_ADD) {
	echo "OK";
}
?>
--EXPECT--
OK
