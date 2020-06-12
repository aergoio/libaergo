require_relative '../../wrappers/Ruby/aergo.rb'

aergo = Aergo.new("testnet-api.aergo.io", 7845)

account = AergoAPI::Account.new
account[:privkey] = "\xCD\xFA\x3B\xF9\x1A\x2C\xB6\x9B\xEC\xB0\x16\x7E\x11\x00\xF6\xDE\x77\xAF\x05\xD6\x4E\xE5\x0C\x2C\xD4\xBA\xDD\x70\x01\xF0\xC5\x4B"

ret = aergo.get_account_state(account)

puts "address: " + account[:address]
puts "nonce  : " + account[:nonce].to_s
puts "balance: " + account[:balance].to_s
