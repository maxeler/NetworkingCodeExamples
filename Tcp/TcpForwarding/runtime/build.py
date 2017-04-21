#!/usr/bin/env python

import sys

try:
	from RuntimeBuilder import *
	from Sim import *
except ImportError, e:
	print "Couldn't find project-utils modules."
	sys.exit(1)

dirs = [os.environ['MAXTCPFPDIR'], os.environ['MAXUDPFPDIR']]

MAXFILES = ['TcpForwarding.max']
sources = ['tcpforwarding.c']
target = 'tcpforwarding'
cflags = ['-I%s/include' % dir for dir in dirs]
ldflags = (['-L%s/lib' % dir for dir in dirs] +
           ['-lmaxtcp', '-lmaxudp', '-lslicnet'])

simNetConfig = [
	{
		'NAME' : 'QSFP_TOP_10G_PORT1',
		'DFE': '172.16.50.1',
		'TAP': '172.16.50.10',
		'NETMASK' : '255.255.255.0'
	},
	{
		'NAME' : 'QSFP_BOT_10G_PORT1',
		'DFE': '172.16.60.1',
		'TAP': '172.16.60.10',
		'NETMASK' : '255.255.255.0'
	}
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
	s.start()

def run_sim():
	build()
	s.start()
	e.execCommand([ "./" + target,
		simNetConfig[0]['DFE'],
		simNetConfig[1]['DFE'],
		"225.0.0.37",
		simNetConfig[1]['TAP'],
		simNetConfig[1]['TAP']])
	e.wait()
#	s.stop()
	

def maxdebug():
	s.maxdebug(MAXFILES)

if __name__ == '__main__':
	fabricate.main()
