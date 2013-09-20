tiny-staq
=========


Overview
--------

**tiny-staq** is a software stack for post-processing in quantum key distribution
(QKD). This python implementation is intended for educational purposes only and
comes without any warranty! If you need a production ready fast and high quality 
software stack, please download `ait-stack`_ and optionally ask for 
professional support.

.. _ait-stack: https://sqt.ait.ac.at/software/projects/qkd-software


Installation:
-------------

1) Build pycodes:

.. code-block:: shell

  > cd pycodes
  > python setup.py build
  > cd ..

2) Generate key files

.. code-block:: shell

  > python gen_raw_key.py

3) Have staq fun:

.. code-block:: shell

  > ./bob.py -v &
  > ./alice -v


Windows Users: Use Python X,Y for easy install.


Acknowledgements
----------------

- The software uses the LDPC decoder from the pycodes package. 
  (http://web.mit.edu/~emin/www.old/source_code/pycodes/)

- LDPC codes are taken from the fabulous and production ready QKD stack from
  AIT, which is open source (https://sqt.ait.ac.at/software/projects/qkd-software)


License
-------

See `LICENSE <LICENSE.rst>`_


Keywords
--------

qkd, qkd-stack, qkd protocol stack, python qkd stack, python stack
