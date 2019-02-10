--TEST--
InspectorFrame getVariable out of bounds
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

function foo($a, $b) {
	$a + $b;
}

$inspector = new InspectorFunction("foo");

$opline = $inspector->getInstruction(2);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		if (!$frame->getVariable(42)) {
			echo "OK";
		}
	}
};

foo(1, 2);
?>
--EXPECT--
OK

