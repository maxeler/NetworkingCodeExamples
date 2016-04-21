#!/usr/bin/env python

import sys

try:
	from RuntimeBuilder import *
	from Sim import *
except ImportError, e:
	print "Couldn't find project-utils modules."
	sys.exit(1)

MAXFILES = ['TcpForwarding.max']
sources = ['tcpforwarding.c']
application = 'tcpforwarding'
includes = []
my_cflags = []
my_ldflags = []


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
	e.execCommand([ "./" + application, "172.16.50.1", "172.16.60.1", "225.0.0.37", "172.16.60.10", "172.16.60.10"])
	e.wait()
#	s.stop()
	

def maxdebug():
	s.maxdebug(MAXFILES)

if __name__ == '__main__':
	fabricate.main()
