require_relative '../../wrappers/Ruby/aergo.rb'

aergo = Aergo.new("testnet-api.aergo.io", 7845)

ret = aergo.query_smart_contract_state_variable(
    "AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe",
    "Name")

if ret["success"]
  puts "Smart Contract State Variable Query OK"
  puts "Response: " + ret["result"]
else
  puts "FAILED when querying the smart contract state variable"
  puts "Response: " + ret["result"]
end
