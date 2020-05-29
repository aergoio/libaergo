Module transfer_example

    Sub Main()

        Using client As New AergoClient()

            client.Connect("testnet-api.aergo.io", 7845)

            Dim account As AergoClient.AergoAccount = New AergoClient.AergoAccount With
                {.privkey = New Byte() {&HDB, &H85, &HDD, &HC, &HBA, &H47, &H32, &HA1, &H1A, &HEB,
                                        &H3C, &H7C, &H48, &H91, &HFB, &HD2, &HFE, &HC4, &H5F, &HC7,
                                        &H2D, &HB3, &H3F, &HB6, &H1F, &H31, &HEB, &H57, &HE7, &H24,
                                        &H61, &H76}}

            Console.WriteLine("Sending the transfer...")


            ' Value as double
            Dim receipt = client.Transfer(account, "AmQFpC4idVstgqhnn7ihyueadTBVBq55LzDLbK8XbzHuhMxKAQ72", 0.15)

            If receipt.Sent Then
                Console.WriteLine("--- Transaction Receipt ---")
                Console.WriteLine("Status : " + receipt.status)
                Console.WriteLine("ret    : " + receipt.ret)
                Console.WriteLine("BlockNo: " + receipt.blockNo.ToString())
                Console.WriteLine("TxIndex: " + receipt.txIndex.ToString())
                Console.WriteLine("GasUsed: " + receipt.gasUsed.ToString())
            Else
                Console.WriteLine("Failed to send the transaction")
            End If


            ' Value as string
            receipt = client.Transfer(account, "AmQFpC4idVstgqhnn7ihyueadTBVBq55LzDLbK8XbzHuhMxKAQ72", "0.25")

            If receipt.Sent Then
                Console.WriteLine("--- Transaction Receipt ---")
                Console.WriteLine("Status : " + receipt.status)
                Console.WriteLine("ret    : " + receipt.ret)
                Console.WriteLine("BlockNo: " + receipt.blockNo.ToString())
                Console.WriteLine("TxIndex: " + receipt.txIndex.ToString())
                Console.WriteLine("GasUsed: " + receipt.gasUsed.ToString())
            Else
                Console.WriteLine("Failed to send the transaction")
            End If

            ' Value as integer and decimal parts
            receipt = client.Transfer(account, "AmQFpC4idVstgqhnn7ihyueadTBVBq55LzDLbK8XbzHuhMxKAQ72", 1, 350000000000000000)

            If receipt.Sent Then
                Console.WriteLine("--- Transaction Receipt ---")
                Console.WriteLine("Status : " + receipt.status)
                Console.WriteLine("ret    : " + receipt.ret)
                Console.WriteLine("BlockNo: " + receipt.blockNo.ToString())
                Console.WriteLine("TxIndex: " + receipt.txIndex.ToString())
                Console.WriteLine("GasUsed: " + receipt.gasUsed.ToString())
            Else
                Console.WriteLine("Failed to send the transaction")
            End If


        End Using

        Console.WriteLine("Press any key to continue")
        Console.ReadKey()
    End Sub

End Module
