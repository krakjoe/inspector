--TEST--
InspectorFile
--FILE--
<?php
use Inspector\InspectorFile;
use Inspector\InspectorInstruction as Instruction;
use Inspector\InspectorBreakPoint as BreakPoint;
use Inspector\InspectorFrame as Frame;

$inspector = new InspectorFile(__FILE__);

$opline = $inspector->findFirstInstruction(Instruction::ZEND_ECHO);

$hook = new class($opline) extends BreakPoint {

	public function hit(Frame $frame) {
		echo "OK\n";
	}

	public $hits;
};

echo "OK\n";
?>
--EXPECT--
OK
OK

