
/*
Message Read Example:
// Create buffer, this buffer can be recyclable use. 
// So you can define it in some global scopes, such as class member.
// If you are using MemoryStream in a small scope, you also can use `using` keyword 
// to ensure it can be released safely.
MemoryStream ms = new MemoryStream();
// Write binary array read from other sources, such as file or network stream
ms.Write(blabla);
// Then, we need reset its read pointer for following parsing
ms.Seek(0, SeekOrigin.Begin);
// Call uniformed deserializer to generate data struct
// If this function return null, it mean that parser can not understand this binary data
// This method also can throw exceptions, usually caused by broken binary data.
// We need construct a BinaryReader in there as its parameter, 
// and set leaveOpen = true to make sure that your MemoryStream will not be closed automatically.
_BpcMessage your_data = YourNameSpace._Helper.UniformDeserialize(new BinaryReader(ms, Encoding.Default, true));
if (your_data is null)
	throw new Exception("Invalid OpCode.");
// Clear buffer for future using. This operation is essential, 
// especially you are recyclable using this buffer.
ms.SetLength(0);

*/

/*
Message Write Example:
// Create buffer, this buffer can be recyclable use. 
// So you can define it in some global scopes, such as class member.
MemoryStream ms = new MemoryStream();
// Prepare your data struct, fill each fields, make sure all of theme are not None.
// The array and list located in your struct has been initialized. You do not need to init them again. Just fill the data.
var your_data = new ExampleMessage();
your_data.essential = 114514;
your_data.essential_list[0] = "test";
// Call your data's serialize() function to get binary data.
// This method also can throw some exceptions, usually caused by that you have not fill all essential fileds.
// We need construct a BinaryWriter in there as its parameter, 
// and set leaveOpen = true to make sure that your MemoryStream will not be closed automatically.
your_data.Serialize(new BinaryWriter(ms, Encoding.Default, true));
// Get bianry data from buffer and send it via some stream which you want.
// Or, you also can use MemoryStream directly to send data, to prevent unnecessary memory use caused by ToArray().
your_sender(ms.ToArray());
// Clear buffer for future using. This operation is essential, 
// especially you are recyclable using this buffer.
ms.SetLength(0);

*/

public interface _BpcMessage {
    _OpCode GetOpCode();
    bool IsReliable();
    void Serialize(BinaryWriter bw);
    void Deserialize(BinaryReader br);
}

public static partial class _Helper {
    public static _OpCode BpcPeekOpCode(this BinaryReader br) {
        _OpCode code = (_OpCode)br.ReadUInt32();
        br.BaseStream.Seek(sizeof(UInt32), SeekOrigin.Current);
        return code;
    }
    public static string BpcReadString(this BinaryReader br) {
        UInt32 length = br.ReadUInt32();
        return Encoding.Default.GetString(br.ReadBytes((int)length));
    }
    public static void BpcWriteString(this BinaryWriter bw, string strl) {
        var rawstr = Encoding.Default.GetBytes(strl);
        UInt32 length = (UInt32)rawstr.Length;
        bw.Write(length);
        bw.Write(rawstr);
    }
}

