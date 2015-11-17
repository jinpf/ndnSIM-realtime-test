#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Author: jinpf
# @Date:   2015-11-09 14:25:33
# @Last Modified by:   jpflc
# @Last Modified time: 2015-11-16 22:11:48

def read_delay(fname):
	with open(fname,'r') as f:
		# {delay:count}
		delay_count = {}
		f.readline()
		record = f.readline()
		count = 0
		while record != '':
			Datas = record.strip('\n').split('\t')
			if len(Datas) == 4 and Datas[1] != ' ':
				count += 1
				if float(Datas[1]) in delay_count:
					delay_count[float(Datas[1])] += 1
				else:
					delay_count[float(Datas[1])] = 1
			record = f.readline()
		return delay_count, count

def cdf(delays,total,fname):
	delay = delays.keys()
	delay.sort()
	s = 0
	with open(fname,'w') as f:
		w_str = 'delay' + '\t' + 'CDF' + '\n'
		f.write(w_str)
		for d in delay:
			s += delays[d]
			w_str = str(d) + '\t' + str(s*1.0/total) + '\n'
			f.write(w_str)



if __name__ == '__main__':
	type = 'pull'
	delays,total = read_delay('app_'+type+'_delay.txt')
	# print delays
	# print total
	cdf(delays,total,'delay_'+type+'_CDF.txt')