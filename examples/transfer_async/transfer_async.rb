require_relative '../../wrappers/Ruby/aergo.rb'

aergo = Aergo.new("testnet-api.aergo.io", 7845)

account = AergoAPI::Account.new
account[:privkey] = "\xDB\x85\xDD\x0C\xBA\x47\x32\xA1\x1A\xEB\x3C\x7C\x48\x91\xFB\xD2\xFE\xC4\x5F\xC7\x2D\xB3\x3F\xB6\x1F\x31\xEB\x57\xE7\x24\x61\x76"

TransferCallback = Proc.new do |context, receipt|
    puts "callback here!"
    puts "status : " + receipt[:status]
    puts "return : " + receipt[:ret]    
    puts "BlockNo: " + receipt[:blockNo].to_s
    puts "TxIndex: " + receipt[:txIndex].to_s
    puts "GasUsed: " + receipt[:gasUsed].to_s
    puts "feeUsed: " + receipt[:feeUsed].to_s
end

ret = aergo.transfer_async(
        TransferCallback,
        nil,
        account,
        "AmQFpC4idVstgqhnn7ihyueadTBVBq55LzDLbK8XbzHuhMxKAQ72",
        0.123)

while aergo.process_requests(5000) > 0
    # loop
end
