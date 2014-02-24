#include <linux/completion.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/slab.h>			

#define CREATE_TRACE_POINTS
#define TRACE_INCLUDE_PATH .
#include "lttng_benchmark.h"

static struct kobject *lttng_benchmark_kobj;

static unsigned int default_count = 1000;
module_param(default_count, uint, 0644);
MODULE_PARM_DESC(default_count, "# of events generated");

static unsigned int event_count;

struct benchmark_thread {
	struct task_struct *thread;
	struct completion *prev;
	struct completion end;
	unsigned long long time;
};

static int benchmark_thread(void *arg)
{
	struct benchmark_thread *thread = (struct benchmark_thread *)arg;
	struct timeval start_tv;
	struct timeval end_tv;
	unsigned long long time;
	int i;

	wait_for_completion_interruptible(thread->prev);

	do_gettimeofday(&start_tv);
	for (i = 0; i < event_count; i++)
		trace_lttng_benchmark_trivial(i);
	do_gettimeofday(&end_tv);

	time = end_tv.tv_sec - start_tv.tv_sec;
	time *= USEC_PER_SEC;
	time += (long long)((long)end_tv.tv_usec - (long)start_tv.tv_usec);

	thread->time = time;

	complete(&thread->end);

	return 0;
}

static ssize_t time_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	DECLARE_COMPLETION_ONSTACK(main_start);
	struct benchmark_thread *threads;
	unsigned int cpu;
	unsigned int online_cpus = 0;
	unsigned long long time = 0;

	for_each_online_cpu(cpu) {
		online_cpus++;
	}

	printk("lttng_benchmark: starting benchmark on %u cpus\n",
	       online_cpus);

	threads = (struct benchmark_thread *)
		kmalloc(online_cpus * sizeof(struct benchmark_thread),
			GFP_KERNEL);

	for_each_online_cpu(cpu) {
		init_completion(&threads[cpu].end);
		threads[cpu].time = 0;
	}

	for_each_online_cpu(cpu) {
		threads[cpu].prev = &main_start;
		threads[cpu].thread = kthread_run(benchmark_thread,
						  &threads[cpu],
						  "lttng_benchmark");
	}

	complete_all(&main_start);

	for_each_online_cpu(cpu) {
		wait_for_completion_interruptible(&threads[cpu].end);
	}
	for_each_online_cpu(cpu) {
		unsigned long long cpu_time = threads[cpu].time;
		printk("lttng_benchmark: time for CPU#%u: %llu\n",
		       cpu, cpu_time);
		time += cpu_time;
	}

	kfree(threads);

	printk("lttng_benchmark: total time: %llu\n", time);
	return sprintf(buf, "%llu\n", time);
}

static ssize_t count_show(struct kobject *kobj, struct kobj_attribute *attr,
			  char *buf)
{
	return sprintf(buf, "%u\n", event_count);
}

static ssize_t count_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	sscanf(buf, "%u", &event_count);
	return count;	
}

static ssize_t events_show(struct kobject *kobj, struct kobj_attribute *attr,
			   char *buf)
{
	return sprintf(buf, "lttng_benchmark_trivial\n");
}

static struct kobj_attribute time_attribute =
	__ATTR(time, 0444, time_show, NULL);

static struct kobj_attribute count_attribute =
	__ATTR(count, 0644, count_show, count_store);

static struct kobj_attribute events_attribute =
	__ATTR(events, 0444, events_show, NULL);

static struct attribute *attrs[] = {
	&time_attribute.attr,
	&count_attribute.attr,
	&events_attribute.attr,
	NULL
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static int __init lttng_benchmark_init(void)
{
	int retval;

	event_count = default_count;

	lttng_benchmark_kobj = kobject_create_and_add
		("lttng_benchmark", kernel_kobj);

	if (!lttng_benchmark_kobj)
		return -ENOMEM;

	retval = sysfs_create_group(lttng_benchmark_kobj, &attr_group);
	if (retval)
		kobject_put(lttng_benchmark_kobj);

	return retval;
}
 
static void __exit lttng_benchmark_cleanup(void)
{
	kobject_put(lttng_benchmark_kobj);
}

module_init(lttng_benchmark_init);
module_exit(lttng_benchmark_cleanup);

MODULE_LICENSE("GPL and additional rights");
MODULE_AUTHOR("Samuel Demers <samuel@samdemers.com>");
MODULE_DESCRIPTION("LTTng benchmark");
