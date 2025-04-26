using System;

namespace contract_call_example
{
    class contract_call
    {
        static void Main(string[] args)
        {
            using (AergoClient client = new AergoClient())
            {
                client.Connect("testnet-api.aergo.io", 7845);

                AergoClient.AergoAccount account = new AergoClient.AergoAccount
                {
                    privkey = new byte[] {
                        0xDB, 0x85, 0xDD, 0x0C, 0xBA, 0x47, 0x32, 0xA1, 0x1A, 0xEB,
                        0x3C, 0x7C, 0x48, 0x91, 0xFB, 0xD2, 0xFE, 0xC4, 0x5F, 0xC7,
                        0x2D, 0xB3, 0x3F, 0xB6, 0x1F, 0x31, 0xEB, 0x57, 0xE7, 0x24,
                        0x61, 0x76
                    }
                };

                /* or use an account on Ledger Nano S */
                //AergoClient.AergoAccount account = new AergoClient.AergoAccount
                //{
                //    use_ledger = true
                //};

                Console.WriteLine("Sending Multicall transaction...");

                string script = @"[
                    [""call"",""AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe"",""set_name"",""multicall""],
                    [""call"",""AmhQebt5N4pikoE4fZdEbaRAExoTwrLG3F5pKoGBVRbG5KeNKeoi"",""call_func""],
                    [""call"",""AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf"",""set"",""1"",""first""],
                    [""call"",""AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf"",""set"",""2"",""second""],
                    [""call"",""AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf"",""get"",""1""],
                    [""return"", ""%last result%""]
                ]";

                Console.WriteLine(script);

                AergoClient.TransactionReceipt receipt = client.MultiCall(ref account, script);

                if (receipt.Sent) {
                    Console.WriteLine("--- Transaction Receipt ---");
                    Console.WriteLine("Status : " + receipt.status);
                    Console.WriteLine("ret    : " + receipt.ret);
                    Console.WriteLine("BlockNo: " + receipt.blockNo);
                    Console.WriteLine("TxIndex: " + receipt.txIndex);
                    Console.WriteLine("GasUsed: " + receipt.gasUsed);
                } else {
                    Console.WriteLine("Failed to send the transaction");
                }

                Console.WriteLine("Press any key to continue");
                Console.ReadKey();
            }
        }     
    }
}
