
var aergo = Aergo(host: "testnet-api.aergo.io", port: 7845)

print("Sending request...");

var ret = aergo.query_async(
  contractAddress: "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7",
  function: "hello",
  args: [],
  callback:
{ context, success, result in

  if (success) {
    print("Smart Contract Query OK")
    print("Response:", result)
  } else {
    print("FAILED when querying the smart contract")
    print("Response:", result)
  }

})

while( aergo.process_requests(timeout: 5000) > 0 ) { /* loop */ }

print("done")
