.PHONY: all demo test
all:
	python3 scripts/build.py
demo: all
	$${UPSILON_ROOT:-../upsilon}/build/upsilon -m 32 --kernel build/bootstrap.elf --bootargs '-s'
test: all
	python3 tests/test_bootstrap.py
