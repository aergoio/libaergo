using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Numerics;

public class AergoClient : IDisposable
{
    public class CallbackState
    {
        public object data;
    }

    public class QueryResult
    {
        public Boolean success;
        public String  result;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct TransactionReceipt
    {
        [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 56)]
        public string contractAddress;

        [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 16)]
        public string status;

        [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 2048)]
        public string ret;

        [MarshalAsAttribute(UnmanagedType.U8)]
        public UInt64 blockNo;

        [MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst = 32)]
        public byte[] blockHash;

        [MarshalAsAttribute(UnmanagedType.I4)]
        public Int32 txIndex;

        [MarshalAsAttribute(UnmanagedType.U8)]
        public UInt64 gasUsed;

        [MarshalAsAttribute(UnmanagedType.R8)]
        public double feeUsed;

        [MarshalAsAttribute(UnmanagedType.Bool)]
        public bool feeDelegation;

        public bool Sent { get; set; }
    };


    [StructLayout(LayoutKind.Sequential)]
    public struct AergoAccount
    {
        [MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst = 32)]
        public byte[] privkey;

        [MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst = 33)]
        public byte[] pubkey;

        [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string address;

        [MarshalAsAttribute(UnmanagedType.U8)]
        public UInt64 nonce;

        [MarshalAsAttribute(UnmanagedType.R8)]
        public double balance;

        [MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst = 32)]
        public byte[] state_root;

        [MarshalAsAttribute(UnmanagedType.Bool)]
        public bool is_updated;
    };

    
    [StructLayout(LayoutKind.Sequential)]
    public struct ContractEvent
    {
        [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string contractAddress;

        [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string eventName;

        [MarshalAsAttribute(UnmanagedType.ByValTStr, SizeConst = 2048)]
        public string jsonArgs;

        [MarshalAsAttribute(UnmanagedType.I4)]
        public Int32 eventIdx;

        [MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst = 64)]
        public byte[] txHash;

        [MarshalAsAttribute(UnmanagedType.U8)]
        public UInt64 blockNo;

        [MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst = 32)]
        public byte[] blockHash;

        [MarshalAsAttribute(UnmanagedType.I4)]
        public Int32 txIndex;
    };

    public delegate void ContractEventCallback(CallbackState Context, ContractEvent Event);
    public delegate void QuerySmartContractCallback(CallbackState Context, bool Success, string Result);
    public delegate void TransactionReceiptCallback(CallbackState Context, TransactionReceipt Receipt);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_connect", CallingConvention = CallingConvention.Cdecl)]
    private static extern IntPtr aergo_connect(string Host, int Port);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_free", CallingConvention = CallingConvention.Cdecl)]
    private static extern void aergo_free(IntPtr _Instance);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_check_privkey", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_check_privkey(IntPtr _Instance, AergoAccount Account);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_get_account_state", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_get_account_state(IntPtr _Instance, ref AergoAccount Account);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_transfer", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_transfer(IntPtr _Instance, ref TransactionReceipt Receipt, ref AergoAccount Account, string ToAccount, double Value);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_transfer_int", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_transfer_int(IntPtr _Instance, ref TransactionReceipt Receipt, ref AergoAccount Account, string ToAccount, UInt64 IntegerPart, UInt64 DecimalPart);
    
    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_transfer_str", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_transfer_str(IntPtr _Instance, ref TransactionReceipt Receipt, ref AergoAccount Account, string ToAccount, string Value);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_transfer_bignum", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_transfer_bignum(IntPtr _Instance, ref TransactionReceipt Receipt, ref AergoAccount Account, string ToAccount, byte[] BigNumber, int Size);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_transfer_async", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_transfer_async(IntPtr _Instance,[MarshalAs(UnmanagedType.FunctionPtr)] TransactionReceiptCallback Callback, CallbackState Context, ref AergoAccount Account, string ToAccount, double Value);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_transfer_str_async", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_transfer_str_async(IntPtr _Instance, [MarshalAs(UnmanagedType.FunctionPtr)] TransactionReceiptCallback Callback, CallbackState Context, ref AergoAccount Account, string ToAccount, string Value);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_transfer_int_async", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_transfer_int_async(IntPtr _Instance, [MarshalAs(UnmanagedType.FunctionPtr)] TransactionReceiptCallback Callback, CallbackState Context, ref AergoAccount Account, string ToAccount, UInt64 IntegerPart, UInt64 DecimalPart);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_transfer_bignum_async", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_transfer_bignum_async(IntPtr _Instance, [MarshalAs(UnmanagedType.FunctionPtr)] TransactionReceiptCallback Callback, CallbackState Context, ref AergoAccount Account, string ToAccount, byte[] BigNumber, int Size);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_call_smart_contract_json", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_call_smart_contract_json(IntPtr _Instance, ref TransactionReceipt Receipt, ref AergoAccount Account, string ContractAddress, string Function, string JsonArgs);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_call_smart_contract_json_async", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_call_smart_contract_json_async(IntPtr _Instance, [MarshalAs(UnmanagedType.FunctionPtr)] TransactionReceiptCallback Callback, CallbackState Context, ref AergoAccount Account, string ContractAddress, string Function, string JsonArgs);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_query_smart_contract_json", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_query_smart_contract_json(IntPtr _Instance, [Out, MarshalAs(UnmanagedType.LPStr)] StringBuilder Result, int Size, string ContractAddress, string Function, string JsonArgs);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_query_smart_contract_json_async", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_query_smart_contract_json_async(IntPtr _Instance, [MarshalAs(UnmanagedType.FunctionPtr)] QuerySmartContractCallback Callback, CallbackState Context, string ContractAddress, string Function, string JsonArgs);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_contract_events_subscribe", CallingConvention = CallingConvention.Cdecl)]
    private static extern bool aergo_contract_events_subscribe(IntPtr _Instance, string ContractAddress, string EventName, ContractEventCallback Callback, CallbackState Context);

    [DllImport("aergo-0.1.dll", EntryPoint = "aergo_process_requests", CallingConvention = CallingConvention.Cdecl)]
    private static extern int  aergo_process_requests(IntPtr _Instance, int Timeout);


    private IntPtr _Instance = IntPtr.Zero;


    public void Connect(string Host, int Port)
    {
        _Instance = aergo_connect(Host, Port);
    }

    public void Dispose()
    {
        if (_Instance != IntPtr.Zero)
        {
            try
            {
                aergo_free(_Instance);
            }
            catch(Exception ex)
            {
                if (System.Diagnostics.Debugger.IsAttached)
                    Console.WriteLine("Error on disposal of instance handle: " + ex);
            }
            _Instance = IntPtr.Zero;
        }
    }

    ~AergoClient() {
        Dispose();
    }

    public int ProcessRequests(int Timeout) {
        return aergo_process_requests(_Instance, Timeout);
    }

    private static string ToJsonArray(object[] parameters)
    {
        if (parameters == null || parameters.Length == 0)
            return string.Empty;

        String JsonArray = string.Empty;
        foreach (object parameter in parameters)
        {
            switch (Type.GetTypeCode(parameter.GetType()))
            {
                case TypeCode.Boolean:
                    JsonArray += parameter.ToString().ToLower() + ",";
                    break;
                case TypeCode.Byte:
                case TypeCode.Decimal:
                case TypeCode.Double:
                case TypeCode.Int16:
                case TypeCode.Int32:
                case TypeCode.Int64:
                case TypeCode.UInt16:
                case TypeCode.UInt32:
                case TypeCode.UInt64:
                case TypeCode.Single:
                    JsonArray += parameter.ToString().Replace(',', '.') + ",";
                    break;
                default:
                case TypeCode.String:
                    JsonArray += "\"" + parameter + "\",";
                    break;
            }
        }

        return "[" + JsonArray.TrimEnd(',') + "]";
    }

    public bool CheckPrivateKey(AergoAccount account) {
        return aergo_check_privkey(_Instance, account);
    }

    public bool GetAccountInfo(ref AergoAccount account) {
        return aergo_get_account_state(_Instance, ref account);
    }

    // Contract Events

    public bool SmartContractEventsSubscribe(string ContractAddress, string EventName, ContractEventCallback Callback, CallbackState Context)
    {
        return aergo_contract_events_subscribe(_Instance, ContractAddress, EventName, Callback, Context);
    }

    // Query Smart Contract

    public QueryResult QuerySmartContract(string ContractAddress, string Function, params object[] parameters)
    {
        StringBuilder sb = new StringBuilder(4096);
        Boolean success = aergo_query_smart_contract_json(_Instance, sb, sb.Capacity, ContractAddress, Function, ToJsonArray(parameters));
        return new QueryResult { result = sb.ToString(), success = success };
    }

    public bool QuerySmartContractAsync(QuerySmartContractCallback Callback, CallbackState Context, string ContractAddress, string Function, params object[] parameters)
    {
        return aergo_query_smart_contract_json_async(_Instance,Callback, Context, ContractAddress, Function, ToJsonArray(parameters));
    }

    // Call Smart Contract

    public TransactionReceipt CallSmartContract(ref AergoAccount account, string pContractAddress, string pFunction, params object[] parameters)
    {
        TransactionReceipt receipt = new TransactionReceipt();
        receipt.Sent = aergo_call_smart_contract_json(_Instance, ref receipt, ref account, pContractAddress, pFunction, ToJsonArray(parameters));
        return receipt;
    }

    public bool CallSmartContractAsync(TransactionReceiptCallback Callback, CallbackState Context, ref AergoAccount account, string pContractAddress, string pFunction, params object[] parameters)
    {        
        return aergo_call_smart_contract_json_async(_Instance, Callback, Context, ref account, pContractAddress, pFunction, ToJsonArray(parameters));
    }

    // Synchronous Transfer

    public TransactionReceipt Transfer(ref AergoAccount account, string ToAccount, double Value)
    {
        TransactionReceipt receipt = new TransactionReceipt();
        receipt.Sent = aergo_transfer(_Instance, ref receipt, ref account, ToAccount, Value);
        return receipt;
    }

    public TransactionReceipt Transfer(ref AergoAccount account, string ToAccount, UInt64 IntegerPart, UInt64 DecimalPart)
    {
        TransactionReceipt receipt = new TransactionReceipt();
        receipt.Sent = aergo_transfer_int(_Instance, ref receipt, ref account, ToAccount, IntegerPart, DecimalPart);
        return receipt;
    }

    public TransactionReceipt Transfer(ref AergoAccount account, string ToAccount, string Value)
    {
        TransactionReceipt receipt = new TransactionReceipt();
        receipt.Sent = aergo_transfer_str(_Instance, ref receipt, ref account, ToAccount, Value);
        return receipt;
    }

    public TransactionReceipt Transfer(ref AergoAccount account, string ToAccount, BigInteger Value)
    {
        TransactionReceipt receipt = new TransactionReceipt();
        byte[] buf = Value.ToByteArray();
        // convert to big endian
        for (int i = 0, j = buf.Length - 1; i < j; i++, j--) {
            byte temp = buf[i];
            buf[i] = buf[j];
            buf[j] = temp;
        }
        receipt.Sent = aergo_transfer_bignum(_Instance, ref receipt, ref account, ToAccount, buf, buf.Length);
        return receipt;
    }

    // Asynchronous Transfer

    public bool TransferAsync(ref AergoAccount account, string ToAccount, double Value, TransactionReceiptCallback Callback, CallbackState Context)
    {        
        return aergo_transfer_async(_Instance, Callback, Context, ref account, ToAccount, Value);
    }

    public bool TransferAsync(ref AergoAccount account, string ToAccount, string Value, TransactionReceiptCallback Callback, CallbackState Context)
    {
        return aergo_transfer_str_async(_Instance, Callback, Context, ref account, ToAccount, Value);
    }

    public bool TransferAsync(ref AergoAccount account, string ToAccount, UInt64 IntegerPart, UInt64 DecimalPart, TransactionReceiptCallback Callback, CallbackState Context)
    {
        return aergo_transfer_int_async(_Instance, Callback, Context, ref account, ToAccount, IntegerPart, DecimalPart);
    }

    public bool TransferAsync(ref AergoAccount account, string ToAccount, BigInteger Value, TransactionReceiptCallback Callback, CallbackState Context)
    {
        byte[] buf = Value.ToByteArray();
        // convert to big endian
        for (int i = 0, j = buf.Length - 1; i < j; i++, j--) {
            byte temp = buf[i];
            buf[i] = buf[j];
            buf[j] = temp;
        }
        return aergo_transfer_bignum_async(_Instance, Callback, Context, ref account, ToAccount, buf, buf.Length);
    }

}
