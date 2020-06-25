import Foundation


// Public objects

class AergoAccount {
  var use_ledger: Bool
  var index: Int32
  var privkey: [UInt8]
  var pubkey: [UInt8]
  var address: String
  var nonce: UInt64
  var balance: Double
  var state_root: [UInt8]
  var is_updated: Bool

  init() {
    use_ledger = false
    index = 0
    privkey = [UInt8](repeating: 0, count: 32)
    pubkey = [UInt8](repeating: 0, count: 33)
    address = ""
    nonce = 0
    balance = 0.0
    state_root = [UInt8](repeating: 0, count: 32)
    is_updated = false
  }
}


class TransactionReceipt {
  var contractAddress: String
  var status: String
  var ret: String
  var blockNo: UInt64
  var blockHash: [UInt8]
  var txIndex: Int32
  var gasUsed: UInt64
  var feeUsed: Double
  var feeDelegation: Bool

  init() {
    contractAddress = ""
    status = ""
    ret = ""
    blockNo = 0
    blockHash = [UInt8](repeating: 0, count: 32)
    txIndex = 0
    gasUsed = 0
    feeUsed = 0.0
    feeDelegation = false
  }
}


class ContractEvent {
  var contractAddress: String
  var eventName: String
  var jsonArgs: String
  var eventIdx: Int32
  var blockNo: UInt64
  var blockHash: [UInt8]
  var txIndex: Int32
  var txHash: [UInt8]

  init() {
    contractAddress = ""
    eventName = ""
    jsonArgs = ""
    eventIdx = 0
    blockNo = 0
    blockHash = [UInt8](repeating: 0, count: 32)
    txIndex = 0
    txHash = [UInt8](repeating: 0, count: 32)
  }
}



// Private classes

private class QueryCallbackInfo {
  let callback: (Any?,Bool,String)->()
  var context: Any?

  init(cb: @escaping (Any?,Bool,String)->(), ctx: Any?) {
    callback = cb
    context = ctx
  }
}


private class TransactionCallbackInfo {
  let callback: (Any?,TransactionReceipt)->()
  var context: Any?

  init(cb: @escaping (Any?,TransactionReceipt)->(), ctx: Any?) {
    callback = cb
    context = ctx
  }
}


private class ContractEventCallbackInfo {
  let callback: (Any?,ContractEvent)->()
  var context: Any?

  init(cb: @escaping (Any?,ContractEvent)->(), ctx: Any?) {
    callback = cb
    context = ctx
  }
}


// Private functions

private func on_query_internal_callback(ctx: UnsafeMutableRawPointer?,
                                        success: CBool,
                                        buffer: UnsafeMutablePointer<Int8>?){

  let callback_info = Unmanaged<QueryCallbackInfo>.fromOpaque(ctx!).takeRetainedValue()

  let result = String(cString: UnsafeRawPointer(buffer!).assumingMemoryBound(to: CChar.self))

  callback_info.callback(callback_info.context, success, result)

}


private func on_receipt_internal_callback(ctx: UnsafeMutableRawPointer?,
                                          receipt_ptr: UnsafeMutablePointer<transaction_receipt>?){

  let callback_info = Unmanaged<TransactionCallbackInfo>.fromOpaque(ctx!).takeRetainedValue()

  var c_receipt = UnsafeMutablePointer<transaction_receipt>(receipt_ptr!).pointee

  let receipt = TransactionReceipt()
  Aergo.update_receipt(receipt: receipt, c_receipt: &c_receipt)

  callback_info.callback(callback_info.context, receipt)

}


private func on_contract_event_internal_callback(ctx: UnsafeMutableRawPointer?,
                                                 event_ptr: UnsafeMutablePointer<contract_event>?){

  let callback_info = Unmanaged<ContractEventCallbackInfo>.fromOpaque(ctx!).takeRetainedValue()

  var c_event = UnsafeMutablePointer<contract_event>(event_ptr!).pointee

  let event = ContractEvent()

  event.contractAddress = String(cString: UnsafeRawPointer([c_event.contractAddress]).assumingMemoryBound(to: CChar.self))

  event.eventName = String(cString: UnsafeRawPointer([c_event.eventName]).assumingMemoryBound(to: CChar.self))

  event.jsonArgs = String(cString: UnsafeRawPointer([c_event.jsonArgs]).assumingMemoryBound(to: CChar.self))

  memcpy(&event.blockHash, &c_event.blockHash, 32)
  memcpy(&event.txHash, &c_event.txHash, 32)

  event.eventIdx = c_event.eventIdx
  event.blockNo = c_event.blockNo
  event.txIndex = c_event.txIndex

  callback_info.callback(callback_info.context, event)

}



// Public Aergo class


class Aergo {
  private var instance: OpaquePointer
  private var contract_events_cb: ContractEventCallbackInfo?

  init(host: String, port: Int) {
    instance = aergo_connect(host, Int32(port))
    contract_events_cb = nil
  }
  deinit {
    aergo_free(instance)
  }


  func process_requests(timeout: Int32? = 0) -> Int32 {
    return aergo_process_requests(instance, timeout ?? 0)
  }


  func query(contractAddress: String, function: String, args: [Any])
    -> (success: Bool, result: String) {

      var ret: Bool
      var json_args: String
      var buffer = [Int8](repeating: 0, count: 65536)

      if (args.count > 0) {
        do {
          let encoded = try JSONSerialization.data(withJSONObject: args)
          json_args = NSString(data: encoded, encoding: String.Encoding.utf8.rawValue)! as String
        } catch {
          return (false, error.localizedDescription)
        }
      } else {
        json_args = ""
      }

      ret = aergo_query_smart_contract_json(
        instance,
        &buffer, 65536,
        contractAddress,
        function,
        json_args)

      if (ret == false) { return (false, "the call failed") }

      return (true, String(cString: buffer))
  }


  func query_async(contractAddress: String, function: String, args: [Any],
                   callback: @escaping (Any?,Bool,String)->(),
                   context: Any? = nil) -> Bool {

    var json_args: String

    if (args.count > 0) {
      do {
        let encoded = try JSONSerialization.data(withJSONObject: args)
        json_args = NSString(data: encoded, encoding: String.Encoding.utf8.rawValue)! as String
      } catch {
        return false
      }
    } else {
      json_args = ""
    }

    let callback_info = QueryCallbackInfo(cb: callback, ctx: context)
    let ctx = UnsafeMutableRawPointer(Unmanaged.passRetained(callback_info).toOpaque())

    let ret = aergo_query_smart_contract_json_async(
      instance,
      on_query_internal_callback,
      ctx,
      contractAddress,
      function,
      json_args
    )

    if (ret == false) {
      // unretain the object
      _ = Unmanaged<QueryCallbackInfo>.fromOpaque(ctx).takeRetainedValue()
    }

    return ret
  }



  class func update_c_account(account: AergoAccount, c_account: inout aergo_account) {

    withUnsafeMutableBytes(of: &c_account.address) { ptr in
      ptr.copyBytes(from: account.address.utf8.prefix(ptr.count))
    }

    memcpy(&c_account.privkey, &account.privkey, 32)
    memcpy(&c_account.pubkey, &account.pubkey, 33)
    memcpy(&c_account.state_root, &account.state_root, 32)

    c_account.use_ledger = account.use_ledger
    c_account.index = account.index
    c_account.balance = account.balance
    c_account.nonce = account.nonce
    c_account.is_updated = account.is_updated

  }


  class func update_account(account: AergoAccount, c_account: inout aergo_account) {

    account.address = String(cString: UnsafeRawPointer([c_account.address]).assumingMemoryBound(to: CChar.self))

    memcpy(&account.pubkey, &c_account.pubkey, 33)
    memcpy(&account.state_root, &c_account.state_root, 32)

    account.use_ledger = c_account.use_ledger
    account.index = c_account.index
    account.balance = c_account.balance
    account.nonce = c_account.nonce
    account.is_updated = c_account.is_updated

  }

  class func update_receipt(receipt: TransactionReceipt, c_receipt: inout transaction_receipt) {

    receipt.contractAddress = String(cString: UnsafeRawPointer([c_receipt.contractAddress]).assumingMemoryBound(to: CChar.self))
    receipt.status = String(cString: UnsafeRawPointer([c_receipt.status]).assumingMemoryBound(to: CChar.self))
    receipt.ret = String(cString: UnsafeRawPointer([c_receipt.ret]).assumingMemoryBound(to: CChar.self))

    memcpy(&receipt.blockHash, &c_receipt.blockHash, 32)

    receipt.blockNo = c_receipt.blockNo
    receipt.txIndex = c_receipt.txIndex
    receipt.gasUsed = c_receipt.gasUsed
    receipt.feeUsed = c_receipt.feeUsed
    receipt.feeDelegation = c_receipt.feeDelegation

  }




  func check_private_key(account: AergoAccount) -> Bool {

    var c_account = aergo_account()

    Aergo.update_c_account(account: account, c_account: &c_account)

    return aergo_check_privkey(instance, &c_account)

  }


  func get_account_state(account: AergoAccount) -> (success: Bool, error: String) {

    var c_account = aergo_account()
    var error_msg = [Int8](repeating: 0, count: 256)

    Aergo.update_c_account(account: account, c_account: &c_account)

    if (aergo_get_account_state(instance, &c_account, &error_msg) == true) {

      Aergo.update_account(account: account, c_account: &c_account)

      return (true, "")
    }

    return (false, String(cString: error_msg))
  }



  func call(account: AergoAccount, contractAddress: String, function: String, args: [Any]) -> TransactionReceipt {

    var ret: Bool
    let receipt = TransactionReceipt()
    var json_args: String

    if (args.count > 0) {
      do {
        let encoded = try JSONSerialization.data(withJSONObject: args)
        json_args = NSString(data: encoded, encoding: String.Encoding.utf8.rawValue)! as String
      } catch {
        receipt.ret = error.localizedDescription
        return receipt
      }
    } else {
      json_args = ""
    }

    var c_receipt = transaction_receipt()
    var c_account = aergo_account()

    Aergo.update_c_account(account: account, c_account: &c_account)

    ret = aergo_call_smart_contract_json(
      instance,
      &c_receipt,
      &c_account,
      contractAddress,
      function,
      json_args
    )

    Aergo.update_account(account: account, c_account: &c_account)

    if (ret == true) {
      Aergo.update_receipt(receipt: receipt, c_receipt: &c_receipt)
    }

    return receipt
  }


  func call_async(account: AergoAccount, contractAddress: String, function: String, args: [Any],
                  callback: @escaping (Any?,TransactionReceipt)->(),
                  context: Any? = nil) -> Bool {

    var ret: Bool
    var json_args: String

    if (args.count > 0) {
      do {
        let encoded = try JSONSerialization.data(withJSONObject: args)
        json_args = NSString(data: encoded, encoding: String.Encoding.utf8.rawValue)! as String
      } catch {
        return false
      }
    } else {
      json_args = ""
    }

    var c_account = aergo_account()

    Aergo.update_c_account(account: account, c_account: &c_account)

    let callback_info = TransactionCallbackInfo(cb: callback, ctx: context)
    let ctx = UnsafeMutableRawPointer(Unmanaged.passRetained(callback_info).toOpaque())

    ret = aergo_call_smart_contract_json_async(
      instance,
      on_receipt_internal_callback,
      ctx,
      &c_account,
      contractAddress,
      function,
      json_args
    )

    Aergo.update_account(account: account, c_account: &c_account)

    if (ret == false) {
      // unretain the object
      _ = Unmanaged<TransactionCallbackInfo>.fromOpaque(ctx).takeRetainedValue()
    }

    return ret
  }



  // Synchronous Transfer - amount as Double

  func transfer(from_account: AergoAccount, to_account: String, amount: Double) -> TransactionReceipt {

    var ret: Bool
    let receipt = TransactionReceipt()
    var c_receipt = transaction_receipt()
    var c_account = aergo_account()

    Aergo.update_c_account(account: from_account, c_account: &c_account)

    ret = aergo_transfer(
      instance,
      &c_receipt,
      &c_account,
      to_account,
      amount
    )

    Aergo.update_account(account: from_account, c_account: &c_account)

    if (ret == true) {
      Aergo.update_receipt(receipt: receipt, c_receipt: &c_receipt)
    }

    return receipt
  }


  // Synchronous Transfer - amount as String

  func transfer(from_account: AergoAccount, to_account: String, amount: String) -> TransactionReceipt {

    var ret: Bool
    let receipt = TransactionReceipt()
    var c_receipt = transaction_receipt()
    var c_account = aergo_account()

    Aergo.update_c_account(account: from_account, c_account: &c_account)

    ret = aergo_transfer_str(
      instance,
      &c_receipt,
      &c_account,
      to_account,
      amount
    )

    Aergo.update_account(account: from_account, c_account: &c_account)

    if (ret == true) {
      Aergo.update_receipt(receipt: receipt, c_receipt: &c_receipt)
    }

    return receipt
  }


  // Synchronous Transfer - amount as integer and decimal parts

  func transfer(from_account: AergoAccount, to_account: String,
                integer_part: UInt64, decimal_part: UInt64) -> TransactionReceipt {

    var ret: Bool
    let receipt = TransactionReceipt()
    var c_receipt = transaction_receipt()
    var c_account = aergo_account()

    Aergo.update_c_account(account: from_account, c_account: &c_account)

    ret = aergo_transfer_int(
      instance,
      &c_receipt,
      &c_account,
      to_account,
      integer_part, decimal_part
    )

    Aergo.update_account(account: from_account, c_account: &c_account)

    if (ret == true) {
      Aergo.update_receipt(receipt: receipt, c_receipt: &c_receipt)
    }

    return receipt
  }


  // Asynchronous Transfer - amount as Double

  func transfer_async(from_account: AergoAccount, to_account: String, amount: Double,
                      callback: @escaping (Any?,TransactionReceipt)->(),
                      context: Any? = nil) -> Bool {

    var ret: Bool
    var c_account = aergo_account()

    Aergo.update_c_account(account: from_account, c_account: &c_account)

    let callback_info = TransactionCallbackInfo(cb: callback, ctx: context)
    let ctx = UnsafeMutableRawPointer(Unmanaged.passRetained(callback_info).toOpaque())

    ret = aergo_transfer_async(
      instance,
      on_receipt_internal_callback,
      ctx,
      &c_account,
      to_account,
      amount
    )

    Aergo.update_account(account: from_account, c_account: &c_account)

    if (ret == false) {
      // unretain the object
      _ = Unmanaged<TransactionCallbackInfo>.fromOpaque(ctx).takeRetainedValue()
    }

    return ret
  }


  // Asynchronous Transfer - amount as String

  func transfer_async(from_account: AergoAccount, to_account: String, amount: String,
                      callback: @escaping (Any?,TransactionReceipt)->(),
                      context: Any? = nil) -> Bool {

    var ret: Bool
    var c_account = aergo_account()

    Aergo.update_c_account(account: from_account, c_account: &c_account)

    let callback_info = TransactionCallbackInfo(cb: callback, ctx: context)
    let ctx = UnsafeMutableRawPointer(Unmanaged.passRetained(callback_info).toOpaque())

    ret = aergo_transfer_str_async(
      instance,
      on_receipt_internal_callback,
      ctx,
      &c_account,
      to_account,
      amount
    )

    Aergo.update_account(account: from_account, c_account: &c_account)

    if (ret == false) {
      // unretain the object
      _ = Unmanaged<TransactionCallbackInfo>.fromOpaque(ctx).takeRetainedValue()
    }

    return ret
  }


  // Asynchronous Transfer - amount as integer and decimal parts

  func transfer_async(from_account: AergoAccount, to_account: String,
                      integer_part: UInt64, decimal_part: UInt64,
                      callback: @escaping (Any?,TransactionReceipt)->(),
                      context: Any? = nil) -> Bool {

    var ret: Bool
    var c_account = aergo_account()

    Aergo.update_c_account(account: from_account, c_account: &c_account)

    let callback_info = TransactionCallbackInfo(cb: callback, ctx: context)
    let ctx = UnsafeMutableRawPointer(Unmanaged.passRetained(callback_info).toOpaque())

    ret = aergo_transfer_int_async(
      instance,
      on_receipt_internal_callback,
      ctx,
      &c_account,
      to_account,
      integer_part, decimal_part
    )

    Aergo.update_account(account: from_account, c_account: &c_account)

    if (ret == false) {
      // unretain the object
      _ = Unmanaged<TransactionCallbackInfo>.fromOpaque(ctx).takeRetainedValue()
    }

    return ret
  }



  func contract_events_subscribe(contractAddress: String, eventName: String,
                                 callback: @escaping (Any?,ContractEvent)->(),
                                 context: Any? = nil) -> Bool {

    contract_events_cb = ContractEventCallbackInfo(cb: callback, ctx: context)
    let ctx = UnsafeMutableRawPointer(Unmanaged.passUnretained(contract_events_cb!).toOpaque())

    let ret = aergo_contract_events_subscribe(
      instance,
      contractAddress,
      eventName,
      on_contract_event_internal_callback,
      ctx
    )

    if (ret == false) {
      // release the object
      contract_events_cb = nil
    }

    return ret
  }


}
