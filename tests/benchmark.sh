#!/usr/bin/env sh

set -e

cd "$(dirname "$0")/.."

if [ $# -ne 1 ]
then
	printf 'Type "%s --help" for usage.\n' "$(basename "$0")" 1>&2
	exit 1
fi
case "$1" in
	--help)
		printf 'This is a simple script which helps in using results `tests/run.py` for benchmarking.\n'
		printf '(It just save them to files and compares them with `vimdiff`.)\n'
		printf '(You should close all other programs for more precise results.)\n'
		printf '\n'
		printf 'Usage:\t%s --help\n' "$(basename "$0")"
		printf '\t%s MODE\n' "$(basename "$0")"
		printf 'Modes:\n'
		printf '\tretry\t- removes the previous result and compares with the old result\n'
		printf '\tshift\t- replaces the old result with the previous result and compares the new result with it\n'
		exit 0
		;;
	shift)
		if [ -e benchmark-result.new ]
		then
			mv benchmark-result.new benchmark-result.old
		fi
		;;
	retry)
		;;
	*)
		printf 'Unknown mode!\n' 1>&2
		exit 1
esac
scons -Q
scons -Q test 2>&1 | tee benchmark-result.new
vimdiff benchmark-result.old benchmark-result.new
