#!/usr/bin/env python3

import collections
import json, pprint, platform, subprocess, tempfile, time, uuid
import lttng, babeltrace as bt

runner_bin	= "./lttng-ust-benchmarks"
props_out   = "./benchmarks.properties"
js_out      = "./benchmarks.json"

def lttng_start(trace_path, session_name):

	if lttng.session_daemon_alive() == 0:
		daemon_cmd = "lttng-sessiond --daemonize --quiet"
		subprocess.check_call(daemon_cmd, shell=True)

	lttng.destroy(session_name)
	ret = lttng.create(session_name, trace_path) 
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
	ret = lttng.destroy(session_name)
	if ret < 0:
		raise RuntimeError("LTTng: " + lttng.strerror(ret))

	# Workaround: wait for the trace to be written on disk to avoid segfault
	# when reading it too soon with Babeltrace
	time.sleep(1)

def events_in_trace(trace_path):
	trace_collection = bt.TraceCollection()
	trace_collection.add_traces_recursive(trace_path, "ctf")
	count = 0
	for event in trace_collection.events:
		count += 1
	return count

def run_benchmark(args, with_ust = False):

	if with_ust:
		session_name = str(uuid.uuid1())
		trace_path = tempfile.mkdtemp() + "/" + session_name
		lttng_start(trace_path, session_name)

	output = subprocess.check_output([runner_bin] + args)

	event_count = 0
	if with_ust:
		lttng_stop(session_name)
		event_count = events_in_trace(trace_path)

	return json.loads(output.decode()), event_count

def run_benchmarks(benchmarks):
	results = {}

	for name in benchmarks.keys():
		
		benchmark = benchmarks[name]

		base, _ = run_benchmark(benchmark["baseline"], False)
		base_start  = base["main"] - base["exec"]
		base_run    = base["end"] - base["start"]

		ust, _ = run_benchmark(benchmark["ust"], False)
		ust_start  = ust["main"] - ust["exec"]
		ust_run    = ust["end"] - ust["start"]
		ust_start_overhead = ust_start - base_start
		
		ust_en, event_count = run_benchmark(benchmark["ust"], True)
		ust_en_start  = ust_en["main"] - ust_en["exec"]
		ust_en_run    = ust_en["end"] - ust_en["start"]
		ust_en_start_overhead = ust_en_start - base_start

		results[name] = {
			"baseline": {
				"args":	       benchmark["baseline"],
				"start_time":  base_start,
				"run_time":	   base_run
				},
			"tracing_disabled": {
				"args":         benchmark["ust"],
				"start_time":   ust_start,
				"run_time":	    ust_run,
				"ns_per_event": (ust_run - base_run)*1000000 / event_count,
				"start_overhead_s":   ust_start_overhead,
				"start_overhead_pct": ust_start_overhead * 100 / base_start
				},
			"tracing_enabled": {
				"args":	        benchmark["ust"],
				"start_time":   ust_en_start,
				"run_time":	    ust_en_run,
				"event_count":  event_count,
				"ns_per_event": (ust_en_run - base_run)*1000000 / event_count,
				"start_overhead_s":   ust_en_start_overhead,
				"start_overhead_pct": ust_en_start_overhead * 100 / base_start,
				}
			}

	return results

def write_properties_internal(prefix, data, f):
	if issubclass(data.__class__, str):
		f.write(prefix + " = " + data + "\n")		
	elif issubclass(data.__class__, collections.Mapping):
		for key in data.keys():
			fullprefix = prefix + "." if len(prefix) > 0 else ""
			write_properties_internal(fullprefix + key, data[key], f)
	elif issubclass(data.__class__, collections.Iterable):
		i = 0
		for value in data:
			fullprefix = prefix + "[" + str(i) + "]"
			write_properties_internal(fullprefix, value, f)
			i += 1
	else:
		f.write(prefix + " = " + str(data) + "\n")

def write_properties(data, filename):
	f = open(filename, "w")
	write_properties_internal("", data, f)
	f.close()

def write_js(data, filename):
	f = open(filename, "w")
	pprint.pprint(data, f)
	f.close()

def main():
	benchmarks = {
		"basic": {
			"baseline": ["basic-benchmark", "1000000"],
			"ust":      ["basic-benchmark-ust", "1000000"]
			},
		"sha2": {
			"baseline": ["sha2-benchmark"],
			"ust":      ["sha2-benchmark-ust"]
			}
		}

	results = {
		"data":     run_benchmarks(benchmarks),
		"platform": dict(zip(
			("system", "node", "release", "version", "machine", "processor"),
			platform.uname()
			))
	}
	write_properties(results, props_out)
	write_js(results, js_out)
		
if __name__ == "__main__":
	main()
