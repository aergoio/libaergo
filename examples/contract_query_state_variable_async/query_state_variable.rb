require_relative '../../wrappers/Ruby/aergo.rb'

QueryCallback = Proc.new do |context, success, result|
    if success
        puts "Smart Contract State Variable Query OK"
        puts "Response: " + result
    else
        puts "FAILED when querying the smart contract state variable"
        puts "Response: " + result
    end
end

aergo = Aergo.new("testnet-api.aergo.io", 7845)

ret = aergo.query_smart_contract_state_variable_async(
    QueryCallback, nil,
    "AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe",
    "Name")

while aergo.process_requests(5000) > 0
    # loop
end
