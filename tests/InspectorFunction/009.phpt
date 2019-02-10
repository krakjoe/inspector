--TEST--
InspectorFunction::getEntryInstruction
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

function foo($a, $b){
	return $a + $b;
}

$inspector = new InspectorFunction("foo");

$entry = $inspector->getEntryInstruction();

if ($entry->getOpcode() == InspectorInstruction::ZEND_ADD) {
	echo "OK";
}
?>
--EXPECT--
OK
