--TEST--
InspectorFunction mapping
--FILE--
<?php
use Inspector\InspectorFunction;

$include = sprintf("%s/021.inc", dirname(__FILE__));

$inspector = new class("map") extends InspectorFunction {
	public function onResolve() {
		/* don't need to do anything */	
		echo "OK";
	}
};

include ($include);

map([1,2,3,4], function($k, $v){}, false);
?>
--EXPECT--
OK
