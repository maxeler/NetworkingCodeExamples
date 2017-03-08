#!/usr/bin/env python

import sys

try:
	from RuntimeBuilder import *
	from Sim import *
except ImportError, e:
	print "Couldn't find project-utils modules."
	sys.exit(1)

MAXFILES = ['TradingStartegyExample.max']
sources = ['tradingstartegyexample.c']
target = 'tradingstartegyexample'
includes = []
cflags = []
ldflags = []

simNetConfig = [
				{ 'NAME' : 'QSFP_TOP_10G_PORT1', 'DFE': '172.16.50.1', 'TAP': '172.16.50.10', 'NETMASK' : '255.255.255.0' },
				{ 'NAME' : 'QSFP_BOT_10G_PORT1', 'DFE': '172.16.60.1', 'TAP': '172.16.60.10', 'NETMASK' : '255.255.255.0' }
			]


b = MaxRuntimeBuilder(maxfiles=MAXFILES)
s = MaxCompilerSim(dfeModel="ISCA")
e = Executor(logPrefix="[%s] " % (target))

def build():
	compile()
	link()

def compile():
	b.slicCompile()
	b.compile(sources, extra_cflags=cflags)

def link():
	b.link(sources, target, extra_ldflags=ldflags)

def clean():
	b.clean()

def start_sim():
	s.start(netConfig=simNetConfig)

def stop_sim():
	s.stop()

def restart_sim():
	stop_sim()
	start_sim()

def run_sim():
	build()
	restart_sim()
	e.execCommand([ "./" + target, './md.dat' ])
	e.wait()
#	s.stop()
	

def maxdebug():
	s.maxdebug(MAXFILES)

if __name__ == '__main__':
	fabricate.main()
