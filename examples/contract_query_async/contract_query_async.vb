Module contract_query_async

    Sub Main()

        Using client As New AergoClient()

            client.Connect("testnet-api.aergo.io", 7845)

            Console.WriteLine("Querying Smart Contract...")

            Dim context = New AergoClient.CallbackState() With {.data = "you can pass any object to the callback here"}

            Dim result = client.QuerySmartContractAsync(AddressOf CallBack, context, "AmgLnRaGFLyvCPCEMHYJHooufT1c1pENTRGeV78WNPTxwQ2RYUW7", "hello", Nothing)

            If result Then
                While (client.ProcessRequests(5000) > 0)
                    ' loop
                End While
            Else
                Console.WriteLine("Failed")
            End If

        End Using

        Console.WriteLine("Press any key to continue")
        Console.ReadKey()

    End Sub

    Private Sub CallBack(context As AergoClient.CallbackState, Success As Boolean, Result As String)
        Console.WriteLine("--- Callback ---")
        Console.WriteLine("Context: " + context.data)
        If Success Then
            Console.WriteLine("Result: " + Result)
        Else
            Console.WriteLine("Error: " + Result)
        End If
    End Sub

End Module
