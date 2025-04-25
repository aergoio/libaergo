var aergo = Aergo(host: "testnet-api.aergo.io", port: 7845)

print("Sending request...");

var ret = aergo.query_state_variable(
    contractAddress: "AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe",
    stateVariable: "Name")

if (ret.success) {
  print("Smart Contract State Variable Query OK")
  print("Response:", ret.result)
} else {
  print("FAILED when querying the smart contract state variable")
  print("Response:", ret.result)
}
