
public interface _BpcMessage {
    _OpCode GetOpCode();
    bool IsReliable();
    void Serialize(BinaryWriter bw);
    void Deserialize(BinaryReader br);
}

public static partial class _helper {
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
    public static void BpcClear(this MemoryStream ms) {
        ms.SetLength(0);
    }
    public static void BpcReset(this MemoryStream ms) {
        ms.Seek(0, SeekOrigin.Begin);
    }
    public static byte[] BpcGetData(this MemoryStream ms) {
        return ms.ToArray();
    }
    public static BinaryReader BpcCreateReader(this MemoryStream ms) {
        return new BinaryReader(ms, Encoding.Default, true);
    }
    public static BinaryWriter BpcCreateWriter(this MemoryStream ms) {
        return new BinaryWriter(ms, Encoding.Default, true);
    }
}
