

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

