# -*- coding: utf-8 -*-
# @Author: jinpf
# @Date:   2016-03-19 16:42:36
# @Last Modified by:   jinpf
# @Last Modified time: 2016-03-19 16:56:06

def read_calculate(fname):
	with open(fname,'r') as f:
		f.readline()
		record = f.readline()
		count = 0
		while record != '':
			Datas = record.strip('\n').split('\t')
			if Datas[5] in ('P_Pull_Interest','C_Data'):
				count += 1
			record = f.readline()
	return count

if __name__ == '__main__':
	type = 'pull'
	print read_calculate('line-'+type+'-packet-record.txt')