#!/usr/bin/python3

import os
import subprocess
import time

curdir = os.path.dirname(__file__)

TARGETS		  = ['sure', 'sure-buf-reuse', 'localhost', 'bridge',
		     'unix', 'skmsg', 'unikraft', 'unikraft-ovs', 'osv',
		     'osv-ovs']
TARGET		  = 'sure'
RUNS		  = 10
RR_ITERATIONS	  = 1000000
WARMUP_ITERATIONS = 1000
RES_FILENAME	  = 'res-rr-latency-sidecar.csv'
SIZES		  = [256, 4096, 8192]
TESTS_GAP	  = 5 # Seconds of gap between two tests
SAR_SECS	  = 5 # Must guarantee that the test won't end before sar
SERVER_COMMANDS = {
	'sure': ['sudo', './sure/run.sh', '1', '-h'],
	'localhost': ['./process/build/rr-latency', '-l', '-h'],
	'localhost-sidecar': ['./process/build/throughput', '-l', '-h'],
}
CLIENT_COMMANDS = {
	'sure': ['sudo', './sure/run.sh', '2'],
	'localhost': ['./process/build/rr-latency', '-l', '-h'],
	'localhost-sidecar': ['./process/build/rr-latency', '-l', '-h',
			      '-p', '80'],
}

out = open(RES_FILENAME, 'w')
out.write('run,msg-size,rr-latency,user,system,softirq,guest,idle\n')

for size in SIZES:
	for run in range(RUNS):
		server_cmd = ['taskset', '1'] + SERVER_COMMANDS[TARGET] + ['-s',
			     str(size)]
		client_cmd = ['taskset', '2'] + CLIENT_COMMANDS[TARGET] + ['-c',
			     '-i', str(RR_ITERATIONS), '-w',
			     str(WARMUP_ITERATIONS)]

		print(f'Run {run}: exchanging {RR_ITERATIONS} rrs of size {size}...')

		server = subprocess.Popen(server_cmd, stderr=subprocess.DEVNULL,
					stdout=subprocess.DEVNULL)
		time.sleep(0.5)
		client = subprocess.Popen(client_cmd, stderr=subprocess.DEVNULL,
					stdout=subprocess.PIPE, text=True)

		time.sleep(2)

		# Collect CPU statistics
		# Make sure the apps run long enough
		cmd = ['sar', '-P', '2', '-u', 'ALL', str(SAR_SECS), '1']
		res = subprocess.run(cmd, check=True, capture_output=True,
				     text=True).stdout.splitlines()
		user	 = float(res[3].split()[3])
		system	 = float(res[3].split()[5])
		softirq	 = float(res[3].split()[9])
		guest	 = float(res[3].split()[10])
		idle	 = float(res[3].split()[12])

		client.wait()
		server.terminate()

		res = client.stdout.readlines()
		for line in res:
			if line.split('=')[0] == 'rr-latency':
				latency = int(line.split('=')[1])

		print(f'latency={latency} ns, user={user:.2f}%, system={system:.2f} %, softirq={softirq:.2f} %, guest={guest:.2f} %, idle={idle:.2f} %\n')

		out.write(f'{run},{size},{latency},{user:.2f},{system:.2f},{softirq:.2f},{guest:.2f},{idle:.2f}\n')
		out.flush()

		time.sleep(TESTS_GAP)

out.close()
