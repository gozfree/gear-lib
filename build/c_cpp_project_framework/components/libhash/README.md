## libhash
This is a simple libhash library, based on hlist from kernel list.

libhash vs libdict:

```
./test_libdict 10000000
         values: 10000000
 initialization: 1.1839 sec
         adding: 5.3447 sec
         lookup: 2.1571 sec
         delete: 2.3988 sec
         adding: 5.4214 sec
           free: 1.3423 sec

./test_libhash 10000000
         values: 10000000
 initialization: 1.2027
         adding: 5.8076
         lookup: 4.9835 //need optimize
         delete: 5.3372 //need optimize
         adding: 5.4309
           free: 1.6785
```
TODO: need optimize libhash for big data, because of hlist said:

```
/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */
```


hash functions refer to

https://en.wikipedia.org/wiki/Jenkins_hash_function

https://en.wikipedia.org/wiki/MurmurHash 

https://github.com/google/cityhash

http://burtleburtle.net/bob/hash/spooky.html

