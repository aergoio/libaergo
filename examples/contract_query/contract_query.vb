Module contract_query_example

    Sub Main()

        Using client As New AergoClient()

            client.Connect("testnet-api.aergo.io", 7845)

            Console.WriteLine("Querying Smart Contract...")

            Dim ret = client.QuerySmartContract("AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7", "hello", Nothing)

            If ret.success Then
                Console.WriteLine("Result: " + ret.result)
            Else
                Console.WriteLine("Error: " + ret.result)
            End If

        End Using

        Console.WriteLine("Press any key to continue")
        Console.ReadKey()

    End Sub

End Module
