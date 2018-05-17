--TEST--
InspectorFrame getStack
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorOpline;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$function = function($a, $b) {
	$a + $b;
};

$inspector = 
	new InspectorFunction($function);

$opline = $inspector->getOpline(3);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		var_dump($frame->getStack());
	}
};

$function(1, 2);
?>
--EXPECT--
array(2) {
  ["a"]=>
  int(1)
  ["b"]=>
  int(2)
}

