--TEST--
InspectorInstruction getExtendedValue ASSIGN DIV
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;
use Inspector\InspectorFile;

$function = function($div) {
	$assign["dim"] /= $div;
};

$inspector = new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_ASSIGN_DIV);

if ($opline->getExtendedValue() == InspectorInstruction::ZEND_ASSIGN_DIM) {
	echo "OK";
}
?>
--EXPECT--
OK

