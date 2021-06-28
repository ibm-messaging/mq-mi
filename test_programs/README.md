# Test Programs

This directory contains test programs to help diagnose issues with MQ Multi Instance queue managers
not behaving as expected.

## test1

This program simulates what MQ does to manage the master lock for a Multi Instance queue manager and monitor
whether the active instance of a queue manager can continue as the active instance,
especially following a failover of an NFS server.

You will need to build the test program, which you can do simply by typing `make` in the
test_programs directory.

You will need to run an instance of the test1 program on each of the two systems where you plan to run
an MQ Multi Instance queue manager.

The one mandatory option to the program is the path to the shared directory where the MQ data and logs for the queue manager will be stored, typically this directory is a mount of an export of an NFS server.
For example, in my testing I ran:
```
./test1 /mnt/MQHA/shared
```

In normal operation, the output you should see from the first instance of the program you run should be
something like:
```
About to attempt to get lock /mnt/MQHA/shared/test1_master on host node-1
Have got the lock at 2021-06-28 09:10:57
Have written lock data "node-1,1918088433,1624867857"
2021-06-28 09:11:07 - data read: "node-1,1918088433,1624867857"
2021-06-28 09:11:17 - data read: "node-1,1918088433,1624867857"
```

This instance of the test program is equivalent to the active instance of an MQ Multi Instance
queue manager, and it
reads the information from the lock file every 10 seconds, and compares the data read with the data
it wrote.

When you run the program with the same option on the second instance you should see something like:
```
About to attempt to get lock /mnt/MQHA/shared/test1_master on host node-2
```

If you do nothing with the NFS server you should not see any additional output from the second instance,
and the first instance should continue to read the same data every 10 seconds or so.

### Test preserving NFS server scope

If you have a simple standalone NFS server or you have an HA NFS server which preserves the NFS server
scope even if the NFS server moves to a different node of an NFS cluster, if you reboot or restart the
NFS server you should not see any impact on either instance of the test program.

This corresponds to the active instance of an MQ Multi Instance queue manager continuing to run on the
same node, and the standby instance continuing to be the standby instance.

### Test with a different NFS server scope

If you have an HA NFS server where the NFS server has a different scope following a failover,
then you will see different behaviour.

When I first built an HA NFS server, when the NFS server moved to a
different node it had a different scope, as the scope was the hostname of the node running the server.

In this case, you will see the first instance of the test program carrying on as if nothing had happened,
something like:
```
./test1 /mnt/MQHA/shared/
About to attempt to get lock /mnt/MQHA/shared/test1_master on host node-1
Have got the lock at 2021-06-28 09:08:47
Have written lock data "node-1,1736844848,1624867727"
2021-06-28 09:08:57 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:09:07 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:09:17 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:09:27 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:09:37 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:09:47 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:09:57 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:10:07 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:10:17 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:10:27 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:10:37 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:10:47 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:10:57 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:11:07 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:11:17 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:11:27 - data read: "node-1,1736844848,1624867727"
2021-06-28 09:11:37 - data read: "node-1,1736844848,1624867727"
```

However, in this case the second instance of the test program will get the lock and will be able to write
its own data and read it back, so each node will think it is still the active instance and will be
reading different data:
```
./test1 /mnt/MQHA/shared/
About to attempt to get lock /mnt/MQHA/shared/test1_master on host node-2
Have got the lock at 2021-06-28 09:10:57
Have written lock data "node-2,1918088433,1624867857"
2021-06-28 09:11:07 - data read: "node-2,1918088433,1624867857"
2021-06-28 09:11:17 - data read: "node-2,1918088433,1624867857"
2021-06-28 09:11:27 - data read: "node-2,1918088433,1624867857"
2021-06-28 09:11:37 - data read: "node-2,1918088433,1624867857"
```

Note that at 09:11:27 for example, each node reads different data.

In MQ, this corresponds to the instance that was the active instance continuing to run,
until it tries to write some data,
while what was the standby instance becomes the active instance.

I think that what is happening is that the data is being returned by the Linux Page Cache,
even when the NFS Client in the Linux kernel does not have the lock.
The following test aims to demonstrate that.

### Test with O_DIRECT

The test program can accept a second option which is one of the values 1, 2 or 3.
If you supply the second option it alters the flags that are used to open the file that is being read:

* If you supply the value 1, then the flag `O_SYNC` is added.

* If you supply the value 2, then `O_DIRECT` is added.

* If you supply the value 3, then both flags are added.

The flag that makes the difference is `O_DIRECT` which should bypass the Page Cache,
so if you run the test program with a second option of 2
you should see that instead of continuing to read the old data, the first instance of the test program
stops with:
```
read: Input/output error
```

## Summary

The ideal behaviour is when the NFS server scope does not change when there is a failover of an HA
NFS server and in that case there is no impact on MQ.

If the NFS server scope changes when there is an HA NFS failover,
the NFS Client continues to return the old data when the file is read,
so an active MQ Multi Instance queue manager instance will continue to think it is the active
instance until the queue
manager attempts to write some data, in which case it will get an I/O error.
Meanwhile, the standby instance will immediately become the active
instance so it can look as if the queue manager is running on both systems at the same time,
but the former active instance will not be able to write to the shared data.