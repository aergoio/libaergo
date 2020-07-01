Module account_info_example

    Sub Main()

        Using client As New AergoClient()

            client.Connect("testnet-api.aergo.io", 7845)

            Dim account As AergoClient.AergoAccount = New AergoClient.AergoAccount With
                {.privkey = New Byte() {&HCD, &HFA, &H3B, &HF9, &H1A, &H2C, &HB6, &H9B, &HEC, &HB0,
                                         &H16, &H7E, &H11, &H0, &HF6, &HDE, &H77, &HAF, &H5, &HD6,
                                         &H4E, &HE5, &HC, &H2C, &HD4, &HBA, &HDD, &H70, &H1, &HF0,
                                         &HC5, &H4B}}

            ' or use an account on Ledger Nano S
            'Dim account As AergoClient.AergoAccount = New AergoClient.AergoAccount With
            '    {.use_ledger = True}

            Dim ret = client.GetAccountInfo(account)

            If ret.success Then
                Console.WriteLine("Account state:")
                Console.WriteLine("Address = " + account.address)
                Console.WriteLine("Balance = " + account.balance.ToString())
                Console.WriteLine("Nonce   = " + account.nonce.ToString())
            Else
                Console.WriteLine("FAILED: " + ret.error)
            End If

        End Using

        Console.WriteLine("Press any key to continue")
        Console.ReadKey()

    End Sub

End Module
