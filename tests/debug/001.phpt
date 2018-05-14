--TEST--
Debug tests (basics)
--SKIPIF--
<?php if (!extension_loaded("inspector")) print "skip"; ?>
--INI--
opcache.optimization_level=0
opcache.enable_cli=0
error_reporting=0
--FILE--
<?php 
require_once(sprintf("%s/../inspector.inc", __DIR__));

use Inspector\Closure;
use Inspector\BreakPoint;
use Inspector\Frame;
use Inspector\Opline;

$zc = function($a, $b) {
	$c = $a + $b;
	
	return $c;
};

$closure = new Closure($zc);

$break = new class($closure->getOpline(2)) extends BreakPoint {

	public function hit(Frame $frame) {

		$print = $frame->getOpline()->getType() == "ZEND_ADD";

		if ($print &&
		!$frame->getOpline()->getOperand(Opline::OP1)->isUnused() &&
		   ($name = $frame->getOpline()->getOperand(Opline::OP1)->getName())) {
			printf("%s = %s\n", 
				$name,
				$frame->getOperand(Opline::OP1));
		}

		if ($print &&
		!$frame->getOpline()->getOperand(Opline::OP2)->isUnused() &&
		   ($name = $frame->getOpline()->getOperand(Opline::OP2)->getName())) {
			printf("%s = %s\n", 
				$name,
				$frame->getOperand(Opline::OP2));
		}
		
		$next = $frame->getOpline()->getNext();

		if ($next && !$next->getBreakPoint()) {
			$this->breaks[] = new self($next);
		}

		$this->disable();
	}
};

var_dump($zc(10, 20)); # instrumented

var_dump($zc(20, 30)); # not instrumented

$break->enable(); # enable breakpoint on entry

var_dump($zc(30, 40)); # instrumented
?>
--EXPECTF--
a = 10
b = 20
int(30)
int(50)
a = 30
b = 40
int(70)
