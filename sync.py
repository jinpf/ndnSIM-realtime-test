#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Author: jinpf

'''
This for synchronize the code with real modification

dir tree:

ns-3
-scratch
-scr
--ndnSIM
--apps
ndnSIM-realtime-test
-ns-3

'''

import os, shutil

# syndir = {'src':'dis'}
syndir = {'../ns-3/scratch' : './ns-3/scratch' , \
		  '../ns-3/src/ndnSIM/apps' : './ns-3/src/ndnSIM/apps' , \
		  '../ns-3/src/ndnSIM/utils/tracers' : './ns-3/src/ndnSIM/utils/tracers'
		  }

def ignorefile(path, names):
	ifile = set([])

	if path == '../ns-3/src/ndnSIM/apps':
		ifile = set(['callback-based-app.h', \
		             'ndn-consumer-window.h', \
		             'ndn-consumer-zipf-mandelbrot.h', \
		             'ndn-consumer-window.cc', \
		             'ndn-producer.h', \
		             'ndn-producer.cc', \
		             'ndn-consumer-cbr.cc', \
		             'ndn-consumer.h', \
		             'callback-based-app.cc', \
		             'ndn-consumer-cbr.h', \
		             'ndn-app.h', \
		             'ndn-consumer-batches.cc', \
		             'ndn-app.cc', \
		             'ndn-consumer.cc', \
		             'ndn-consumer-zipf-mandelbrot.cc', \
		             'ndn-consumer-batches.h'])
	if path == '../ns-3/src/ndnSIM/utils/tracers':
		ifile = set(['ndn-cs-tracer.h', \
					 'ipv4-rate-l3-tracer.cc', \
					 'ndn-l3-tracer.h', \
					 'ipv4-seqs-app-tracer.h', \
					 'ipv4-l3-tracer.h', \
					 'ndn-l3-tracer.cc', \
					 'l2-tracer.h', \
					 'l2-rate-tracer.cc', \
					 'ipv4-rate-l3-tracer.h', \
					 'ipv4-seqs-app-tracer.cc', \
					 'l2-tracer.cc', \
					 'ndn-l3-aggregate-tracer.h', \
					 'ndn-app-delay-tracer.h', \
					 'ipv4-app-tracer.cc', \
					 'ndn-app-delay-tracer.cc', \
					 'ipv4-app-tracer.h', \
					 'l2-rate-tracer.h', \
					 'ndn-l3-rate-tracer.h', \
					 'ipv4-l3-tracer.cc', \
					 'ndn-l3-aggregate-tracer.cc', \
					 'ndn-l3-rate-tracer.cc', \
					 'ndn-cs-tracer.cc'])


	return ifile


def syntree(src, dst):
	if os.path.exists(dst):
		shutil.rmtree(dst)
	shutil.copytree(src, dst, ignore=ignorefile)


if __name__ == '__main__':
	for i in syndir:
		syntree(i,syndir[i])