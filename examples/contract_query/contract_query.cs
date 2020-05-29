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

                Console.WriteLine("Querying Smart Contract...");

                var ret = client.QuerySmartContract("AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7", "hello");

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
