#!/bin/bash

info() {
	echo -e "\E[33m$@\033[0m"
}

error() {
	echo -e "\E[31m$@\033[0m"
}

success() {
	echo -e "\E[32m$@\033[0m"
}

TDIR="$(dirname $0)"
FILES=(`ls $TDIR/*.test`)
i=0
while [ $i -lt ${#FILES[@]} ]; do
	info "Running ${FILES[$i]}"
	res=$(${FILES[$i]} 2>&1)
	if [ $? -ne 0 ]; then
		error "$res"
		exit 1
	fi
	info "Valgrind on ${FILES[$i]}"
	res=$(valgrind --leak-check=full ${FILES[$i]} 2>&1 | awk '/ERROR SUMMARY/ {print $4}')
	if [ $res -gt 0 ]; then
		error "Memory error on ${FILES[$i]}"
		error $(valgrind --leak-check=full ${FILES[$i]})
		exit 1
	fi
	success "Test Passed"
	i=$((i+1))
done
