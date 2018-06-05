--TEST--
InspectorInstruction getExtendedValue
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction as Instruction;

$func = function($thing) {
	foreach ($thing as $k => $v) {
		
	}

	$thing["dim"] += 2;
};

$inspector = new InspectorFunction($func);

$opline = $inspector->findFirstInstruction(Instruction::ZEND_FE_FETCH_R);
$end = $inspector->findFirstInstruction(Instruction::ZEND_FE_FREE, $opline->getOffset());

if ($opline->getOffset() + $opline->getExtendedValue() ==  $end->getOffset()) {
	echo "OK\n";
}

$opline = $inspector->findFirstInstruction(Instruction::ZEND_ASSIGN_ADD, $end->getOffset());

if ($opline->getExtendedValue() == Instruction::ZEND_ASSIGN_DIM) {
	echo "OK\n";
}
?>
--EXPECT--
OK
OK
