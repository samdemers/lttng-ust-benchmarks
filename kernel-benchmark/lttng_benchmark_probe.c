#include <linux/module.h>

#define LTTNG_PACKAGE_BUILD
#define CREATE_TRACE_POINTS
#define TRACE_INCLUDE_PATH .
#include "lttng_benchmark.h"

MODULE_LICENSE("GPL and additional rights");
MODULE_AUTHOR("Samuel Demers <samuel@samdemers.com>");
MODULE_DESCRIPTION("LTTng benchmark probe");
