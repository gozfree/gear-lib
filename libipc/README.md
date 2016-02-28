##libipc
This is a simple libipc library.
ipc no need to transfer big data?

###Frontend
* Serialize/Deserialize message format
* async I/O data flow
* support 1:1 1:n n:m

###Backend

* POSIX message queue (coding finished)

  usage

  `$ sudo mkdir /dev/mqueue`

  `$ sudo mount -t mqueue none /dev/mqueue/`

  if ipc connection setup, "/dev/mqueue/IPC_SERVER" and "/dev/mqueue/IPC_CLIENT" will be created

  each ipc endpoint contain two message queue: mq_rd and mq_wr


* netlink (coding)

  usage

  `$ make driver=y`

  `$ sudo insmod netlink_driver.ko`


* share memory (TODO)

##IPC server
refer to [ipcd](https://github.com/gozfree/ipcd)
