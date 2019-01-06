libipc
======

This is a simple libipc library.
ipc no need to transfer big data?

## Frontend

* Serialize/Deserialize message format
* async I/O data flow
* support 1:1 1:n n:m

## Backend
libipc backend support posix mqueue, sysv mqueue, netlink, share memory and unix socket

* POSIX message queue  
  usage  
  `$ sudo mkdir /dev/mqueue`  
  `$ sudo mount -t mqueue none /dev/mqueue/`  
  if one ipc connection setup, "/dev/mqueue/IPC_SERVER" and "/dev/mqueue/IPC_CLIENT"  
  will be created, each ipc endpoint contain two message queue: mq_rd and mq_wr  

* SYSV message queue  
  usage  
  `$ ipcs -q`  
  `$ ipcrm -q <id>`  

* netlink  
  usage  
  `$ make driver=y`  
  `$ sudo insmod netlink_driver.ko`  

* share memory (coding)

* unix domain socket  

## IPC server
refer to [ipcd](https://github.com/gozfree/ipcd)
