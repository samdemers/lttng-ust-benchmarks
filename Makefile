BIN        = lttng-ust-benchmarks basic-benchmark basic-benchmark-ust \
             sha2-benchmark sha2-benchmark-ust
CFLAGS    += -std=gnu99 -g -O2 -Wall -D_GNU_SOURCE -D_LGPL_SOURCE -DNDEBUG
LDFLAGS   += -lrt -ldl
UST_FLAGS  = -DWITH_UST -llttng-ust -lurcu-bp
ARTIFACTS  = message.tp.c message.tp.h sum.tp.c sum.tp.h \
             benchmarks.json jenkins_plot_data

.PHONY: all clean

all: $(BIN)

lttng-ust-benchmarks: lttng-ust-benchmarks.c shared_events.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

basic-benchmark: basic-benchmark.c shared_events.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

basic-benchmark-ust: basic-benchmark.c shared_events.o sum.tp.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(UST_FLAGS)

sha2-benchmark: sha2-benchmark.c shared_events.o sha2.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

sha2-benchmark-ust: sha2-benchmark.c shared_events.o message.tp.o sha2-ust.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(UST_FLAGS)

sha2-ust.o: sha2.c
	$(CC) -c -o $@ $^ $(CFLAGS) $(LDFLAGS) $(UST_FLAGS)

sha2.o: sha2.c
	$(CC) -c -o $@ $^ $(CFLAGS) $(LDFLAGS)

# Override implicit rules.
%.tp:;

%.tp.o: %.tp
	CFLAGS="$(CFLAGS)" lttng-gen-tp $^ -o $<.o -o $<.c -o $<.h

clean:
	rm -rf $(BIN) $(ARTIFACTS) *.o
