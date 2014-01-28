#!/usr/bin/env python3

import random, string, sys

def add_field(type, args):
	fields.append("{}({})".format(type, ", ".join(args)))

def clever_name():
	name = ""
	for i in range(random.randint(2, 5)):
		name += random.choice("bcdfghjklmnpqrstvwxyz")
		name += random.choice("aeiouy")
	return name

def add_ctf_numeric(ctf_type, c_type):
	v = ["tp_{}_{}".format(c_type, i+1) for i in range(random.randint(1,2))]
	vars.update(v)
	add_field(ctf_type, [c_type, clever_name(), " + ".join(v)])	

def add_ctf_integer_int():
	add_ctf_numeric("ctf_integer", "int")

def add_ctf_integer_long():
	add_ctf_numeric("ctf_integer", "long")

def add_ctf_float_float():
	add_ctf_numeric("ctf_float", "float")

def add_ctf_float_double():
	add_ctf_numeric("ctf_float", "double")

def add_ctf_string():
	vars.add("tp_charptr_1")
	add_field("ctf_string", [clever_name(), "tp_charptr_1"])

def add_ctf_sequence_char():
	vars.add("tp_charptr_1")
	vars.add("tp_charptr_1_size")
	add_field("ctf_sequence", ["char", clever_name(), "tp_charptr_1",
							   "size_t", "tp_charptr_1_size"])

def add_ctf_sequence_text_char():
	vars.add("tp_charptr_1")
	vars.add("tp_charptr_1_size")
	add_field("ctf_sequence_text", ["char", clever_name(), "tp_charptr_1",
									"size_t", "tp_charptr_1_size"])

def add_ctf_array_char():
	vars.add("tp_charptr_1")
	add_field("ctf_array", ["char", clever_name(), "tp_charptr_1",
							str(c_vars["tp_charptr_1_size"][1])])

def add_ctf_array_text_char():
	vars.add("tp_charptr_1")
	add_field("ctf_array_text", ["char", clever_name(), "tp_charptr_1",
								 str(c_vars["tp_charptr_1_size"][1])])

def gen_tp(provider, name, args, fields):
	return """TRACEPOINT_EVENT(
	{},
	{},
	TP_ARGS({}),
	TP_FIELDS(
		{}
	)
)""".format(provider, name, ", ".join(args), "\n\t\t".join(fields))

fun = [add_ctf_integer_int,
	   add_ctf_integer_long,
	   add_ctf_float_float,
	   add_ctf_float_double,
	   add_ctf_string,
	   add_ctf_sequence_char,
	   add_ctf_sequence_text_char,
	   add_ctf_array_char,
	   add_ctf_array_text_char]

if len(sys.argv) < 2:
	print("Usage: {} num_tp".format(sys.argv[0]), file=sys.stderr)
	exit(1)

num_tp = int(sys.argv[1])
random.seed(10)

def randint():
	return random.randint(-2**31+1, 2**31-1)

c_tp = []

for i in range(1, num_tp+1):
	vars   = set()
	fields = []

	random_string = "".join(random.choice(string.ascii_letters)
							for i in range(random.randint(2, 128)))

	c_vars = { "tp_int_1": ["int", randint()],
			   "tp_int_2": ["int", randint()],
			   "tp_long_1": ["long", randint()],
			   "tp_long_2": ["long", randint()],
			   "tp_float_1": ["float", random.random()],
			   "tp_float_2": ["float", random.random()],
			   "tp_double_1": ["double", random.random()],
			   "tp_double_2": ["double", random.random()],
			   "tp_charptr_1": ["char *", '"{}"'.format(random_string)],
			   "tp_charptr_1_size": ["size_t", len(random_string)] }

	for j in range(random.randint(1,10)):
		random.choice(fun)()

	sorted_vars = list(vars)
	sorted_vars.sort()

	provider = "benchmark_tracepoint_" + str(i)
	tp_name = "random_tp_" + str(i)
	c_args = ", ".join([str(c_vars[name][1]) for name in sorted_vars])
	c_tp.append("tracepoint({}, {}, {});".format(provider, tp_name, c_args))
	tp_args = ["{}, {}".format(c_vars[name][0], name) for name in sorted_vars]

	filename = tp_name + ".tp"
	f = open(filename, "w")
	f.write(gen_tp(provider, tp_name, tp_args, fields))
	f.close()

includes = ["#include \"random_tp_{}.tp.h\"".format(i)
			for i in range(1, num_tp+1)]
filename = "generated_tp.h"
f = open(filename, "w")
f.write("\n".join(includes) + "\n\n")
f.write("#define GENERATED_TP\\\n\t" + "\\\n\t".join(c_tp) + "\n")
f.close()

