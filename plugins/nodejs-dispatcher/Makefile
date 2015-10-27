all:
	find -name '*.ts' -type f -exec tsc -m commonjs {} ';'

.PHONY: clean
clean:
	find -name '*.js' -type f -exec $(RM) {} ';'
