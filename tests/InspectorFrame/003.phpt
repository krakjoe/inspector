--TEST--
InspectorFrame getFunction
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

function foo($a, $b) {
	$a + $b;
}

$inspector = 
	new InspectorFunction("foo");

$opline = $inspector->getInstruction(2);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		if ($frame->getFunction() instanceof InspectorFunction) {
			echo "OK";
		}
	}
};

foo(1, 2);
?>
--EXPECT--
OK

