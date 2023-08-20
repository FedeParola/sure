#!/usr/bin/python3

import os
import subprocess
import time

curdir = os.path.dirname(__file__)

TARGETS		  = ['radiobox', 'localhost', 'bridge',	'unix', 'skmsg']
TARGET		  = 'localhost'
RUNS		  = 1
RR_ITERATIONS	  = 1000000
WARMUP_ITERATIONS = 1000
RES_FILENAME	  = 'res-rr-latency.csv'
MSG_SIZE	  = 64
MGR_PATH	  = '/users/fparola/src/unimsg/manager/unimsg_manager'
TESTS_GAP	  = 5 # Seconds of gap between two tests
SAR_SECS	  = 5 # Must guarantee that the test won't end before sar
SERVER_COMMANDS = {
	'radiobox': ['sudo', './radiobox/run_vm.sh'],
	'localhost': ['./process/build/rr-latency'],
	'bridge': ['sudo', 'ip', 'netns', 'exec', 'ns1',
		   './process/build/rr-latency'],
	'unix': ['./process/build/rr-latency'],
	'skmsg': ['sudo', './process/build/rr-latency']
}
CLIENT_COMMANDS = {
	'radiobox': ['sudo', './radiobox/run_vm.sh'],
	'localhost': ['./process/build/rr-latency'],
	'bridge': ['sudo', 'ip', 'netns', 'exec', 'ns2',
		   './process/build/rr-latency'],
	'unix': ['./process/build/rr-latency'],
	'skmsg': ['sudo', './process/build/rr-latency']
}
FLAGS = {
	'radiobox': [],
	'localhost': ['-l'],
	'bridge': [],
	'unix': ['-u'],
	'skmsg': ['-l', '-m']
}

out = open(RES_FILENAME, 'w')
out.write('run,msg-size,rr-latency,user,system,softirq,guest,idle\n')

manager_cmd = [MGR_PATH]
manager = subprocess.Popen(manager_cmd, stderr=subprocess.DEVNULL,
			   stdout=subprocess.DEVNULL)

for run in range(RUNS):
	server_cmd = ['taskset', '1'] + SERVER_COMMANDS[TARGET] + FLAGS[TARGET]
	client_cmd = ['taskset', '2'] + CLIENT_COMMANDS[TARGET] + ['-c', '-i',
		     str(RR_ITERATIONS), '-s', str(MSG_SIZE), '-w',
		     str(WARMUP_ITERATIONS)] + FLAGS[TARGET]

	print(f'Run {run}: exchanging {RR_ITERATIONS} rrs of size {MSG_SIZE}...')

	server = subprocess.Popen(server_cmd, stderr=subprocess.DEVNULL,
				  stdout=subprocess.DEVNULL)
	time.sleep(1)
	client = subprocess.Popen(client_cmd, stderr=subprocess.DEVNULL,
				  stdout=subprocess.PIPE, text=True)

	time.sleep(1.5)

	# Collect CPU statistics
	# Make sure the apps run long enough
	cmd = ['sar', '-P', '0,1', '-u', 'ALL', str(SAR_SECS), '1']
	res = subprocess.run(cmd, check=True, capture_output=True, text=True) \
		        .stdout
	resline0 = res.splitlines()[3]
	resline1 = res.splitlines()[4]
	user	 = float(resline0.split()[3])
	system	 = float(resline0.split()[5])
	softirq	 = float(resline0.split()[9])
	guest	 = float(resline0.split()[10])
	idle	 = float(resline0.split()[12])
	user	 += float(resline1.split()[3])
	system	 += float(resline1.split()[5])
	softirq	 += float(resline1.split()[9])
	guest	 += float(resline1.split()[10])
	idle	 += float(resline1.split()[12])

	server.wait()
	client.wait()

	res = client.stdout.readlines()
	for line in res:
		if line.split('=')[0] == 'rr-latency':
			latency = int(line.split('=')[1])

	print(f'latency={latency} ns, user={user:.2f}%, system={system:.2f} %, softirq={softirq:.2f} %, guest={guest:.2f} %, idle={idle:.2f} %\n')

	out.write(f'{run},{MSG_SIZE},{latency},{user:.2f},{system:.2f},{softirq:.2f},{guest:.2f},{idle:.2f}\n')
	out.flush()

	time.sleep(TESTS_GAP)

manager.terminate()
manager.wait()

out.close()
