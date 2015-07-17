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
		  '../ns-3/src/ndnSIM/apps' : './ns-3/src/ndnSIM/apps'}

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
	return ifile


def synfile(src, dst):
	if os.path.exists(dst):
		shutil.rmtree(dst)
	shutil.copytree(src, dst, ignore=ignorefile)


if __name__ == '__main__':
	for i in syndir:
		synfile(i,syndir[i])