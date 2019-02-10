--TEST--
InspectorFrame getInstruction getBreakPoint
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
		$opline = $frame->getInstruction();

		if ($opline->getBreakPoint() == $this) {
			echo "OK";
		}
	}
};

foo(1, 2);
?>
--EXPECT--
OK

