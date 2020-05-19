# libaergo

This library is used to interface with the Aergo blockchains
from C, C++ and even other languages.


## Supported OS

* Linux
* Mac
* Windows
* iOS
* Android


## Compiling

```
make
sudo make install
```


## Dependencies

It depends on [secp256k1-vrf](https://github.com/aergoio/secp256k1-vrf)
which can be installed with:

```
git clone --depth=1 https://github.com/aergoio/secp256k1-vrf
cd secp256k1-vrf
./autogen.sh
./configure
make
sudo make install
cd ..
```


## Usage

You can link your application to the external dynamic library.

On some languages you can also include the static library in your project instead of linking to the dynamic library.


## Private keys

For now your application is responsible for generating a true random private key, storing it into persistent storage
and loading it into the account object.


## Supported Features

* Get Account State
* Smart Contract Call
* Smart Contract Query
* Smart Contract Events Notification
* Transfer


## API

While the documentation is not ready please check the
[exported functions](https://github.com/aergoio/herac/blob/master/aergo.h),
the 
[C++ class header](https://github.com/aergoio/herac/blob/master/aergo.hpp)
and the examples.


## Examples

There are many [usage examples](https://github.com/aergoio/herac/tree/master/examples)
available for each supported language, for synchronous and asynchronous calls.

Compiling an example code:

### C

```
gcc examples/contract_call/contract_call.c -laergo -o contract_call
```

### C++

```
g++ examples/contract_call/contract_call.cpp -std=c++17 -laergo -o contract_call
```
