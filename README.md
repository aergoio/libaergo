[![C/C++ CI](https://github.com/aergoio/libaergo/workflows/C/C++%20CI/badge.svg)](https://github.com/aergoio/libaergo/actions)

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

On Linux:

```
sudo apt-get install gcc make automake libtool -y
```

On Mac:

```
brew install automake libtool
```

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


## Supported Features

* Get Account State
* Smart Contract Call
* Smart Contract Query
* Smart Contract Events Notification
* Transfer


## API

* [documentation](https://github.com/aergoio/libaergo/wiki/en---Documentation---C)
* [exported functions](https://github.com/aergoio/herac/blob/master/aergo.h)
* [C++ class header](https://github.com/aergoio/herac/blob/master/aergo.hpp)


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
