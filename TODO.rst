
TODO
====


Bugs:
-----

* Find memory leak in ldpc.get_beliefs()


Features:
---------

* Change key passing to numpy array (saves conversions)
* Make reader own thread
* Flexible blocklength (currently only ecc code length supported)
* Cleanup useage of globals
* Use logging facility
* Documentation


Future Release:
---------------

* Add asyncronous mode to increase speed and avoid handshake delays
* Add multithreading for ldpc decoder

