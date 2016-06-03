GILECTOMY README
================

Welcome to the exciting world of segfaults and massive slowdowns!

Some things you need to know in order to work with the Gilectomy.

First, this branch is known to minimally work on 64-bit platforms
only, Linux and OS X.  Work is underway for Win64.  We have no idea
whether or not it works on 32-bit platforms.  Boldly go!  Forge the
new frontier!  Let us know!

Second, as you hack on the Gilectomy you may break your "python"
executable rather badly.  This is of course expected.  However, the
python Makefile itself depends on having a working local python
interpreter, so when you break that you often break your build too.
We provide a workaround available for this problem: "fix_makefile.py"
in the root of the Gilectomy branch.  You should run this with a
known-good python excecutable *from circa the time the Gilectomy was
forked from CPython*.  There's a git "tag", "gilectomy_base", on the
correct revision.  Once that's built, run this:

    % cd gilectomy_branch_checkout
    % ./configure
    % ../path/to/gilectomy_base/python fix_makefile.py
    % make
