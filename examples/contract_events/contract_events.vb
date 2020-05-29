Module contract_events_subscribe

    Sub Main()

        Using client As New AergoClient()

            client.Connect("testnet-api.aergo.io", 7845)

            Console.WriteLine("Waiting for events...")

            Dim result = client.SmartContractEventsSubscribe("AmhcceopRiU7r3Gwy5tmtkk4Z3Px53SfsKBifGMvaSSNiyWrvKYe", String.Empty, AddressOf Callback, Nothing)

            If result Then
                While (True)
                    client.ProcessRequests(5000)
                End While
            Else
                Console.WriteLine("Error on executing query")
            End If

        End Using

        Console.WriteLine("Press any key to continue")
        Console.ReadKey()

    End Sub

    Private Sub Callback(ByVal Context As AergoClient.CallbackState, ByVal ContractEvent As AergoClient.ContractEvent)
        Console.WriteLine("--- New ContractEvent ---")
        Console.WriteLine("contractAddress: " + ContractEvent.contractAddress)
        Console.WriteLine("eventName: " + ContractEvent.eventName)
        Console.WriteLine("jsonArgs: " + ContractEvent.jsonArgs)
        Console.WriteLine("eventIdx: " + ContractEvent.eventIdx.ToString())
        Console.WriteLine("blockNo: " + ContractEvent.blockNo.ToString())
        Console.WriteLine("txIndex: " + ContractEvent.txIndex.ToString())
    End Sub

End Module
