# Test hôte de la rotation de logs (sysmodule/source/log.c).
# Compile log.c avec -DLOG_HOST_TEST pour exposer les helpers d'introspection.
CC     ?= cc
CFLAGS := -O1 -g -Wall -Werror -DLOG_HOST_TEST -I../../sysmodule/source

log_test: main.c ../../sysmodule/source/log.c
	$(CC) $(CFLAGS) $^ -o $@

check: log_test
	rm -rf /tmp/tricord-log-test
	mkdir -p /tmp/tricord-log-test
	TRICORD_LOG_DIR=/tmp/tricord-log-test ./log_test

clean:
	rm -f log_test
	rm -rf /tmp/tricord-log-test

.PHONY: check clean
