#!/usr/bin/env python3
#
# x.py by Larry Hastings
#
# Official test program of the Gilectomy.
#
# Runs a bad fib generator to 30,
# with some number of threads, default 7.

import threading
import sys

def fib(n):
    if n < 2: return 1
    return fib(n-1) + fib(n-2)


def test():
    print(fib(30))

threads = 7

if len(sys.argv) > 1:
    threads = int(sys.argv[1])

for i in range(threads - 1):
    threading.Thread(target=test).start()

if threads > 0:
    test()
