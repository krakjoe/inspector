--TEST--
InspectorFile resolve returns refcounted
--FILE--
<?php
use Inspector\InspectorFile;
use Inspector\InspectorInstruction as Instruction;
use Inspector\InspectorBreakPoint as BreakPoint;
use Inspector\InspectorFrame as Frame;

$pending = sprintf("%s/pending.inc", __DIR__);

$inspector = new class($pending) extends InspectorFile {
	public function onResolve() {
		echo "OK\n";
		return $this;
	}
};

include($pending);

echo "OK";
?>
--EXPECT--
OK
Hello World
OK
