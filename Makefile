.PHONY: all demo test
all:
	python3 build.py
demo: all
	$${UPSILON_ROOT:-../upsilon}/build/upsilon -m 32 --kernel build/bootstrap.elf --bootargs '-s'
test: all
	python3 test_bootstrap.py
