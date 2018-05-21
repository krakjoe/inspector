--TEST--
InspectorInstruction getExtendedValue ASSIGN BW AND
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;
use Inspector\InspectorFile;

$function = function($bw) {
	$assign["dim"] &= $bw;
};

$inspector = new InspectorFunction($function);

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_ASSIGN_BW_AND);

if ($opline->getExtendedValue() == InspectorInstruction::ZEND_ASSIGN_DIM) {
	echo "OK";
}
?>
--EXPECT--
OK

