#!/usr/bin/env python3

import os, subprocess

class LinuxModule:

	def __init__(self, mod_path):
		self.mod_path = mod_path
		self.mod_name, _ = os.path.splitext(os.path.basename(mod_path))

	def is_loaded(self):
		return os.path.exists("/sys/module/" + self.mod_name)

	def load(self):
		if self.is_loaded(): return
		subprocess.check_call("insmod " + self.mod_path, shell=True)

	def unload(self):
		if not self.is_loaded(): return
		subprocess.check_call("rmmod " + self.mod_name, shell=True)

	def reload(self):
		self.unload()
		self.load()

main_module = LinuxModule("kernel-benchmark/lttng_benchmark.ko")
probe_module = LinuxModule("kernel-benchmark/lttng_benchmark_probe.ko")

def benchmark(count):
	main_module.load()
	probe_module.load()
	count_file = open("/sys/kernel/lttng_benchmark/count", "w")
	count_file.write(str(count))
	count_file.flush()
	time_file = open("/sys/kernel/lttng_benchmark/time", "r")
	return float(time_file.read().rstrip())/1E6

def unload_modules():
	main_module.unload()
	probe_module.unload()

def events():
	events_file = open("/sys/kernel/lttng_benchmark/events", "r")
	return [line.rstrip() for line in events_file]
