#!/usr/bin/env python3

usage_str = """
fix_makefile.py
Copyright 2016 by Larry Hastings

usage: fix_makefile.py [-h] [path-to-cpython-trunk] [path-to-python-binary]
"""

docs = """
CPython's build process itself depends on the python interpreter
that it builds.  So when you break the CPython interpreter as much
as I have, you discover that the build breaks too...!

This script modifies CPython's Makefile (in the current directory)
to use a different interpreter.  The default is my "known good"
interpreter I build from trunk now and then.  You can specify your
own, though you must specify the path to the directory where you
built a recent CPython trunk.

Why must it be a trunk you built yourself?  It also needs the
_freeze_importlib program, which uses the CPython .so, and
isn't installed when you install python3.  And anyway that's all
black magic and I don't know if something bad happens if
_freeze_importlib is built from a very different revision of trunk.
So I just keep my "buildtrunk" at a reasonably similar version and
hope for the best.  Good luck!
"""


import os
import stat
import sys

def error(s, path=None):
    print(s)
    if (path):
        print("   ", path)
    print(usage_str)
    sys.exit(-1)

if "-h" in sys.argv[1:]:
    print(usage_str)
    print(docs.strip())
    sys.exit(0)


if len(sys.argv) > 3:
    error("Your command-line parameters are wrong and bad!  Shame on you!")

if len(sys.argv) == 3:
    path_to_python = sys.argv[2]
else:
    path_to_python = sys.executable

if len(sys.argv) >= 2:
    path_to_trunk = sys.argv[1]
else:
    path_to_trunk = "."


if ((sys.version_info.major < 3) 
    or (sys.version_info.minor < 6)):
    error("I need Python 3.6+.")

for fn in (
    os.path.normpath,
    os.path.expanduser,
    os.path.abspath,
    ):
    path_to_trunk = fn(path_to_trunk)

if not os.path.isdir(path_to_trunk):
    error("You must specify a directory:", path_to_trunk)


def test_exe(path, name):
    try:
        s = os.stat(path)
    except FileNotFoundError:
        error('Path to {} doesn\'t exist:'.format(name), path)

    if not (s.st_mode & stat.S_IXUSR):
        error('{} specified isn\'t executable:'.format(name), path)

test_exe(path_to_python, "python")

path_to_freeze = os.path.join(os.path.dirname(path_to_python),
    "Programs/_freeze_importlib")

test_exe(path_to_freeze, "_freeze_importlib")

print('Path to  trunk: "{}"'.format(path_to_trunk))
print('Path to python: "{}"'.format(path_to_python))
print('Path to freeze: "{}"'.format(path_to_freeze))
print()

if not os.path.isfile("Makefile"):
    error("Makefile not found in current directory!")

with open("Makefile", "rt") as f:
    makefile = f.read().split("\n")


for s, replacement, expected in (
    (
        "PYTHON_FOR_BUILD=./$(BUILDPYTHON) -E",
        "PYTHON_FOR_BUILD={} -E".format(path_to_python),
        1
    ),
    (
        "./Programs/_freeze_importlib \\",
        "{} \\".format(path_to_freeze),
        2
    ),
    ):
    lines = []
    found = 0
    for line in makefile:
        if line.endswith(s):
            found += 1
            line = line.replace(s, replacement)
        lines.append(line)
    if found != expected:
        error("Didn't find the number of expected substitutions for line!", s)
    makefile = lines


with open("Makefile", "wt") as f:
    f.write("\n".join(makefile))

print("Success!")
sys.exit(0)
