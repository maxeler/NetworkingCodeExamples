#!/usr/bin/env python

import sys

try:
	from RuntimeBuilder import *
	from Sim import *
except ImportError, e:
	print "Couldn't find project-utils modules."
	sys.exit(1)

MAXFILES = ['Fragmenter.max']
sources = ['fragmenter.c']
target = 'fragmenter'
includes = []
my_cflags = []
my_ldflags = []


b = MaxRuntimeBuilder(maxfiles=MAXFILES)
s = MaxCompilerSim(dfeModel="ISCA")
e = Executor(logPrefix="[%s] " % (target))

def build():
	compile()
	link()

def compile():
	b.slicCompile()
	b.compile(sources, extra_cflags=my_cflags)

def link():
	b.link(sources, target, extra_ldflags=my_ldflags)

def clean():
	b.clean()

def start_sim():
	s.start()

def stop_sim():
	s.stop()

def restart_sim():
	s.start()

def run_sim():
	build()
	s.start(netConfig=[])
	e.execCommand([ "./" + target ])
	e.wait()
	s.stop()
	

def maxdebug():
	s.maxdebug(MAXFILES)

if __name__ == '__main__':
	fabricate.main()
