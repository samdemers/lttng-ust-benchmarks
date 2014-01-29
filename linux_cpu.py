#!/usr/bin/env python3

import sys

class _LinuxCpu(object):

	def _read_cpu_file(self, filename):
		f = open(filename, "r");
		cpus = f.read().rstrip()
		return set() if len(cpus) == 0 else {int(n) for n in cpus.split("-")}

	def _set_cpu_online(self, cpu_id, online):
		f = open("/sys/devices/system/cpu/cpu{}/online".format(cpu_id), "w")
		f.write("1" if online else "0")

	def _set_cpus_online(self, cpus, online):
		for cpu in cpus:
			self._set_cpu_online(cpu, online)

	@property
	def online_cpus(self):
		"""Set of CPUs currently online. Setting this property enables and
		disables CPUs as needed."""
		return self._read_cpu_file("/sys/devices/system/cpu/online")

	@online_cpus.setter
	def online_cpus(self, cpus):
		self._set_cpus_online(self.online_cpus - cpus, False)
		self._set_cpus_online(self.offline_cpus & cpus, True)

	@property
	def offline_cpus(self):
		"""Set of CPUs currently offline. Setting this property enables and
		disables CPUs as needed."""
		return self._read_cpu_file("/sys/devices/system/cpu/offline")

	@offline_cpus.setter
	def offline_cpus(self, cpus):
		self._set_cpus_online(self.online_cpus & cpus, False)
		self._set_cpus_online(self.offline_cpus - cpus, True)

	@property
	def online_count(self):
		"""Number of CPUs currently online. Setting this property enables and
		disables CPUs as needed."""
		return len(self.online_cpus)

	@online_count.setter
	def online_count(self, count):
		online_cpus = self.online_cpus
		offline_cpus = self.offline_cpus
		cpus_to_enable = count - len(online_cpus)
		if cpus_to_enable > len(offline_cpus):
			raise Exception("cannot not get {} CPUs online: only {} more are"
							" available".format(count, len(offline_cpus)))
		elif count < 1:
			raise Exception("cannot get less than one CPU online")
		elif cpus_to_enable < 0:
			for cpu_id in sorted(list(online_cpus))[cpus_to_enable:]:
				self._set_cpu_online(cpu_id, False)
		else:
			for cpu_id in sorted(list(offline_cpus))[:cpus_to_enable]:
				self._set_cpu_online(cpu_id, True)

		new_count = len(self.online_cpus)
		if (new_count != count):
			raise Exception("got {} CPUs online instead of {}"
							.format(new_count, count))

sys.modules[__name__] = _LinuxCpu()
