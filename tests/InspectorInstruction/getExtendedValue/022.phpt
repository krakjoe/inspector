--TEST--
InspectorInstruction getExtendedValue NEW
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;
use Inspector\InspectorFile;

$inspector = new InspectorFunction(function () {
	new stdClass("one");
});

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_NEW);

if ($opline->getExtendedValue() == 1) {
	echo "OK";
}
?>
--EXPECT--
OK

