using System;

namespace contract_events_subscribe_cs
{
    class contract_events_subscribe
    {

        static void Main(string[] args)
        {
            using (AergoClient client = new AergoClient())
            {
                client.Connect("testnet-api.aergo.io", 7845);

                Console.WriteLine("Waiting for events...");

                var ret = client.SmartContractEventsSubscribe("AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe", string.Empty, Callback, new AergoClient.CallbackState());

                while (client.ProcessRequests(5000) > 0) { /* repeat */ }
            }            
        }

        public static void Callback(AergoClient.CallbackState context, AergoClient.ContractEvent Event)
        {
            Console.WriteLine("--- New Event ---");
            Console.WriteLine("contractAddress: " + Event.contractAddress);
            Console.WriteLine("eventName: " + Event.eventName);
            Console.WriteLine("jsonArgs: " + Event.jsonArgs);
            Console.WriteLine("eventIdx: " + Event.eventIdx);
            Console.WriteLine("blockNo: " + Event.blockNo);
            Console.WriteLine("txIndex: " + Event.txIndex);
        }
    
    }
}
