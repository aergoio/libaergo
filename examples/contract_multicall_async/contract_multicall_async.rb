require_relative '../../wrappers/Ruby/aergo.rb'

aergo = Aergo.new("testnet-api.aergo.io", 7845)

account = AergoAPI::Account.new
# set the private key
account[:privkey] = "\xDB\x85\xDD\x0C\xBA\x47\x32\xA1\x1A\xEB\x3C\x7C\x48\x91\xFB\xD2\xFE\xC4\x5F\xC7\x2D\xB3\x3F\xB6\x1F\x31\xEB\x57\xE7\x24\x61\x76"
# or use an account on Ledger Nano S
#account[:use_ledger] = true
#account[:index] = 0

ContractCallback = Proc.new do |context, receipt|
    puts "status : " + receipt[:status]
    puts "return : " + receipt[:ret]    
    puts "BlockNo: " + receipt[:blockNo].to_s
    puts "TxIndex: " + receipt[:txIndex].to_s
    puts "GasUsed: " + receipt[:gasUsed].to_s
    puts "feeUsed: " + receipt[:feeUsed].to_s
end

puts "Calling Smart Contract Function Asynchronously..."

script = <<-JSON
[
  ["call","AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe","set_name","multicall async"],
  ["call","AmhQebt5N4pikoE4fZdEbaRAExoTwrLG3F5pKoGBVRbG5KeNKeoi","call_func"],
  ["call","AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf","set","1","first"],
  ["call","AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf","set","2","second"],
  ["call","AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf","get","1"],
  ["return", "%last result%"]
]
JSON

puts script

ret = aergo.multicall_async(
        ContractCallback,
        nil,
        account,
        script)

while aergo.process_requests(5000) > 0
    # loop
end
