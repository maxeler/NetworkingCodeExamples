#!/usr/bin/env python

import sys
import os

try:
	from RuntimeBuilder import *
	from Sim import *
except ImportError, e:
	print "Couldn't find project-utils modules."
	sys.exit(1)

MAXUDPFPDIR = Environment.require('MAXUDPFPDIR')

MAXFILES = ['UdpOneToOne.max']
sources = ['udponetoone.c']
application = 'udponetoone'
includes = []
my_cflags = ['-I%s/include' % MAXUDPFPDIR]
my_ldflags = ['-L%s/lib' % MAXUDPFPDIR, '-lmaxudp']


b = MaxRuntimeBuilder(maxfiles=MAXFILES)
s = MaxCompilerSim(dfeModel="ISCA")
e = Executor(logPrefix="[%s] " % (application))

def build():
	compile()
	link()

def compile():
	b.slicCompile()
	b.compile(sources, extra_cflags=my_cflags)

def link():
	b.link(sources, application, extra_ldflags=my_ldflags)

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
	s.start()
	e.execCommand([ "./" + application, "172.16.50.1", "172.16.50.10"])
	e.wait()
#	s.stop()
	

def maxdebug():
	s.maxdebug(MAXFILES)

if __name__ == '__main__':
	fabricate.main()
