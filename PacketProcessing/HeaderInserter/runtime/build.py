#!/usr/bin/python

import os
import sys
import getpass
import subprocess

try:
	from fabricate import *
except ImportError, e:
	print "Couldn't find the fabricate module."
	sys.exit(1)

MAXOSDIR = os.environ['MAXELEROSDIR']
MAXCOMPILERDIR = os.environ['MAXCOMPILERDIR']
MAXCOMPILERNETDIR = os.environ['MAXCOMPILERNETDIR']
DFE_MODEL = 'ISCA'

MAXFILE = 'HeaderInserter.max'
DESIGN_NAME = MAXFILE.replace('.max', '')
sources = ['headerTest.c']
target = 'header'
includes = []

#port_ip =  { 'TOP': '172.16.50.1' , 'BOT': '172.16.60.1'  }
#port_tap = { 'TOP': '172.16.50.10', 'BOT': '172.16.60.10' }
#netmask = '255.255.255.0'

def slicCompile():
	"""Compiles a maxfile in to a .o file"""
	run("%s/bin/sliccompile" % (MAXCOMPILERDIR), MAXFILE, MAXFILE.replace('.max', '.o'))

def getMaxCompilerInc():
	"""Return the includes to be used in the compilation."""
	return ['-I.', '-I%s/include' % MAXOSDIR, '-I%s/include/slic' % MAXCOMPILERDIR, '-I%s/include/slicnet' % MAXCOMPILERNETDIR]

def getMaxCompilerLibs():
	"""Return the libraries to be used in linking."""
	return ['-L%s/lib' % MAXCOMPILERDIR, '-L%s/lib' % MAXCOMPILERNETDIR,'-L%s/lib' % MAXOSDIR, '-lslicnet', '-lslic', '-lmaxeleros', '-lm', '-lpthread']

def getLinkerLibs():
	"""Returns the libraries to be used for linking."""
	return getMaxCompilerLibs() +  [MAXFILE.replace('.max', '.o'), '-lslicnet', '-lslic'] 


cflags = ['-ggdb', '-O2', '-fPIC', 
		  '-std=gnu99', '-Wall', '-Werror', 
		  '-DDESIGN_NAME=%s' % (DESIGN_NAME)] + includes + getMaxCompilerInc() 

def build():
	compile()
	link()
	print ("\n\nTo run in simulation, do:\n" 
		 "\t$ ./build.py run_sim\n")

def compile():
	slicCompile()
	for source in sources:
		run('gcc', cflags, '-c', source, '-o', source.replace('.c', '.o'))

def link():
	objects = [s.replace('.c', '.o') for s in sources]
	run('gcc', objects, getLinkerLibs(), '-o', target)

def clean():
	autoclean()

def getSimName():
	return getpass.getuser() + 'Sim'

def maxcompilersim():
	return '%s/bin/maxcompilersim' % MAXCOMPILERDIR


def eth_sim(port):
#	cmd =  ['-e', 'QSFP_%s_10G_PORT1:%s:%s' % (port, port_tap[port], netmask)]
#	cmd += ['-p', 'QSFP_%s_10G_PORT1:%s.pcap' % (port, port) ] 
#	return cmd
	return []

def run_sim():
	build()
	restart_sim()
	subprocess.call(['./' + target])

def start_sim():
	cmd = [maxcompilersim(), '-n', getSimName(), '-c', DFE_MODEL ] + eth_sim("TOP") + eth_sim("BOT") + ["restart"];
	print "cmd " + " ".join(cmd);
	subprocess.call(cmd)

def stop_sim():
	subprocess.call([maxcompilersim(), '-n', getSimName()] + eth_sim("TOP") + eth_sim("BOT") + ["stop"])

def restart_sim():
	start_sim()	
	
def sim_debug():
	subprocess.call(['maxdebug', '-g', 'graph_%s' % getSimName(), '-d',  '%s0:%s' % (getSimName(), getSimName()), MAXFILE])

if __name__ == '__main__':
	main()
