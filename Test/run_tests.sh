#!/bin/bash

TDIR="$(dirname $0)"
for t in $(ls $TDIR/*.test); do
	./$t
done
