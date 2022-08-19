# CryptoLW-RK

*Port of the lightweight rweather Arduino Cryptography Library (CryptoLW) for Particle*

- [Original library Github](https://github.com/rweather/arduinolibs)
- [Documentation](http://rweather.github.io/arduinolibs/crypto.html)

## Tests

### 1-TestAcorn

Tested on a Photon running Device OS 1.0.0:

```
State Size ... 64

Test Vectors:
Acorn128 #1 ... Passed
Acorn128 #2 ... Passed
Acorn128 #3 ... Passed
Acorn128 #4 ... Passed
Acorn128 #5 ... Passed

Performance Tests:
Acorn128 #4 SetKey ... 94.36us per operation, 10597.37 per second
Acorn128 #4 Encrypt ... 0.38us per byte, 2638304.89 bytes per second
Acorn128 #4 Decrypt ... 0.40us per byte, 2480331.74 bytes per second
Acorn128 #4 AddAuthData ... 0.43us per byte, 2348968.66 bytes per second
Acorn128 #4 ComputeTag ... 68.21us per operation, 14661.25 per second
```

### 2-TestAscon

Tested on a Photon running Device OS 1.0.0:

```
State Size ... 72

Test Vectors:
Ascon128 #1 ... Passed
Ascon128 #2 ... Passed
Ascon128 #3 ... Passed
Ascon128 #4 ... Passed
Ascon128 #5 ... Passed

Performance Tests:
Ascon128 #4 SetKey ... 33.40us per operation, 29940.12 per second
Ascon128 #4 Encrypt ... 2.22us per byte, 450450.45 bytes per second
Ascon128 #4 Decrypt ... 2.21us per byte, 453219.27 bytes per second
Ascon128 #4 AddAuthData ... 2.15us per byte, 465403.77 bytes per second
Ascon128 #4 ComputeTag ... 34.54us per operation, 28951.10 per second
```



### 3-TestSpeck

Tested on a Photon running Device OS 1.0.0:

```
State Sizes:
Speck ... 288
SpeckSmall ... 80
SpeckTiny ... 48

Speck Test Vectors:
Speck-128-ECB Encryption ... Passed
Speck-128-ECB Decryption ... Passed
Speck-192-ECB Encryption ... Passed
Speck-192-ECB Decryption ... Passed
Speck-256-ECB Encryption ... Passed
Speck-256-ECB Decryption ... Passed

SpeckSmall Test Vectors:
Speck-128-ECB Encryption ... Passed
Speck-128-ECB Decryption ... Passed
Speck-192-ECB Encryption ... Passed
Speck-192-ECB Decryption ... Passed
Speck-256-ECB Encryption ... Passed
Speck-256-ECB Decryption ... Passed

SpeckTiny Test Vectors:
Speck-128-ECB Encryption ... Passed
Speck-192-ECB Encryption ... Passed
Speck-256-ECB Encryption ... Passed

Speck Performance Tests:
Speck-128-ECB Set Key ... 16.98us per operation, 58890.73 per second
Speck-128-ECB Encrypt ... 0.48us per byte, 2090738.03 bytes per second
Speck-128-ECB Decrypt ... 0.44us per byte, 2275507.01 bytes per second

Speck-192-ECB Set Key ... 17.45us per operation, 57305.28 per second
Speck-192-ECB Encrypt ... 0.49us per byte, 2033967.25 bytes per second
Speck-192-ECB Decrypt ... 0.45us per byte, 2213552.48 bytes per second

Speck-256-ECB Set Key ... 18.04us per operation, 55433.29 per second
Speck-256-ECB Encrypt ... 0.50us per byte, 1981915.03 bytes per second
Speck-256-ECB Decrypt ... 0.46us per byte, 2155578.91 bytes per second

SpeckSmall Performance Tests:
Speck-128-ECB Set Key ... 14.11us per operation, 70878.25 per second
Speck-128-ECB Encrypt ... 1.88us per byte, 532527.44 bytes per second
Speck-128-ECB Decrypt ... 0.99us per byte, 1009909.74 bytes per second

Speck-192-ECB Set Key ... 15.11us per operation, 66182.21 per second
Speck-192-ECB Encrypt ... 1.97us per byte, 507939.73 bytes per second
Speck-192-ECB Decrypt ... 1.03us per byte, 973176.81 bytes per second

Speck-256-ECB Set Key ... 16.09us per operation, 62140.36 per second
Speck-256-ECB Encrypt ... 2.05us per byte, 487742.42 bytes per second
Speck-256-ECB Decrypt ... 1.07us per byte, 938526.51 bytes per second

SpeckTiny Performance Tests:
Speck-128-ECB Set Key ... 0.51us per operation, 1979414.09 per second
Speck-128-ECB Encrypt ... 1.88us per byte, 532435.29 bytes per second

Speck-192-ECB Set Key ... 0.61us per operation, 1650437.37 per second
Speck-192-ECB Encrypt ... 1.96us per byte, 509071.01 bytes per second

Speck-256-ECB Set Key ... 0.74us per operation, 1348072.26 per second
Speck-256-ECB Encrypt ... 2.05us per byte, 487709.72 bytes per second
```

The flash usage is quite reasonable for the Speck example:

```
Memory use: 
   text	   data	    bss	    dec	    hex	filename
  11068	    108	   1820	  12996	   32c4	/workspace/target/workspace.elf
```

----

## Original Read Me

Arduino Cryptography Library
============================

This distribution contains a libraries and example applications to perform
cryptography operations on Arduino devices.  They are distributed under the
terms of the MIT license.

The [documentation](http://rweather.github.com/arduinolibs/crypto.html)
contains more information on the libraries and examples.

This repository used to contain a number of other examples and libraries
for other areas of Arduino functionality but most users are only interested
in the cryptography code.  The other projects have been moved to a
separate [repository](https://github.com/rweather/arduino-projects) and
only the cryptography code remains in this repository.

For more information on these libraries, to report bugs, or to suggest
improvements, please contact the author Rhys Weatherley via
[email](mailto:rhys.weatherley@gmail.com).

Recent significant changes to the library
-----------------------------------------

Apr 2018:

* Acorn128 and Ascon128 authenticated ciphers (finalists in the CAESAR AEAD
  competition in the light-weight category).
* Split the library into Crypto (core), CryptoLW (light-weight), and
  CryptoLegacy (deprecated algorithms).
* Tiny and small versions of AES for reducing memory requirements.
* Port the library to ESP8266 and ESP32.
* Make the RNG class more robust if the app doesn't call begin() or loop().

Nov 2017:

* Fix the AVR assembly version of Speck and speed it up a little.
* API improvements to the RNG class.


----

## Original License

```
/*
 * Copyright (C) 2012 Southern Storm Software, Pty Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
 ```
