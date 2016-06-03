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

myset1 = set()
myset2 = set()

def fib(n):
    if n < 2: return 1
    res = fib(n-1) + fib(n-2)
    if n < 15 or res % 2:
        myset1.add(res)
        t = myset2.issubset(myset1)
        t = myset2.issuperset(myset1)
        t = myset2.difference(myset1)
        t = myset2.symmetric_difference(myset1)
        t = 33 in myset1
        tmp = myset2.intersection(myset1)
        tmp = myset2.copy()
    else:
        myset2.add(res)
        t = myset1.issubset(myset2)
        t = myset1.issuperset(myset2)
        t = myset1.difference(myset2)
        t = myset1.symmetric_difference(myset2)
        t = 33 in myset1
        tmp = myset1.intersection(myset2)
        tmp = myset1.copy()
    return res


def test():
    print(fib(30))

threads = 7

if len(sys.argv) > 1:
    threads = int(sys.argv[1])

for i in range(threads - 1):
    threading.Thread(target=test).start()

if threads > 0:
    test()

print(myset1, myset2)
