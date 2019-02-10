--TEST--
InspectorBreakPoint hit throws
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
		throw new RuntimeException();
	}
}

$break = new BreakPoint($opline);

try {
	foo(1, 2);
} catch (RuntimeException $ex) {
	echo "OK";
}
?>
--EXPECT--
OK
