#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Author: jinpf

'''
This for recover the code to real modification

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

# redir = {'src':'dis'}
redir = {'./ns-3/scratch' : '../ns-3/scratch' , \
		 './ns-3/src/ndnSIM/apps' : '../ns-3/src/ndnSIM/apps' , \
		 './ns-3/src/ndnSIM/utils/tracers' : '../ns-3/src/ndnSIM/utils/tracers'}

def recover(src, dst):
	names = os.listdir(src)
	if not os.path.exists(dst):
		os.makedirs(dst)
	for name in names:
		srcname = os.path.join(src, name)
		dstname = os.path.join(dst, name)
		if os.path.isdir(srcname):
			recover(srcname,dstname)
		else:
			shutil.copy2(srcname,dstname)
	shutil.copystat(src, dst)

if __name__ == '__main__':
	for i in redir:
		recover(i,redir[i])