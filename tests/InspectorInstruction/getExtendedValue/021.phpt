--TEST--
InspectorInstruction getExtendedValue FETCH R global
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;
use Inspector\InspectorFile;

$inspector = new InspectorFunction(function () {
	@$_SERVER;
});

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_FETCH_R);

if ($opline->getExtendedValue() == "global") {
	echo "OK";
}
?>
--EXPECT--
OK

