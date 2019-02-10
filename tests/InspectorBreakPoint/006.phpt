--TEST--
InspectorBreakPoint hit returns refcounted
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorInstruction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

function foo($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction("foo");

$opline = $inspector->getInstruction(2);

class BreakPoint extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		return $frame;
	}
}

$break = new BreakPoint($opline);

foo(1, 2);

echo "OK";
?>
--EXPECT--
OK
