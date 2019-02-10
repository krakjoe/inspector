--TEST--
InspectorFrame getParameters
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;
use Inspector\InspectorInstruction;

function foo($a, $b) {
	$a + $b;
}

$inspector = new InspectorFunction("foo");

$opline = $inspector->findFirstInstruction(InspectorInstruction::ZEND_ADD);

$break = new class($opline) extends InspectorBreakPoint {
	public function hit(InspectorFrame $frame){
		$parameters = $frame->getParameters();

		if ($parameters[0] == 1 &&
		    $parameters[1] == 2) {
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

