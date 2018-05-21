--TEST--
InspectorInstruction getExtendedValue FETCH R local
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;
use Inspector\InspectorFile;

$inspector = new InspectorFunction(function () {
	@$thing;
});

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_FETCH_R);

if ($opline->getExtendedValue() == "local") {
	echo "OK";
}
?>
--EXPECT--
OK

