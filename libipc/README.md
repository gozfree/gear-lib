##libipc
This is a simple libipc library.
Among processes 

###Frontend
* Serialize/Deserialize message format
* async I/O data flow
* support 1:1 1:n n:m

###Backend

* netlink

* message queue
**sudo mkdir /dev/mqueue
**sudo mount -t mqueue none /dev/mqueue/

* share memory
