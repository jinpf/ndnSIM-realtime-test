#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Author: jinpf
# @Date:   2015-07-24 22:14:06
# @Last Modified by:   jpflc
# @Last Modified time: 2015-11-13 16:26:12


def read_calculate(fname):
	with open(fname,'r') as f:
		# P_GData_seq : {seq:time}
		P_GData_seq = {}
		# C_Data_seq : {ci:{seq:time}}
		C_Data_seq = {}
		f.readline()
		record = f.readline()
		while record != '':
			Datas = record.strip('\n').split('\t')
			if len(Datas) < 6:
				break
			if Datas[5] == 'P_GData' and (int(Datas[4]) not in P_GData_seq):
				P_GData_seq[int(Datas[4])] = float(Datas[0])
			elif Datas[5] == 'C_Data':
				if Datas[1] not in C_Data_seq:
					C_Data_seq[Datas[1]] = {}
				if (int(Datas[4]) not in C_Data_seq[Datas[1]]):
					C_Data_seq[Datas[1]][int(Datas[4])] = float(Datas[0])
			record = f.readline()

		# delay = {ci:{seq:delay}}
		delay = {}
		for user in C_Data_seq:
			delay[user] = {}
			for seq in C_Data_seq[user]:
				delay[user][seq] = C_Data_seq[user][seq] -P_GData_seq[seq]

		return delay

# delay: {seq:delay}
def ave_delay(delay):
	ave = []
	for ci in delay:
		a = sum(delay[ci].values()) / len(delay[ci])
		print ci, ' maxseq: ', max(delay[ci].keys()), ' ave: ', a
		ave.append(a)
	return sum(ave) / len(ave)

def write_delay(fname,delay):
	with open(fname,'w') as f:
		user = delay.keys()
		user.sort()
		W_str = 'seq'
		maxseq = 1
		for ci in user:
			W_str += '\t' + ci
			maxseq = max(max(delay[ci].keys()),maxseq)
		W_str += '\n'
		f.write(W_str)

		for seq in range(1,maxseq+1):
			W_str = str(seq)
			for ci in user:
				W_str += '\t' + (str(delay[ci][seq]) if delay[ci].has_key(seq) else ' ')
			W_str += '\n'
			f.write(W_str)

if __name__ == '__main__':
	type = 'pull'
	delay = read_calculate('bottle-'+type+'-packet-record.txt')
	print ave_delay(delay)
	write_delay('app_'+type+'_delay.txt',delay)