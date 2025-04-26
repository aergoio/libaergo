[![C/C++ CI](https://github.com/aergoio/libaergo/workflows/C/C++%20CI/badge.svg)](https://github.com/aergoio/libaergo/actions)

<p align="center"><img width="50%" src="https://user-images.githubusercontent.com/7624275/92682929-eaede000-f307-11ea-9c08-7be9125795c5.png" alt="libaergo"></p>

This library is used to interface with the Aergo blockchains from different programming languages:

* C
* C++
* C#
* VB.NET
* Swift
* Ruby

Other languages can also use it via FFI.


## Supported OS

* Linux
* Mac
* Windows
* iOS
* Android


## Pre-compiled binaries

You can download binaries for iOS and Windows in the [releases](https://github.com/aergoio/libaergo/releases)


## Dependencies

libaergo depends on [secp256k1-vrf](https://github.com/aergoio/secp256k1-vrf) and [libcurl](https://github.com/curl/curl)

Use the `http1` branch if you want a version that does not depend on libcurl. Subscriptions to events are not possible on that case.

It also depends on [hidapi](https://github.com/libusb/hidapi)
when interfacing with a Ledger Nano S


### Compiling the dependencies

First install the tools required for compilation

On Linux:

```
sudo apt-get install gcc make automake libtool libcurl4-openssl-dev -y
```

On Mac:

```
brew install automake libtool
```

Compiling and installing `secp256k1-vrf`:

```
git clone --depth=1 https://github.com/aergoio/secp256k1-vrf
cd secp256k1-vrf
./autogen.sh
./configure
make
sudo make install
cd ..
```

Compiling and installing `hidapi` (only required if using a Ledger device):

```
sudo apt-get install libudev-dev libusb-1.0-0-dev
git clone --depth=1 https://github.com/libusb/hidapi
cd hidapi
./bootstrap
./configure
make
sudo make install
cd ..
```


## Compiling libaergo

```
make
sudo make install
```

For iOS:

```
cd ../secp256k1-vrf
./makeios
cd -
./makeios
```

## Usage

You can link your application to the external dynamic library.

On some languages you can also include the static library in your project instead of linking to the dynamic library.


## Supported Features

* Get Account State
* Smart Contract Call
* Smart Contract Query
* Smart Contract Query State Variable
* Smart Contract Events Notification
* Transfer
* Multicall

## API

* [Documentation](https://github.com/aergoio/libaergo/wiki)
* [Exported functions](https://github.com/aergoio/libaergo/blob/master/aergo.h)
* [C++ class header](https://github.com/aergoio/libaergo/blob/master/aergo.hpp)


## Examples

There are many [usage examples](https://github.com/aergoio/libaergo/tree/master/examples)
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

### C# and VB.NET

Open the example in a new project, include the [.NET wrapper](https://github.com/aergoio/libaergo/blob/master/wrappers/dotNet/AergoClient.cs)
in the project, then build and execute it.

Or compile it via command line: (Windows)

```
PATH=c:\Windows\Microsoft.NET\Framework\v4.0.30319;%PATH%
csc examples\contract_query\contract_query.cs wrappers\dotNet\AergoClient.cs /r:System.Numerics.dll
```

### Swift

Open the example in a new project, include the [Swift wrapper](https://github.com/aergoio/libaergo/blob/master/wrappers/Swift/aergo.swift)
and the [bridging header](https://github.com/aergoio/libaergo/blob/master/wrappers/Swift/libaergo-Bridging-Header.h)
on the project, then build and execute it.

Or compile it via command line:

```
swiftc examples/contract_query/main.swift wrappers/Swift/aergo.swift -import-objc-header wrappers/Swift/libaergo-Bridging-Header.h -L/usr/local/lib -laergo
```

### Ruby

Install the ffi module:

```
[sudo] gem install ffi
```

To run an example:

```
ruby examples/contract_call/contract_call.rb
```
