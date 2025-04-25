using System;
using System.Collections.Generic;
using System.Text;

namespace contract_query_cs
{
    class contract_query
    {
        static void Main(string[] args)
        {
            using (AergoClient client = new AergoClient())
            {
                client.Connect("testnet-api.aergo.io", 7845);

                Console.WriteLine("Querying Smart Contract State Variable...");

                var ret = client.QuerySmartContractStateVariable("AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe", "Name");

                if (!ret.success) {
                    Console.WriteLine("Error: " + ret.result);
                }  else {
                    Console.WriteLine("Result: " + ret.result);
                }

                Console.WriteLine("Press any key to continue");
                Console.ReadKey();
            }
        }
    }
}
