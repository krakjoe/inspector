--TEST--
InspectorFrame getStack
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

$opline = $inspector->getInstruction(3);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		var_dump($frame->getStack());
	}
};

foo(1, 2);
?>
--EXPECT--
array(2) {
  ["a"]=>
  int(1)
  ["b"]=>
  int(2)
}

