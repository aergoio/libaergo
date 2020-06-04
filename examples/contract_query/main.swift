
var aergo = Aergo(host: "testnet-api.aergo.io", port: 7845)

print("Sending request...");

var ret = aergo.query(
    contractAddress: "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7",
    function: "hello",
    args: [])

if (ret.success) {
  print("Smart Contract Query OK")
  print("Response:", ret.result)
} else {
  print("FAILED when querying the smart contract")
  print("Response:", ret.result)
}
