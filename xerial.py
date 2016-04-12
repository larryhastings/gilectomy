#!/usr/bin/env python3
#
# xerial.py by Larry Hastings
#
# A serial version of x.py, to compare
# against doing the same work without any
# contention.
#
# Runs a bad fib generator to 30
# some number of times, default 7.

import sys

def fib(n):
    if n < 2: return 1
    return fib(n-1) + fib(n-2)

def test():
    print(fib(30))

iterations = 7

if len(sys.argv) > 1:
    iterations = int(sys.argv[1])

for i in range(iterations):
    test()
