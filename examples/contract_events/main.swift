
var aergo = Aergo(host: "testnet-api.aergo.io", port: 7845)

print("Waiting for smart contract events...");

var ret = aergo.contract_events_subscribe(
  contractAddress: "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7",
  eventName: "",
  callback:
{ context, event in

  print("--- New Event ---");
  print("contractAddress:", event.contractAddress);
  print("eventName:", event.eventName);
  print("jsonArgs:", event.jsonArgs);
  print("eventIdx:", event.eventIdx);
  print("blockNo:", event.blockNo);
  print("txIndex:", event.txIndex);

})

while( aergo.process_requests(timeout: 5000) > 0 ) { /* loop */ }

print("done")
