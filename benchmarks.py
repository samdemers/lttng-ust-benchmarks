#!/usr/bin/env python3

import collections
import json, os, pprint, platform, subprocess, tempfile, uuid
import lttng

runner_bin  = "./lttng-ust-benchmarks"
props_dir   = "./jenkins_plot_data"
js_out      = "./benchmarks.json"

def lttng_start(trace_path, session_name):

	if lttng.session_daemon_alive() == 0:
		daemon_cmd = "lttng-sessiond --daemonize --quiet"
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

def lttng_stop(session_name):
	ret = lttng.stop(session_name)
	if ret < 0:
		raise RuntimeError("LTTng: " + lttng.strerror(ret))

	ret = lttng.destroy(session_name)
	if ret < 0:
		raise RuntimeError("LTTng: " + lttng.strerror(ret))

def run_benchmark(args, with_ust = False):

	if with_ust:
		session_name = str(uuid.uuid1())
		trace_path = tempfile.mkdtemp() + "/" + session_name
		lttng_start(trace_path, session_name)

	output = subprocess.check_output([runner_bin] + args)

	if with_ust:
		lttng_stop(session_name)

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
		dst[prefix] = str(data)

def flatten_dict(data):
	dst = dict()
	flatten_dict_internal("", data, dst)
	return dst

def write_plot_properties(data, dst_dir):
	os.makedirs(dst_dir, 0o777, True)
	flat_data = flatten_dict(data)
	for key in flat_data:
		f = open(dst_dir + "/" + key + ".properties", "w")
		f.write("YVALUE=" + flat_data[key] + "\n")
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

	results = {
		"data":     run_benchmarks(benchmarks),
		"platform": dict(zip(
			("system", "node", "release", "version", "machine", "processor"),
			platform.uname()
			))
	}
	write_plot_properties(results["data"], props_dir)
	write_js(results, js_out)
		
if __name__ == "__main__":
	main()
