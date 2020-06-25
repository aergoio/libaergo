require 'ffi'
require 'json'

module AergoAPI
  extend FFI::Library
  ffi_lib "libaergo." + FFI::Platform::LIBSUFFIX

  attach_function 'aergo_lib_version', [], :strptr

  attach_function 'aergo_connect', [:string, :int], :pointer
  attach_function 'aergo_free', [:pointer], :void

  attach_function 'aergo_process_requests', [:pointer, :int], :int

  # Accounts

  class Account < FFI::Struct
    layout :privkey, [:uint8, 32],
           :pubkey,  [:uint8, 33],
           :address, [:uint8, 64],
           :nonce,   :uint64,
           :balance, :double,
           :state_root, [:uint8, 32],
           :is_updated, :bool
  end

  attach_function 'aergo_check_privkey', [:pointer, Account.by_ref], :bool
  attach_function 'aergo_get_account_state', [:pointer, Account.by_ref], :bool

  # Transaction Receipt

  class Receipt < FFI::Struct
    layout :contractAddress, [:uint8, 56],
           :status, [:uint8, 16],
           :ret, [:uint8, 2048],
           :blockNo, :uint64,
           :blockHash, [:uint8, 32],
           :txIndex, :int32,
           :txHash, [:uint8, 32],
           :gasUsed, :uint64,
           :feeUsed, :double,
           :feeDelegation, :bool
  end

  callback :receipt_callback, [:pointer, Receipt.by_ref], :void

  # Smart Contract Query

  callback :query_smart_contract_cb, [:pointer, :bool, :string], :void

  attach_function 'aergo_query_smart_contract_json', [:pointer, :pointer, :int, :string, :string, :string], :bool

  attach_function 'aergo_query_smart_contract_json_async', [:pointer, :query_smart_contract_cb, :pointer, :string, :string, :string], :bool

  # Smart Contract Call

  attach_function 'aergo_call_smart_contract_json', [:pointer, Receipt.by_ref, Account.by_ref, :string, :string, :string], :bool

  attach_function 'aergo_call_smart_contract_json_async', [:pointer, :receipt_callback, :pointer, Account.by_ref, :string, :string, :string], :bool

  # Smart Contract Events

  class ContractEvent < FFI::Struct
    layout :contractAddress, [:uint8, 64],
           :eventName, [:uint8, 64],
           :jsonArgs, [:uint8, 2048],
           :eventIdx, :int32,
           :txHash, [:uint8, 32],
           :blockHash, [:uint8, 32],
           :blockNo, :uint64,
           :txIndex, :int32
  end

  callback :contract_event_callback, [:pointer, ContractEvent.by_ref], :void

  attach_function 'aergo_contract_events_subscribe', [:pointer, :string, :string, :contract_event_callback, :pointer], :bool

  # Transfer

  attach_function 'aergo_transfer', [:pointer, Receipt.by_ref, Account.by_ref, :string, :double], :bool

  attach_function 'aergo_transfer_str', [:pointer, Receipt.by_ref, Account.by_ref, :string, :string], :bool

  attach_function 'aergo_transfer_int', [:pointer, Receipt.by_ref, Account.by_ref, :string, :uint64, :uint64], :bool

  attach_function 'aergo_transfer_async', [:pointer, :receipt_callback, :pointer, Account.by_ref, :string, :double], :bool

  attach_function 'aergo_transfer_str_async', [:pointer, :receipt_callback, :pointer, Account.by_ref, :string, :string], :bool

  attach_function 'aergo_transfer_int_async', [:pointer, :receipt_callback, :pointer, Account.by_ref, :string, :uint64, :uint64], :bool

end



class Aergo
    @instance = 0

    def initialize(host, port)
        puts "initializing"
       @instance = AergoAPI.aergo_connect(host, port)
    end


    def version()
        version, version_ptr = AergoAPI.aergo_lib_version()
        version
    end


    def process_requests(timeout)
        AergoAPI.aergo_process_requests(@instance, timeout)
    end


    # Account

    def check_private_key(account)
        AergoAPI.aergo_check_privkey(@instance, account)
    end

    def get_account_state(account)
        error_msg = FFI::MemoryPointer.new(:int8, 256)

        ret = AergoAPI.aergo_get_account_state(@instance, account, error_msg)

        error_msg = error_msg.read_string().force_encoding('UTF-8')

        { "success" => ret, "error" => error_msg }
    end


    # Smart Contract Query

    def query_smart_contract(contract_address, function, *args)

        resultlen = 2048
        resultptr = FFI::MemoryPointer.new(:int8, resultlen)

        ret = AergoAPI.aergo_query_smart_contract_json(@instance,
            resultptr,
            resultlen,
            contract_address,
            function,
            args.to_json)

        result = resultptr.read_string().force_encoding('UTF-8')

        { "success" => ret, "result" => result }

    end

    def query_smart_contract_async(callback, context, contract_address, function, *args)

        AergoAPI.aergo_query_smart_contract_json_async(@instance,
            callback,
            context,
            contract_address,
            function,
            args.to_json)

    end


    # Smart Contract Call

    def call_smart_contract(account, contract_address, function, *args)

        receipt = AergoAPI::Receipt.new

        ret = AergoAPI.aergo_call_smart_contract_json(@instance,
            receipt,
            account,
            contract_address,
            function,
            args.to_json)

        if ret == false
            receipt[:status] = "FAILED"
        end

        receipt

    end

    def call_smart_contract_async(callback, context, account, contract_address, function, *args)

        AergoAPI.aergo_call_smart_contract_json_async(@instance,
            callback,
            context,
            account,
            contract_address,
            function,
            args.to_json)

    end


    # Smart Contract Events

    def contract_events_subscribe(contract_address, event_name, callback, context)

        AergoAPI.aergo_contract_events_subscribe(@instance,
            contract_address,
            event_name,
            callback,
            context)

    end


    # Transfer

    def transfer(from_account, to_account, *amount)

        receipt = AergoAPI::Receipt.new

        if amount.size == 1

            amount = amount[0]

            if amount.is_a?(String)

                ret = AergoAPI.aergo_transfer_str(@instance,
                    receipt,
                    from_account,
                    to_account,
                    amount)

            else

                ret = AergoAPI.aergo_transfer(@instance,
                    receipt,
                    from_account,
                    to_account,
                    amount)

            end

        elsif amount.size == 2

            ret = AergoAPI.aergo_transfer_int(@instance,
                receipt,
                from_account,
                to_account,
                amount[0], amount[1])

        else

            ret = false
            receipt[:ret] = "wrong number of arguments"

        end

        if ret == false
            receipt[:status] = "FAILED"
        end

        receipt

    end


    def transfer_async(callback, context, from_account, to_account, *amount)

        if amount.size == 1

            amount = amount[0]

            if amount.is_a?(String)

                AergoAPI.aergo_transfer_str_async(@instance,
                    callback,
                    context,
                    from_account,
                    to_account,
                    amount)

            else

                AergoAPI.aergo_transfer_async(@instance,
                    callback,
                    context,
                    from_account,
                    to_account,
                    amount)

            end

        elsif amount.size == 2

            AergoAPI.aergo_transfer_async(@instance,
                callback,
                context,
                from_account,
                to_account,
                amount[0], amount[1])

        end

    end

end
