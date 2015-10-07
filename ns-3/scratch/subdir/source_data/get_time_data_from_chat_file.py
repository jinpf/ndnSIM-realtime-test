#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Author: jinpf
# @Date:   2015-10-07 10:02:16
# @Last Modified by:   jinpf
# @Last Modified time: 2015-10-07 18:47:58

# transfor xx:xx:xx to x s
def time_to_second(s):
	t = s.split(':')
	r = int(t[0]) * 3600 + int(t[1]) * 60 + int(t[2])
	return r

def read_get_data(fname,shif_time,time_size):
	with open(fname,'r') as f:
		lines = f.readlines()
		state = 0
		nowtime = 0
		stime = 0
		for line in lines:
			info = line.strip("\n").split(' ')
			if len(info) == 3 and len(info[0]) == 10:
				if stime == 0:
					stime = time_to_second(info[1])
				nowtime = shif_time + time_to_second(info[1]) - stime
				time_size[nowtime] = 0
			else:
				time_size[nowtime] += len(line)				
				# print line , len(line)
		return time_size

def record_in_file(fname,time_size):
	time = time_size.keys()
	time.sort()
	lines = ""
	for i in time:
		lines += str(i/1000.0) + '\t' + str(time_size[i]) + '\n'
	with open(fname,'w') as f:
		f.write("time\tsize\n")
		f.writelines(lines)


if __name__ == '__main__':
	time_size = {}
	time = 0
	time_size = read_get_data("2.txt",time,time_size)
	record_in_file("chat.txt",time_size)

	# time = max(time_size.keys()) + 120
	
	# for i in time_size:
	# 	print i,time_size[i]
	# print time_to_second("8:51:34")

	# time = max(time_size.keys())
	# size = max(time_size.values())
	# print len(time_size),time,size