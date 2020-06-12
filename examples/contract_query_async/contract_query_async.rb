require '../../wrappers/Ruby/aergo.rb'

QueryCallback = Proc.new do |context, success, result|
    if success
        puts "Smart Contract Query OK"
        puts "Response: " + result
    else
        puts "FAILED when querying the smart contract"
        puts "Response: " + result
    end
end

aergo = Aergo.new("testnet-api.aergo.io", 7845)

ret = aergo.query_smart_contract_async(
    QueryCallback, nil,
    "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7",
    "hello")

while aergo.process_requests(5000) > 0
    # loop
end
