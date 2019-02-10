--TEST--
InspectorFrame getPrevious
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

function foo($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction("foo");

$opline = $inspector->getInstruction(2);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		if ($frame->getPrevious() instanceof InspectorFrame) {
			echo "OK";
		}
	}
};

function test($function) {
	$function(1, 2);
}

test("foo");
?>
--EXPECT--
OK

