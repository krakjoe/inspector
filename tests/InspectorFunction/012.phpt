--TEST--
InspectorFunction::findFirstInstruction negative
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

function foo($a, $b){
	return $a + $b;
}

$inspector = new InspectorFunction("foo");

$first = $inspector->findFirstInstruction(InspectorInstruction::ZEND_ADD, -3);

if ($first->getOpcode() == InspectorInstruction::ZEND_ADD) {
	echo "OK";
}
?>
--EXPECT--
OK
