## librpc
This is a simple RPC(Remote Procedure Call) library.

## Frontend

* Serialize/Deserialize message format
* async I/O data flow
* support 1:1 1:n n:m

## Backend
librpc backend support posix mqueue, sysv mqueue, netlink, share memory and unix socket

* POSIX message queue  
  usage  
  `$ sudo mkdir /dev/mqueue`  
  `$ sudo mount -t mqueue none /dev/mqueue/`  
  if one rpc connection setup, "/dev/mqueue/IPC_SERVER" and "/dev/mqueue/IPC_CLIENT"  
  will be created, each rpc endpoint contain two message queue: mq_rd and mq_wr  

* SYSV message queue  
  usage  
  `$ rpcs -q`  
  `$ rpcrm -q <id>`  

* netlink  
  usage  
  `$ make driver=y`  
  `$ sudo insmod netlink_driver.ko`  

* share memory (coding)

* unix domain socket  

##RPC server
refer to [rpcd](https://github.com/gozfree/rpcd)

