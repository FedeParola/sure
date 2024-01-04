#!/usr/bin/python3

import os
import signal
import subprocess
import time

curdir = os.path.dirname(__file__)

TARGETS		  = ['radiobox', 'localhost', 'localhost-sidecar']
TARGET		  = 'radiobox'
OSV_PATH	  = '/users/fparola/src/osv'
RUNS		  = 10
CONCURRENCY	  = 32
DURATION	  = 10
RES_FILENAME	  = 'res-throughput-sidecar.csv'
MSG_SIZES	  = [256, 4096, 8192]
TESTS_GAP	  = 5 # Seconds of gap between two tests
SAR_SECS	  = 5 # Must guarantee that the test won't end before sar
SERVER_COMMANDS = {
	'radiobox': ['sudo', './radiobox/run.sh', '1', '-h'],
	'localhost': ['./process/build/throughput', '-l', '-h'],
	'localhost-sidecar': ['./process/build/throughput', '-l', '-h'],
}
CLIENT1_COMMANDS = {
	'radiobox': ['sudo', './radiobox/run.sh', '2', '-h'],
	'localhost': ['./process/build/throughput', '-l', '-h'],
	'localhost-sidecar': ['./process/build/throughput', '-l', '-h',
		       	      '-p', '80'],
}
CLIENT2_COMMANDS = {
	'radiobox': ['sudo', './radiobox/run.sh', '3', '-h'],
	'localhost': ['./process/build/throughput', '-l', '-h'],
	'localhost-sidecar': ['./process/build/throughput', '-l', '-h',
		       	      '-p', '80'],
}

out = open(RES_FILENAME, 'w')
out.write('run,concurrency,msg-size,rps\n')

for size in MSG_SIZES:
	for run in range(RUNS):
		server_cmd = ['taskset', '1'] + SERVER_COMMANDS[TARGET] + ['-s',
			     str(size)]
		client1_cmd = ['taskset', '2'] + CLIENT1_COMMANDS[TARGET] + \
			      ['-c', str(CONCURRENCY), '-d', str(DURATION),
			       '-s', str(size)]
		client2_cmd = ['taskset', '4'] + CLIENT2_COMMANDS[TARGET] + \
			      ['-c', str(CONCURRENCY), '-d', str(DURATION),
			       '-s', str(size)]

		print(f'Run {run}: running {CONCURRENCY * 2} connections with message size {size}...')

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

		client1.wait()
		client2.wait()
		if TARGET == 'localhost-sidecar':
			server.send_signal(signal.SIGINT)
		server.wait()

		res = server.stdout.readlines()
		for line in res:
			if line.split('=')[0] == 'rps':
				rps = int(line.split('=')[1])

		print(f'rps={rps}\n')

		out.write(f'{run},{CONCURRENCY * 2},{size},{rps}\n')
		out.flush()

		time.sleep(TESTS_GAP)

out.close()
