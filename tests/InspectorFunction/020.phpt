--TEST--
InspectorFunction onResolve/purge
--FILE--
<?php
use Inspector\InspectorFunction;
use Inspector\InspectorBreakPoint;
use Inspector\InspectorFrame;

$include = sprintf("%s/purge.inc", dirname(__FILE__));

$inspector = new class("add") extends InspectorFunction {
	public function onResolve() {
		$instruction = $this->getEntryInstruction();

		unset($this->hook);

		$this->hook = new class($instruction) extends InspectorBreakPoint {
			public function hit(InspectorFrame $frame) {
				echo "OK\n";
			}
		};
	}

	private $hook;
};

include($include); # defines add(int, int)

add(1,2);

InspectorFunction::purge([]); # deletes add(int, int)

include($include); # defines add(int, int)

add(3,4);
?>
--EXPECT--
OK
OK
