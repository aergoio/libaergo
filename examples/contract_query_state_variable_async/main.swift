var aergo = Aergo(host: "testnet-api.aergo.io", port: 7845)

print("Sending request...");

var ret = aergo.query_state_variable_async(
  contractAddress: "AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe",
  stateVariable: "Name",
  callback:
{ context, success, result in

  if (success) {
    print("Smart Contract State Variable Query OK")
    print("Response:", result)
  } else {
    print("FAILED when querying the smart contract state variable")
    print("Response:", result)
  }

})

while( aergo.process_requests(timeout: 5000) > 0 ) { /* loop */ }

print("done")
