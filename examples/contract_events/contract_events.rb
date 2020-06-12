require '../../wrappers/Ruby/aergo.rb'

aergo = Aergo.new("testnet-api.aergo.io", 7845)

EventCallback = Proc.new do |context, event|
    puts "--- new event ---"
    puts "address  : " + event[:contractAddress]
    puts "eventName: " + event[:eventName]
    puts "jsonArgs : " + event[:jsonArgs]    
    puts "BlockNo  : " + event[:blockNo].to_s
end

puts "Waiting for smart contract events..."

ret = aergo.contract_events_subscribe(
        "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7",
        "",
        EventCallback,
        nil)

while aergo.process_requests(5000) > 0
    # loop
end
