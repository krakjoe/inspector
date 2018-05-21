--TEST--
InspectorInstruction getExtendedValue ASSIGN REF function
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;
use Inspector\InspectorFile;

$inspector = new InspectorFunction(function &() {
	return func();
});

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_RETURN_BY_REF);

if ($opline->getExtendedValue() == "function") {
	echo "OK";
}
?>
--EXPECT--
OK

