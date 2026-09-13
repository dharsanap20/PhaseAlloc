CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -Isrc

LIB_SRC = src/phasealloc.c src/json.c src/json_phase.c

TESTS = test_basic.exe \
        test_alignment.exe \
        test_edge_cases.exe \
        test_overflow.exe \
        test_reset.exe \
        test_destroy.exe \
        test_growth.exe \
        test_peak.exe \
        test_api_contracts.exe \
        test_stats.exe \
        test_json.exe \
        test_json_phase.exe

.PHONY: all test benchmark benchmark-json clean

all:
	$(CC) $(CFLAGS) $(LIB_SRC) examples/basic.c -o basic.exe
	$(CC) $(CFLAGS) $(LIB_SRC) examples/json_phase.c -o json_phase.exe

test: $(TESTS)
	@echo.
	@echo Running PhaseAlloc tests...
	@echo.
	@for %%T in ($(TESTS)) do %%T
	@echo.
	@echo All PhaseAlloc tests completed.

benchmark:
	$(CC) $(CFLAGS) $(LIB_SRC) benchmarks/benchmark.c -o benchmark.exe
	@echo.
	@echo Running PhaseAlloc benchmark...
	@echo.
	benchmark.exe

benchmark-json:
	$(CC) $(CFLAGS) $(LIB_SRC) benchmarks/benchmark_json.c -o benchmark_json.exe
	@echo.
	@echo Running PhaseAlloc JSON benchmark...
	@echo.
	benchmark_json.exe

test_basic.exe:
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_basic.c -o $@

test_alignment.exe:
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_alignment.c -o $@

test_edge_cases.exe:
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_edge_cases.c -o $@

test_overflow.exe:
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_overflow.c -o $@

test_reset.exe:
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_reset.c -o $@

test_destroy.exe:
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_destroy.c -o $@

test_growth.exe:
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_growth.c -o $@

test_peak.exe:
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_peak.c -o $@

test_api_contracts.exe:
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_api_contracts.c -o $@

test_stats.exe:
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_stats.c -o $@

test_json.exe:
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_json.c -o $@

test_json_phase.exe:
	$(CC) $(CFLAGS) $(LIB_SRC) tests/test_json_phase.c -o $@

clean:
	del /Q *.exe 2>NUL