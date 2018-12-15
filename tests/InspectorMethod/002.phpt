--TEST--
InspectorMethod::getDeclaringClass returns InspectorClass
--FILE--
<?php
use Inspector\InspectorClass;
use Inspector\InspectorMethod;

class Test {
	public function method(){}
}

$inspector = 
	new InspectorMethod(Test::class, "method");

$inspector = $inspector->getDeclaringClass();

if ($inspector instanceof InspectorClass) {
	echo "OK";
}
?>
--EXPECT--
OK
