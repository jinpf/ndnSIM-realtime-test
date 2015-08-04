#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Author: jinpf
# @Date:   2015-07-24 22:14:06
# @Last Modified by:   jinpf
# @Last Modified time: 2015-08-04 15:42:31


def read_calculate(fname):
	with open(fname,'r') as f:
		# P_GData_seq : {seq:time}
		P_GData_seq = {}
		# C_Data_seq : {seq:time}
		C_Data_seq = {}
		f.readline()
		record = f.readline()
		while record != '':
			Datas = record.strip('\n').split('\t')
			if len(Datas) < 6:
				break
			if Datas[5] == 'P_GData':
				P_GData_seq[int(Datas[4])] = float(Datas[0])
			if Datas[5] == 'C_Data':
				C_Data_seq[int(Datas[4])] = float(Datas[0])
			record = f.readline()

		delay = {}
		for seq in C_Data_seq:
			delay[seq] = C_Data_seq[seq] - P_GData_seq[seq]

		return delay

# delay: {seq:delay}
def ave_delay(delay):
	print max(delay.keys())
	return sum(delay.values()) / len(delay)

def write_delay(fname,delay):
	with open(fname,'w') as f:
		W_str = 'seq' + '\t' + 'delay' + '\n'
		f.write(W_str)
		for seq in range(1,max(delay.keys())+1):
			W_str = str(seq) + '\t' + (str(delay[seq]) if delay.has_key(seq) else ' ') + '\n'
			f.write(W_str)

if __name__ == '__main__':
	type = 'push'
	delay = read_calculate('line-'+type+'-packet-record.txt')
	print ave_delay(delay)
	write_delay('app_'+type+'_delay.txt',delay)