﻿using System;
using System.Collections.Generic;

namespace contract_query_async
{
    class contract_query_async
    {

        public static void QueryCallback(AergoClient.CallbackState context, bool Success, string Result)
        {
            Console.WriteLine("-----------------");
            Console.WriteLine("Callback received");
            Console.WriteLine("Context= " + context.data);
            Console.WriteLine("Success= " + Success);
            Console.WriteLine("Result = " + Result + "\n");
        }

        static void Main(string[] args)
        {

            using (AergoClient client = new AergoClient())
            {
                client.Connect("testnet-api.aergo.io", 7845);

                Console.WriteLine("Sending request...");

                var context = new AergoClient.CallbackState() { data = "you can pass any object to the callback here" };

                var lResult = client.QuerySmartContractStateVariableAsync(QueryCallback, context, "AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe", "Name");

                if (lResult) {
                    Console.WriteLine("Done. Waiting for response...");
                    while (client.ProcessRequests(5000) > 0) { /* repeat */ }
                } else {
                    Console.WriteLine("Error on querying state variable.");
                }

                Console.WriteLine("Press any key to continue");
                Console.ReadKey();
            }
        }
        
    }
}
