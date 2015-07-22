#!/usr/bin/python

import os
import sys

try:
	from fabricate import *
except ImportError, e:
	print "Couldn't find the fabricate module."
	sys.exit(1)

sources = ['sender.c']
target = 'sender'
includes = []

cflags = ['-O2', '-std=gnu99'] + includes + getMaxCompilerInc() 

def build():
	compile()
	link()

def compile():
	slicCompile()
	for source in sources:
		run('gcc', cflags, '-c', source, '-o', source.replace('.c', '.o'))

def link():
	objects = [s.replace('.c', '.o') for s in sources]
	run('gcc', objects, getLinkerLibs(), '-o', target, '-rdynamic')

if __name__ == '__main__':
	main()
