--TEST--
InspectorFrame setInstruction
--FILE--
<?php
use Inspector\InspectorFunction;

use Inspector\InspectorFrame as Frame;
use Inspector\InspectorInstruction as Instruction;
use Inspector\InspectorBreakPoint as BreakPoint;

$function = function() {
	echo "O";
	echo "FAIL";
	echo "K";
	echo "\n";
};

$inspector = new InspectorFunction($function);

$first = $inspector->findFirstInstruction(Instruction::ZEND_ECHO);

$second = $inspector->findFirstInstruction(Instruction::ZEND_ECHO, $first->getOffset() + 1);

$darkness = new class($second) extends Inspector\InspectorBreakPoint {
	public function hit(Inspector\InspectorFrame $frame) {
		$opline    = $frame->getInstruction();
		$function  = $frame->getFunction();

		$frame->setInstruction(
			$function->findFirstInstruction(
				Instruction::ZEND_ECHO, $opline->getOffset() + 1));
	}
};

$function();
?>
--EXPECT--
OK

