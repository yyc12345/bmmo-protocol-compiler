
public abstract class _BpMessage {
    virtual _OpCode GetOpCode() { throw new NotImplementedException(); }
    virtual bool IsReliable() { throw new NotImplementedException(); }
    virtual void Serialize(BinaryWriter _bw) { throw new NotImplementedException(); }
    virtual void Deserialize(BinaryReader _br) { throw new NotImplementedException(); }
    
    protected static _OpCode _BpPeekOpCode(this BinaryReader br) {
        _OpCode code = (_OpCode)br.ReadUInt32();
        br.BaseStream.Seek(sizeof(UInt32), SeekOrigin.Current);
        return code;
    }
    protected static string _BpReadString(this BinaryReader br) {
        UInt32 length = br.ReadUInt32();
        return Encoding.UTF8.GetString(br.ReadBytes((int)length));
    }
    protected static void _BpWriteString(this BinaryWriter bw, string strl) {
        var rawstr = Encoding.UTF8.GetBytes(strl);
        UInt32 length = (UInt32)rawstr.Length;
        bw.Write(length);
        bw.Write(rawstr);
    }
}
