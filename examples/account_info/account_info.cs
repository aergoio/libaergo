using System;

namespace account_info_cs
{
    class account_info
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

                Console.WriteLine("Sending request...");

                var ret = client.GetAccountInfo(ref account);

                if (ret.success) {
                    Console.WriteLine("Account state:");
                    Console.WriteLine("Address = " + account.address);
                    Console.WriteLine("Balance = " + account.balance);
                    Console.WriteLine("Nonce   = " + account.nonce);
                } else {
                    Console.WriteLine("Failed: " + ret.error);
                }

            }

            Console.WriteLine("Press any key to continue");
            Console.ReadKey();

        }        
    }
}
