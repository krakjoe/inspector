Inspector
========
*Inspector allows you to programmatically inspect PHP code*

There are many tools that dump or print opcodes, but none of these tools allow programmatic interaction.

Inspector provides an OO API to walk through the opcodes of a given function, accessing their operands.

API
===

The following API is provided:

```php
namespace Inspector 
{
	class Inspector {
		public function __construct(string function);
		public function __construct(string scope, string method);
		public function __construct(Closure closure);
	}

	class Node {
		const OP1;
		const OP2;
		const RESULT;

		public function getType() : string;
		public function getOperand(int which) : Operand;
		public function getExtendedValue() : mixed;
	}

	class Operand {
		public function isUnused() : bool;
		public function isExtendedTypeUnused() : bool;
		public function isCompiledVariable() : bool;
		public function isTemporaryVariable() : bool;
		public function isVariable() : bool;
		public function isConstant() : bool;
		public function isJumpTarget() : bool;
		public function getWhich() : int;
		public function getValue() : mixed;
		public function getName() : string;
		public function getNumber() : int;
	}
}
```

Example
======
*Other people are better at this than I am ...*

Making stuff look nice is not my forte, all the same, here is some simple code using Inspector:

```php
use Inspector\Inspector;
use Inspector\Node;
use Inspector\Operand;

function printConstant($mixed) {
	if (strlen($mixed) > 8) {
		printf(
			"%s...\t", substr(str_replace(
			"\n",
			"\\n",
			$mixed
		), 0, 8));
	} else printf("%s\t", $mixed ?: "null");
}

function printOperand(Operand $op) {
	if ($op->isConstant()) {
		printConstant($op->getValue());
	} else if ($op->isCompiledVariable()) {
		printf("\$%s\t", $op->getName());
	} else if ($op->isTemporaryVariable()) {
		printf("T%d\t", $op->getNumber());
	} else if($op->isVariable()) {
		printf("V%d\t", $op->getNumber());
	} else printf("-\t");
}

function printInspector(Inspector $inspector) {
	printf("OPCODE\t\tOP1\tOP2\tRESULT\n");
	foreach ($inspector as $opline) {
		printf("%s\t", $opline->getType());
		printOperand($opline->getOperand(NODE::OP1));
		printOperand($opline->getOperand(NODE::OP2));
		printOperand($opline->getOperand(NODE::RESULT));
		printf("\n");
	}
}

printInspector(new Inspector(function($a, $b){
	return $a + $b;
}));
```

Which will yield:

```
OPCODE          OP1     OP2     RESULT
ZEND_RECV       -       -       $a
ZEND_RECV       -       -       $b
ZEND_ADD        $a      $b      T1
ZEND_RETURN     T1      -       -
ZEND_RETURN     null    -       -
```

TODO
====

 * tests

**This extension is experimental, and requires PHP7**

