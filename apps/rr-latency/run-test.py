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
RR_ITERATIONS	  = 1000000
WARMUP_ITERATIONS = 1000
RES_FILENAME	  = 'res-rr-latency.csv'
SIZES		  = [64, 4096, 8192]
TESTS_GAP	  = 5 # Seconds of gap between two tests
SAR_SECS	  = 5 # Must guarantee that the test won't end before sar
SERVER_COMMANDS = {
	'radiobox': ['sudo', './radiobox/run_vm.sh', '1'],
	'radiobox-buf-reuse': ['sudo', './radiobox/run_vm.sh', '1'],
	'localhost': ['./process/build/rr-latency'],
	'bridge': ['sudo', 'ip', 'netns', 'exec', 'ns1',
		   './process/build/rr-latency'],
	'unix': ['./process/build/rr-latency'],
	'skmsg': ['sudo', './process/build/rr-latency'],
	'unikraft': ['sudo', './unikraft/run_server.sh'],
	'unikraft-ovs': ['sudo', './unikraft/run_server_ovs.sh'],
	'osv': ['sudo', OSV_PATH + '/modules/rr-latency/run_server.sh'],
	'osv-ovs': ['sudo', OSV_PATH + '/modules/rr-latency/run_server_ovs.sh'],
}
CLIENT_COMMANDS = {
	'radiobox': ['sudo', './radiobox/run_vm.sh', '2'],
	'radiobox-buf-reuse': ['sudo', './radiobox/run_vm.sh', '2'],
	'localhost': ['./process/build/rr-latency'],
	'bridge': ['sudo', 'ip', 'netns', 'exec', 'ns2',
		   './process/build/rr-latency'],
	'unix': ['./process/build/rr-latency'],
	'skmsg': ['sudo', './process/build/rr-latency'],
	'unikraft': ['sudo', './unikraft/run_client.sh'],
	'unikraft-ovs': ['sudo', './unikraft/run_client_ovs.sh'],
	'osv': ['sudo', OSV_PATH + '/modules/rr-latency/run_client.sh'],
	'osv-ovs': ['sudo', OSV_PATH + '/modules/rr-latency/run_client_ovs.sh'],
}
FLAGS = {
	'radiobox': [],
	'radiobox-buf-reuse': ['-r'],
	'localhost': ['-l'],
	'bridge': [],
	'unix': ['-u'],
	'skmsg': ['-l', '-m'],
	'unikraft': [],
	'unikraft-ovs': [],
	'osv': [],
	'osv-ovs': [],
}

out = open(RES_FILENAME, 'w')
out.write('run,msg-size,rr-latency,user,system,softirq,guest,idle\n')

for size in SIZES:
	for run in range(RUNS):
		server_cmd = ['taskset', '1'] + SERVER_COMMANDS[TARGET] \
			     + ['-s', str(size)] + FLAGS[TARGET]
		client_cmd = ['taskset', '2'] + CLIENT_COMMANDS[TARGET] + ['-c',
			     '-i', str(RR_ITERATIONS), '-s', str(size), '-w',
			     str(WARMUP_ITERATIONS)] + FLAGS[TARGET]	

		print(f'Run {run}: exchanging {RR_ITERATIONS} rrs of size {size}...')

		server = subprocess.Popen(server_cmd, stderr=subprocess.DEVNULL,
					stdout=subprocess.DEVNULL)
		time.sleep(0.5)
		client = subprocess.Popen(client_cmd, stderr=subprocess.DEVNULL,
					stdout=subprocess.PIPE, text=True)

		time.sleep(0.5)

		if TARGET == 'unikraft' or TARGET == 'osv':
			cmd = ['pgrep', '-l', 'vhost']
			res = subprocess.run(cmd, check=True,
					     capture_output=True, text=True) \
					     .stdout.splitlines()
			cmd = ['sudo', 'taskset', '-p', '4', res[0].split()[0]]
			subprocess.run(cmd, check=True,
				       stdout=subprocess.DEVNULL)
			cmd = ['sudo', 'taskset', '-p', '8', res[1].split()[0]]
			subprocess.run(cmd, check=True,
				       stdout=subprocess.DEVNULL)
			cpus = 4
		elif TARGET == 'unikraft-ovs' or TARGET == 'osv-ovs':
			cpus = 3
		else:
			cpus = 2

		time.sleep(2)

		# Collect CPU statistics
		# Make sure the apps run long enough
		cmd = ['sar', '-P', '0-' + str(cpus - 1), '-u', 'ALL',
		       str(SAR_SECS), '1']
		res = subprocess.run(cmd, check=True, capture_output=True,
				     text=True).stdout.splitlines()
		user	= 0
		system	= 0
		softirq	= 0
		guest	= 0
		idle	= 0
		for i in range(cpus):
			user	 += float(res[3 + i].split()[3])
			system	 += float(res[3 + i].split()[5])
			softirq	 += float(res[3 + i].split()[9])
			guest	 += float(res[3 + i].split()[10])
			idle	 += float(res[3 + i].split()[12])

		server.wait()
		client.wait()

		res = client.stdout.readlines()
		for line in res:
			if line.split('=')[0] == 'rr-latency':
				latency = int(line.split('=')[1])

		print(f'latency={latency} ns, user={user:.2f}%, system={system:.2f} %, softirq={softirq:.2f} %, guest={guest:.2f} %, idle={idle:.2f} %\n')

		out.write(f'{run},{size},{latency},{user:.2f},{system:.2f},{softirq:.2f},{guest:.2f},{idle:.2f}\n')
		out.flush()

		time.sleep(TESTS_GAP)

out.close()
