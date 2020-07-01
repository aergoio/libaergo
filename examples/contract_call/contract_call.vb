Module contract_call

    Sub Main()

        Using client As New AergoClient()

            client.Connect("testnet-api.aergo.io", 7845)

            Dim account As AergoClient.AergoAccount = New AergoClient.AergoAccount With
                {.privkey = New Byte() {&HDB, &H85, &HDD, &HC, &HBA, &H47, &H32, &HA1, &H1A, &HEB,
                                        &H3C, &H7C, &H48, &H91, &HFB, &HD2, &HFE, &HC4, &H5F, &HC7,
                                        &H2D, &HB3, &H3F, &HB6, &H1F, &H31, &HEB, &H57, &HE7, &H24,
                                        &H61, &H76}}

            ' or use an account on Ledger Nano S
            'Dim account As AergoClient.AergoAccount = New AergoClient.AergoAccount With
            '    {.use_ledger = True}

            Console.WriteLine("Calling Smart Contract Function...")

            Dim receipt = client.CallSmartContract(account, "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7", "set_name", "VB.NET")

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
