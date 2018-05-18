--TEST--
InspectorFunction::findFirstInstruction negative
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = new InspectorFunction(function($a, $b){
	return $a + $b;
});

$first = $inspector->findFirstInstruction(InspectorInstruction::ZEND_ADD, -3);

if ($first->getOpcode() == InspectorInstruction::ZEND_ADD) {
	echo "OK";
}
?>
--EXPECT--
OK
