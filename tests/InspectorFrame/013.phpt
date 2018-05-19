--TEST--
InspectorFrame getThis non-object scope
--FILE--
<?php
use Inspector\InspectorInstruction;
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

function test($a) {
	return $a;
}

$in = new InspectorFunction("test");

$break = new class ($in->getEntryInstruction()) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame) {
		if (!$frame->getThis()) {
			echo "OK";
		}
	}
};

test(42);
?>
--EXPECT--
OK

