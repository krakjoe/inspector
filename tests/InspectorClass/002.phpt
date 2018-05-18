--TEST--
InspectorClass::getMethod returns InspectorMethod
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

if ($inspector->getMethod("method") instanceof InspectorMethod) {
	echo "OK";
}
?>
--EXPECT--
OK
