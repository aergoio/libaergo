Module contract_call_async

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

            Console.WriteLine("Calling Smart Contract Function Asynchronously...")

            Dim script As String = "[" & _
                "[""call"",""AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe"",""set_name"",""multicall async""]," & _
                "[""call"",""AmhQebt5N4pikoE4fZdEbaRAExoTwrLG3F5pKoGBVRbG5KeNKeoi"",""call_func""]," & _
                "[""call"",""AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf"",""set"",""1"",""first""]," & _
                "[""call"",""AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf"",""set"",""2"",""second""]," & _
                "[""call"",""AmgqG45bTohBSQUgfDK4z31fq5jTy7CeRdwAf46hfaATG6tZtShf"",""get"",""1""]," & _
                "[""return"", ""%last result%""]" & _
            "]"

            Console.WriteLine(script)

            Dim context = New AergoClient.CallbackState() With {.data = "you can pass any object to the callback here"}

            Dim ret = client.MultiCallAsync(AddressOf CallBack, context, account, script)

            If ret Then
                Console.WriteLine("Waiting for response...")
                While (client.ProcessRequests(5000) > 0)
                    ' loop
                End While
            Else
                Console.WriteLine("Failed to send the transaction")
            End If

        End Using

        Console.WriteLine("Press any key to continue")
        Console.ReadKey()
    End Sub

    Private Sub CallBack(ByVal context As AergoClient.CallbackState, ByVal receipt As AergoClient.TransactionReceipt)

        Console.WriteLine("--- Transaction Receipt ---")
        Console.WriteLine("Status : " + receipt.status)
        Console.WriteLine("ret    : " + receipt.ret)
        Console.WriteLine("BlockNo: " + receipt.blockNo.ToString())
        Console.WriteLine("TxIndex: " + receipt.txIndex.ToString())
        Console.WriteLine("GasUsed: " + receipt.gasUsed.ToString())

    End Sub

End Module
