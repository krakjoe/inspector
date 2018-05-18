--TEST--
InspectorInstruction getExtendedValue
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorOperand;

$inspector = 
	new InspectorFunction(function($a, $b) {
	# 0 RECV
	# 1 RECV
	(string) $a; # 2 CAST
});

$opline = $inspector->getInstruction();

do {
	if ($opline->getOpcode() == InspectorInstruction::CAST) {
		var_dump($opline->getExtendedValue());
		break;
	}
} while ($opline = $opline->getNext());
?>
--EXPECT--
string(6) "string"

