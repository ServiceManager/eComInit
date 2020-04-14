Workflow
========

System XVI is simple to build and work with. This section of the Developers'
Handbook explains how to compile S16, make changes, and submit them for
inclusion.

Dependencies
~~~~~~~~~~~~

System XVI has few dependencies, but does require some in order to be built.
Some of them are only neededed on particular platforms and others are totally
optional. They are all detailed below:

Required: All Platforms
***********************
- readline-devel

Required: GNU/Linux
*******************
- procpsng-devel
- libkqueue-devel

Optional: All Platforms
***********************
- Java JDK 1.8 or later (for the S16 Java library)
- Python 3 (for the pre-commit checks)

Getting the Source Tree
~~~~~~~~~~~~~~~~~~~~~~~

The System XVI source tree is managed by the Git version control system. You
will need it installed in order to check out the source code. If you wish to
make changes to be submitted for integration, use GitHub to create a fork of
the source tree and clone that. Otherwise, follow the instructions on the
System XVI GitHub page to clone the repository.

Having cloned the repository, you must now set up the submodules. These are
subtrees of the source tree which contain sources from other projects which
S16 makes use of. Run this command to do so:

::

  git submodule update --init --recursive


Building System XVI
~~~~~~~~~~~~~~~~~~~

System XVI leverages CMake for building. In order to build System XVI, you need
to create a folder in which to build (for example, make a subfolder of the S16
tree called *build*), enter that directory, and then run:

::

  cmake ..

After this, in order to build the sources, run:

::

  make


This should run without any errors being generated. If any do appear, please
make an Issue on the S16 GitHub page.