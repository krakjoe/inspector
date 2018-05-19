--TEST--
InspectorFrame getThis
--FILE--
<?php
use Inspector\InspectorInstruction;
use Inspector\InspectorMethod;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

class Test {
	public function method($a) {
		return $a;
	}
}

$in = new InspectorMethod(Test::class, "method");

$break = new class ($in->getEntryInstruction()) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame) {
		if ($frame->getThis() instanceof Test) {
			echo "OK";
		}
	}
};

$test = new Test();
$test->method(42);
?>
--EXPECT--
OK

