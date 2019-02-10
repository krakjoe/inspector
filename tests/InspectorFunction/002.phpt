--TEST--
InspectorClass::getMethod
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

$inspector = $inspector->getMethod("method");

if ($inspector instanceof InspectorMethod) {
	echo "OK";
}
?>
--EXPECT--
OK
