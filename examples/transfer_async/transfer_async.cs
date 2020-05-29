using System;
using System.Numerics;

namespace transfer_async_cs
{
    class transfer_async
    {

        public static void ReceiptReceived(AergoClient.CallbackState context, AergoClient.TransactionReceipt receipt)
        {
            Console.WriteLine("--- Transaction Receipt - " + context.data.ToString() + " ---");
            Console.WriteLine("Status : " + receipt.status);
            Console.WriteLine("ret    : " + receipt.ret);
            Console.WriteLine("BlockNo: " + receipt.blockNo);
            Console.WriteLine("TxIndex: " + receipt.txIndex);
            Console.WriteLine("GasUsed: " + receipt.gasUsed);
        }

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


                AergoClient.CallbackState context;
                bool ret;


                Console.WriteLine("Sending transfers...");


                context = new AergoClient.CallbackState() { data = "double" };
                ret = client.TransferAsync(ref account, "AmQFpC4idVstgqhnn7ihyueadTBVBq55LzDLbK8XbzHuhMxKAQ72", 0.15, ReceiptReceived, context);
                if (ret == false) {
                    Console.WriteLine("Failed with " + context.data);
                }


                context = new AergoClient.CallbackState() { data = "integer and decimal" };
                ret = client.TransferAsync(ref account, "AmQFpC4idVstgqhnn7ihyueadTBVBq55LzDLbK8XbzHuhMxKAQ72", 1, 250000000000000000, ReceiptReceived, context);
                if (ret == false) {
                    Console.WriteLine("Failed with " + context.data);
                }


                context = new AergoClient.CallbackState() { data = "string" };
                ret = client.TransferAsync(ref account, "AmQFpC4idVstgqhnn7ihyueadTBVBq55LzDLbK8XbzHuhMxKAQ72", "0.35", ReceiptReceived, context);
                if (ret == false) {
                    Console.WriteLine("Failed with " + context.data);
                }


                context = new AergoClient.CallbackState() { data = "BigInteger" };
                ret = client.TransferAsync(ref account, "AmQFpC4idVstgqhnn7ihyueadTBVBq55LzDLbK8XbzHuhMxKAQ72", new BigInteger(1450000000000000000), ReceiptReceived, context);
                if (ret == false) {
                    Console.WriteLine("Failed with " + context.data);
                }


                Console.WriteLine("Waiting for the receipts...");

                while (client.ProcessRequests(5000) > 0) { /* repeat */ }

                Console.WriteLine("Press any key to continue");
                Console.ReadKey();
            }
        }
    }
}
