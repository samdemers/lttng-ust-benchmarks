#undef TRACE_SYSTEM
#define TRACE_SYSTEM lttng_benchmark

#if !defined(_TRACE_TRIVIAL_TP_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_TRIVIAL_TP_H

#include <linux/tracepoint.h>

TRACE_EVENT(lttng_benchmark_trivial,
	TP_PROTO(int arg),
	TP_ARGS(arg),
	TP_STRUCT__entry(
		__field(	int, arg	)
	),
	TP_fast_assign(
		tp_assign(arg, arg)
	),
	TP_printk("%d", __entry->arg)
)

#endif /* _TRACE_TRIVIAL_TP_H */

/* This part must be outside protection */
#include "probes/define_trace.h"
