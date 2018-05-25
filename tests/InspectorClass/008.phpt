--TEST--
InspectorClass::getMethods invalid parameter
--FILE--
<?php
use Inspector\InspectorClass;
use Inspector\InspectorMethod;

class Test {
	public function method() {
		
	}
}

$inspector = 
	new InspectorClass(Test::class);

try {
	$inspector->getMethods("string");
} catch(TypeError $ex) {
	echo "OK";
}

?>
--EXPECT--
OK
