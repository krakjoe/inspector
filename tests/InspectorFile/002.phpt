--TEST--
InspectorFile pending source
--FILE--
<?php
use Inspector\InspectorFile;
use Inspector\InspectorInstruction as Instruction;
use Inspector\InspectorBreakPoint as BreakPoint;
use Inspector\InspectorFrame as Frame;

$pending = sprintf("%s/pending.inc", __DIR__);

$inspector = new class($pending) extends InspectorFile {
	public function onResolve() {
		$opline = 
			$this->findFirstInstruction(Instruction::ZEND_ECHO);

		$this->hook = new class($opline) extends BreakPoint {
			public function hit(Frame $frame) {
				echo "OK\n";
			}
		};
	}
};

include($pending);
?>
--EXPECT--
OK
Hello World

