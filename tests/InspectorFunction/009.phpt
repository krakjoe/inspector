--TEST--
InspectorFunction::getEntryInstruction
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;

$inspector = new InspectorFunction(function($a, $b){
	return $a + $b;
});

$entry = $inspector->getEntryInstruction();

if ($entry->getOpcode() == InspectorInstruction::ZEND_ADD) {
	echo "OK";
}
?>
--EXPECT--
OK
