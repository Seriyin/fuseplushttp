# fuseplushttp #

## Authors ##

- André Diogo - A75505@alunos.uminho.pt
- António Silva - A73287@alunos.uminho.pt
- Seihakrith Tan - E8030@alunos.uminho.pt

---------------------------

## Pre-requisites ##

1. FUSE kernel dev library (use package manager to get libfuse-dev)
2. FUSE user library (compile via meson+ninja the libfuse library from https://github.com/libfuse/libfuse)
3. libmicrotthpd-dev installed (use package manager)
4. FUSE user library is registered with pkg-config (check pkg-config --modversion fuse3).
5. Port 3001 available for listening on localhost for http requests
6. Run make using Makefile provided
7. Run ourfs -f [mountpoint] where mountpoint specifies a path to an existing directory
8. Probably would only work on Linux.
--------------------------------------------------------------------------------------

## What is this File System? ##

**_You can run ourfs --help for additional info._**

------------------------------------------------------------------

This file system mounts a single file, whose name can be user specified via the argument _name=[name]_.

On mounting, a minimal http server running on port 3001 will be listening for a connection.

On root there will be a string of text to use in the URL.

Navigating to the URL with said string will allow the user to open the file.

This file system does not contemplate simultaneous users or multi-machine setups.

------------------------------------------------------------------------------

## Example ##

*in the terminal*

>./ourfs -f .

>cat ./access

--------------------------------------------------------------------

*in the browser bar*

>localhost:3001/

Record the string of text that appears (Example: uhjeifle12k3j4)

>localhost:3001/uhjeifle12k3j4

---------------------------------------------------------------------

*in the terminal*

>The file was opened correctly, Congrats!

----------------------------------------------------------------------

## WIP ##

Currently explodes on a pipe read from the http server side with error EIO.

Is supposed to transfer the unique string of text from ourfs to the http server via pipe.

## TODO ##

Substitute Pipes with sockets.