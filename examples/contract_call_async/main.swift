
var aergo = Aergo(host: "testnet-api.aergo.io", port: 7845)


var account = AergoAccount()

/* set the private key */
account.privkey = [UInt8](arrayLiteral:
        0xDB, 0x85, 0xDD, 0x0C, 0xBA, 0x47, 0x32, 0xA1,
        0x1A, 0xEB, 0x3C, 0x7C, 0x48, 0x91, 0xFB, 0xD2,
        0xFE, 0xC4, 0x5F, 0xC7, 0x2D, 0xB3, 0x3F, 0xB6,
        0x1F, 0x31, 0xEB, 0x57, 0xE7, 0x24, 0x61, 0x76)

/* or use an account on Ledger Nano S */
//account.use_ledger = true
//account.index = 0


print("Calling Smart Contract Function...");

var name = "Swift!"

var ret = aergo.call_async(
    account: account,
    contractAddress: "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7",
    function: "set_name",
    args: [name],
    callback:
{ context, receipt in

  if (receipt.status == "SUCCESS") {
    print("Smart Contract Call OK")
  } else {
    print("FAILED when calling the smart contract")
  }

  print("Response:", receipt.ret)
  print("BlockNo:", receipt.blockNo)
  print("TxIndex:", receipt.txIndex)
  print("GasUsed:", receipt.gasUsed)
  print("feeUsed:", receipt.feeUsed)

})

while( aergo.process_requests(timeout: 5000) > 0 ) { /* loop */ }

print("done")
