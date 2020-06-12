require '../../wrappers/Ruby/aergo.rb'

aergo = Aergo.new("testnet-api.aergo.io", 7845)

ret = aergo.query_smart_contract(
    "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7",
    "hello")

if ret["success"]
  puts "Smart Contract Query OK"
  puts "Response: " + ret["result"]
else
  puts "FAILED when querying the smart contract"
  puts "Response: " + ret["result"]
end
