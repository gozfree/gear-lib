##libgevent
The libgevent is a simple event library refered to libevent,

##How To Use
Just like libevent, but more easily to use.
```
	base = gevent_base_create()
	event = gevent_create()
	gevent_add(base, event)
	gevent_base_loop()
```

##TODO
  now select/poll backend can't be used until the fd/event hash table achieved

  event timer
