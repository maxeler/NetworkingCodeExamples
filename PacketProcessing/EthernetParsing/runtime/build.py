#!/usr/bin/env python

import sys

try:
	from RuntimeBuilder import *
	from Sim import *
except ImportError, e:
	print "Couldn't find project-utils modules."
	sys.exit(1)

MAXFILES = ['EthFwd.max']
sources = ['ethFwd.c']
target = 'ethFwd'
includes = []

network_config = [ 
				{ 'NAME' : 'QSFP_TOP_10G_PORT1', 'DFE': '172.16.50.1', 'TAP': '172.16.50.10', 'NETMASK' : '255.255.255.0' }, 
				{ 'NAME' : 'QSFP_BOT_10G_PORT1', 'DFE': '172.16.60.1', 'TAP': '172.16.60.10', 'NETMASK' : '255.255.255.0' }
			] 


b = MaxRuntimeBuilder(maxfiles=MAXFILES)
s = MaxCompilerSim(dfeModel="ISCA")
e = Executor(logPrefix="[%s] " % (target))

def build():
	compile()
	link()
	subprocess.call(['test/build.py'])

def compile():
	b.slicCompile()
	b.compile(sources)

def link():
	b.link(sources, target)

def clean():
	b.clean()
	subprocess.call(['test/build.py', 'clean'])

def start_sim():
	s.start(netConfig=network_config)

def stop_sim():
	s.stop()

def restart_sim():
	start_sim()

def run_sim():
	build()
	start_sim()
	e.execCommand([ "./" + target , network_config[0]["DFE"], network_config[1]["DFE"], network_config[1]["TAP"]])
	e.wait()
	s.stop()
	

def maxdebug():
	s.maxdebug(MAXFILES)

if __name__ == '__main__':
	fabricate.main()
