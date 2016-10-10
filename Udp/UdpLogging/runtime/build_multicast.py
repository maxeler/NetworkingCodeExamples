#!/usr/bin/python

import os
import sys

try:
	from fabricate import *
	from Executor import *
except ImportError, e:
	print "Couldn't find the fabricate module."
	sys.exit(1)



e = Executor(logPrefix="[multicast] ")


sources = ['multicast.c']
target = 'multicast'
includes = []

cflags = ['-O2', '-std=gnu99'] + includes

def build():
	compile()
	link()

def compile():
	for source in sources:
		run('gcc', cflags, '-c', source, '-o', source.replace('.c', '.o'))

def link():
	objects = [s.replace('.c', '.o') for s in sources]
	run('gcc', objects, '-o', target)

def run_multicast():
	build()
	e.execCommand([ './multicast', '172.16.50.10', '225.0.0.37'])
	e.wait()
	

def clean():
	autoclean()

if __name__ == '__main__':
	main()
