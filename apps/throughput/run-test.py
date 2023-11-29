#!/usr/bin/python3

import os
import subprocess
import time

curdir = os.path.dirname(__file__)

TARGETS		  = ['radiobox', 'radiobox-buf-reuse', 'localhost', 'bridge',
		     'unix', 'skmsg', 'unikraft', 'unikraft-ovs', 'osv',
		     'osv-ovs']
TARGET		  = 'radiobox'
OSV_PATH	  = '/users/fparola/src/osv'
RUNS		  = 10
CONCURRENCIES	  = [1, 2, 4, 8, 16, 32, 64]
DURATION	  = 10
RES_FILENAME	  = 'res-throughput.csv'
MSG_SIZE	  = 64
TESTS_GAP	  = 5 # Seconds of gap between two tests
SAR_SECS	  = 5 # Must guarantee that the test won't end before sar
SERVER_COMMANDS = {
	'radiobox': ['sudo', './radiobox/run_vm.sh', '1'],
	'radiobox-buf-reuse': ['sudo', './radiobox/run_vm.sh', '1', '-r'],
	'localhost': ['./process/build/throughput', '-l'],
	'bridge': ['sudo', 'ip', 'netns', 'exec', 'ns1',
		   './process/build/throughput'],
	'unix': ['./process/build/throughput', '-u'],
	'skmsg': ['sudo', './process/build/throughput', '-l', '-m'],
	'unikraft': ['sudo', './unikraft/run_vm.sh', '1'],
	'unikraft-ovs': ['sudo', './unikraft/run_vm_ovs.sh', '1'],
	'osv': ['sudo', OSV_PATH + '/modules/throughput/run_vm.sh', '1'],
	'osv-ovs': ['sudo', OSV_PATH + '/modules/throughput/run_vm_ovs.sh',
		    '1'],
}
CLIENT1_COMMANDS = {
	'radiobox': ['sudo', './radiobox/run_vm.sh', '2'],
	'radiobox-buf-reuse': ['sudo', './radiobox/run_vm.sh', '2', '-r'],
	'localhost': ['./process/build/throughput', '-l'],
	'bridge': ['sudo', 'ip', 'netns', 'exec', 'ns2',
		   './process/build/throughput'],
	'unix': ['./process/build/throughput', '-u'],
	'skmsg': ['sudo', './process/build/throughput', '-l', '-m'],
	'unikraft': ['sudo', './unikraft/run_vm.sh', '2'],
	'unikraft-ovs': ['sudo', './unikraft/run_vm_ovs.sh', '2'],
	'osv': ['sudo', OSV_PATH + '/modules/throughput/run_vm.sh', '2'],
	'osv-ovs': ['sudo', OSV_PATH + '/modules/throughput/run_vm_ovs.sh',
		    '2'],
}
CLIENT2_COMMANDS = {
	'radiobox': ['sudo', './radiobox/run_vm.sh', '3'],
	'radiobox-buf-reuse': ['sudo', './radiobox/run_vm.sh', '3', '-r'],
	'localhost': ['./process/build/throughput', '-l'],
	'bridge': ['sudo', 'ip', 'netns', 'exec', 'ns3',
		   './process/build/throughput'],
	'unix': ['./process/build/throughput', '-u'],
	'skmsg': ['sudo', './process/build/throughput', '-l', '-m'],
	'unikraft': ['sudo', './unikraft/run_vm.sh', '3'],
	'unikraft-ovs': ['sudo', './unikraft/run_vm_ovs.sh', '3'],
	'osv': ['sudo', OSV_PATH + '/modules/throughput/run_vm.sh', '3'],
	'osv-ovs': ['sudo', OSV_PATH + '/modules/throughput/run_vm_ovs.sh', 
		    '3'],
}

out = open(RES_FILENAME, 'w')
out.write('run,concurrency,msg-size,rps\n')

for concurrency in CONCURRENCIES:
	for run in range(RUNS):
		server_cmd = ['taskset', '1'] + SERVER_COMMANDS[TARGET] + ['-s',
			     str(MSG_SIZE)]
		client1_cmd = ['taskset', '2'] + CLIENT1_COMMANDS[TARGET] + \
			      ['-c', str(concurrency), '-d', str(DURATION),
			       '-s', str(MSG_SIZE)]
		client2_cmd = ['taskset', '4'] + CLIENT2_COMMANDS[TARGET] + \
			      ['-c', str(concurrency), '-d', str(DURATION),
			       '-s', str(MSG_SIZE)]

		print(f'Run {run}: running {concurrency * 2} connections with message size {MSG_SIZE}...')

		server = subprocess.Popen(server_cmd, stderr=subprocess.DEVNULL,
					  stdout=subprocess.PIPE, text=True)
		time.sleep(0.5)
		client1 = subprocess.Popen(client1_cmd,
					   stderr=subprocess.DEVNULL,
					   stdout=subprocess.DEVNULL)
		client2 = subprocess.Popen(client2_cmd,
					   stderr=subprocess.DEVNULL,
					   stdout=subprocess.DEVNULL)

		time.sleep(0.5)

		if TARGET == 'unikraft' or TARGET == 'osv':
			cmd = ['pgrep', '-l', 'vhost']
			res = subprocess.run(cmd, check=True,
					     capture_output=True, text=True) \
					     .stdout.splitlines()
			cmd = ['sudo', 'taskset', '-p', '8', res[0].split()[0]]
			subprocess.run(cmd, check=True,
				       stdout=subprocess.DEVNULL)
			cmd = ['sudo', 'taskset', '-p', '10', res[1].split()[0]]
			subprocess.run(cmd, check=True,
				       stdout=subprocess.DEVNULL)
			cmd = ['sudo', 'taskset', '-p', '20', res[2].split()[0]]
			subprocess.run(cmd, check=True,
				       stdout=subprocess.DEVNULL)

		time.sleep(2)

		server.wait()
		client1.wait()
		client2.wait()

		res = server.stdout.readlines()
		for line in res:
			if line.split('=')[0] == 'rps':
				rps = int(line.split('=')[1])

		print(f'rps={rps}\n')

		out.write(f'{run},{concurrency * 2},{MSG_SIZE},{rps}\n')
		out.flush()

		time.sleep(TESTS_GAP)

out.close()
