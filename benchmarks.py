#!/usr/bin/env python3

import collections
import json, numbers, pprint, platform, tempfile, uuid
import os, shutil, subprocess, signal
import lttng

runner_bin  = "./lttng-ust-benchmarks"
props_dir   = "./jenkins_plot_data"
js_out      = "./benchmarks.json"
passes      = 3
tmp_dir     = tempfile.mkdtemp()
session_name     = str(uuid.uuid1())
trace_path       = tmp_dir + "/" + session_name
sessiond_pidfile = tmp_dir + "/lttng-sessiond.pid"

def lttng_start():
	if lttng.session_daemon_alive() == 0:
		daemon_cmd  = "lttng-sessiond --daemonize --quiet"
		daemon_cmd += " --pidfile " + sessiond_pidfile
		subprocess.check_call(daemon_cmd, shell=True)

	lttng.destroy(session_name)
	ret = lttng.create_snapshot(session_name, trace_path)
	if ret < 0:
		raise RuntimeError("LTTng: " + lttng.strerror(ret))

	domain = lttng.Domain()
	domain.type = lttng.DOMAIN_UST

	channel = lttng.Channel()
	channel.name = "channel0"
	lttng.channel_set_default_attr(domain, channel.attr)

	han = lttng.Handle(session_name, domain)
	if han is None:
		raise RuntimeError("LTTng: failed to create handle")

	res = lttng.enable_channel(han, channel)
	if res < 0:
		raise RuntimeError("LTTng: " + lttng.strerror(ret))
	
	event = lttng.Event()
	event.name = "*"
	event.type = lttng.EVENT_TRACEPOINT
	event.loglevel = lttng.EVENT_LOGLEVEL_ALL
	ret = lttng.enable_event(han, event, channel.name)
	if ret < 0:
		raise RuntimeError("LTTng: " + lttng.strerror(ret))
	
	ret = lttng.start(session_name)
	if ret < 0:
		raise RuntimeError("LTTng: " + lttng.strerror(ret))

def lttng_stop():
	ret = lttng.stop(session_name)
	if ret < 0:
		raise RuntimeError("LTTng: " + lttng.strerror(ret))

	ret = lttng.destroy(session_name)
	if ret < 0:
		raise RuntimeError("LTTng: " + lttng.strerror(ret))

def run_benchmark(args, with_ust = False):

	if with_ust:
		lttng_start()

	output = subprocess.check_output([runner_bin] + args)

	if with_ust:
		lttng_stop()

	return json.loads(output.decode())

def run_benchmarks(benchmarks):
	results = {}

	for name in benchmarks.keys():
		
		benchmark = benchmarks[name]

		base = run_benchmark(benchmark["baseline"], False)
		base_start  = base["main"] - base["exec"]
		base_run    = base["end"] - base["start"]

		ust = run_benchmark(benchmark["ust"], False)
		ust_start  = ust["main"] - ust["exec"]
		ust_run    = ust["end"] - ust["start"]
		ust_start_overhead = ust_start - base_start
		
		ust_en = run_benchmark(benchmark["ust"], True)
		ust_en_start  = ust_en["main"] - ust_en["exec"]
		ust_en_run    = ust_en["end"] - ust_en["start"]
		ust_en_start_overhead = ust_en_start - base_start

		results[name] = {
			"baseline": {
				"args":	       benchmark["baseline"],
				"start_time":  base_start,
				"run_time":    base_run
				},
			"tracing_disabled": {
				"args":         benchmark["ust"],
				"start_time":   ust_start,
				"run_time":     ust_run,
				"ns_per_event": (ust_run - base_run)*1E9 / benchmark["nr_loops"],
				"start_overhead_s":   ust_start_overhead,
				"start_overhead_pct": ust_start_overhead * 100 / base_start
				},
			"tracing_enabled": {
				"args":	        benchmark["ust"],
				"start_time":   ust_en_start,
				"run_time":     ust_en_run,
				"event_count":  benchmark["nr_loops"],
				"ns_per_event": (ust_en_run - base_run)*1E9 / benchmark["nr_loops"],
				"start_overhead_s":   ust_en_start_overhead,
				"start_overhead_pct": ust_en_start_overhead * 100 / base_start,
				}
			}

	return results

def flatten_dict_internal(prefix, data, dst):
	if issubclass(data.__class__, str):
		dst[prefix] = data
	elif issubclass(data.__class__, collections.Mapping):
		for key in data.keys():
			fullprefix = prefix + "." + key if len(prefix) > 0 else key
			flatten_dict_internal(fullprefix, data[key], dst)
	elif issubclass(data.__class__, collections.Iterable):
		i = 0
		for value in data:
			fullprefix = prefix + "." + str(i)
			flatten_dict_internal(fullprefix, value, dst)
			i += 1
	else:
		dst[prefix] = data

def flatten_dict(data):
	dst = dict()
	flatten_dict_internal("", data, dst)
	return dst

def write_plot_properties(data, dst_dir):
	os.makedirs(dst_dir, 0o777, True)
	flat_data = data
	for key in flat_data:
		f = open(dst_dir + "/" + key + ".properties", "w")
		f.write("YVALUE=" + str(flat_data[key]) + "\n")
		f.close()

def write_js(data, filename):
	f = open(filename, "w")
	pprint.pprint(data, f)
	f.close()

def main():
	benchmarks = {
		"basic": {
			# number of loops here must match nr_loops
			# below.
			"baseline": ["basic-benchmark", "10000000"],
			"ust":      ["basic-benchmark-ust", "10000000"],
			"nr_loops": 10000000
			},
		"sha2": {
			# number of loops here must match nr_loops
			# below.
			"baseline": ["sha2-benchmark", "1000000"],
			"ust":      ["sha2-benchmark-ust", "1000000"],
			"nr_loops": 1000000
			}
		}

	data = []
	flat_data = []
	all_keys = set()
	for i in range(passes):
		pass_data = run_benchmarks(benchmarks)
		flat_pass_data = flatten_dict(pass_data)

		data.append(pass_data)
		flat_data.append(flat_pass_data)

		all_keys |= flat_pass_data.keys()

	avg_data = {}
	for key in all_keys:
		total = 0
		count = 0
		for flat_pass_data in flat_data:
			if (isinstance(flat_pass_data[key], numbers.Number)):
				total += flat_pass_data[key]
				count += 1
		if count > 0:
			avg_data[key] = total / count

	results = {
		"data":     data,
		"platform": dict(zip(
			("system", "node", "release", "version", "machine", "processor"),
			platform.uname()
			))
	}
	write_plot_properties(avg_data, props_dir)
	write_js(results, js_out)

def cleanup():
	lttng.stop(session_name)
	lttng.destroy(session_name)

	try:
		f = open(sessiond_pidfile, "r")
		pid = int(f.readline())
		f.close()
		os.kill(pid, signal.SIGTERM)
	except (OSError, IOError) as e:
		# Nothing to do: we may not have launched the sessiond
		pass

	# Ignore errors when deleting the temporary directory (otherwise
	# an exception may be raised if the sessiond removes its pidfile
	# while rmtree() is running).
	shutil.rmtree(tmp_dir, True)
		
if __name__ == "__main__":
	try:
		main()
	except KeyboardInterrupt:
		pass
	finally:
		cleanup()
