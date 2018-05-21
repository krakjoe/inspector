--TEST--
InspectorInstruction getExtendedValue ASSIGN REF
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;
use Inspector\InspectorFile;

$function = function(&$bw) {
	$bw = &$data;
};

$inspector = new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_ASSIGN_REF);

if ($opline->getExtendedValue() == "value") {
	echo "OK";
}
?>
--EXPECT--
OK

