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

		printf("hit\n");
		
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
--EXPECT--
hit
hit
hit
int(30)
int(50)
hit
hit
hit
int(70)
