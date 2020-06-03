
var aergo = Aergo(host: "testnet-api.aergo.io", port: 7845)


var account = AergoAccount()

account.privkey = [UInt8](arrayLiteral:
        0xCD, 0xFA, 0x3B, 0xF9, 0x1A, 0x2C, 0xB6, 0x9B,
        0xEC, 0xB0, 0x16, 0x7E, 0x11, 0x00, 0xF6, 0xDE,
        0x77, 0xAF, 0x05, 0xD6, 0x4E, 0xE5, 0x0C, 0x2C,
        0xD4, 0xBA, 0xDD, 0x70, 0x01, 0xF0, 0xC5, 0x4B)


if (aergo.get_account_state(account: account) == true) {

  print("------------------------------------")
  print("Account address: ", account.address)
  print("Account balance: ", account.balance)
  print("Account nonce: ", account.nonce)

} else {

  print("Failed to get the account state")

}
