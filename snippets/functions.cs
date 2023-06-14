
public abstract class BpStruct {
    public virtual void Serialize(BinaryWriter _bw) { throw new NotImplementedException(); }
    public virtual void Deserialize(BinaryReader _br) { throw new NotImplementedException(); }
}

public abstract class BpMessage : BpStruct {
    public virtual OpCode GetOpCode() { throw new NotImplementedException(); }
    public virtual bool IsReliable() { throw new NotImplementedException(); }
}

public static partial class BPHelper {
    public static void UniformSerialize(BpMessage instance, BinaryWriter bw) {
        bw.Write((UInt32)instance.GetOpCode());
        instance.Serialize(bw);
    }

    private static void _BpByteSwap(Span<byte> data, int unit_size) {
        switch (unit_size) {
            case 1:
                return;     // byte do not need any swap
            case 2: 
                {
                    var span16 = MemoryMarshal.Cast<byte, ushort>(data);
                    for (int i = 0; i < span16.Length; ++i) {
                        span16[i] = BinaryPrimitives.ReverseEndianness(span16[i]);
                    }
                    break;
                }
            case 4: {
                    var span32 = MemoryMarshal.Cast<byte, uint>(data);
                    for (int i = 0; i < span32.Length; ++i) {
                        span32[i] = BinaryPrimitives.ReverseEndianness(span32[i]);
                    }
                    break;
                }
            case 8: {
                    var span64 = MemoryMarshal.Cast<byte, ulong>(data);
                    for (int i = 0; i < span64.Length; ++i) {
                        span64[i] = BinaryPrimitives.ReverseEndianness(span64[i]);
                    }
                    break;
                }
            default:
                throw new ArgumentOutOfRangeException("Illegal unit size for byte swap!");
        }
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static byte _BpReadByte(this BinaryReader br) => br.ReadByte();
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static sbyte _BpReadSByte(this BinaryReader br) => br.ReadSByte();
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static UInt16 _BpReadUInt16(this BinaryReader br) => br.ReadUInt16();
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Int16 _BpReadInt16(this BinaryReader br) => br.ReadInt16();
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static UInt32 _BpReadUInt32(this BinaryReader br) => br.ReadUInt32();
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Int32 _BpReadInt32(this BinaryReader br) => br.ReadInt32();
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static UInt64 _BpReadUInt64(this BinaryReader br) => br.ReadUInt64();
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Int64 _BpReadInt64(this BinaryReader br) => br.ReadInt64();

    public static void _BpSkip(this BinaryReader br, long offset) {
        br.BaseStream.Seek(offset, SeekOrigin.Current);
    }

    public static string _BpReadString(this BinaryReader br) {
        UInt32 length = br._BpReadUInt32();
        return Encoding.UTF8.GetString(br.ReadBytes((int)length));
    }

    public static void _BpReadNumberTuple<TItem>(this BinaryReader br, ref TItem[] data, int unit_size) where TItem : struct {
        var buffer = MemoryMarshal.Cast<TItem, byte>(data.AsSpan());
        br.Read(buffer);
        if (!BitConverter.IsLittleEndian) _BpByteSwap(buffer, unit_size);
    }

    public static void _BpReadNumberList<TItem>(this BinaryReader br, ref List<TItem> data, int count, int unit_size) where TItem : struct {
        var raw = new TItem[count];

        var buffer = MemoryMarshal.Cast<TItem, byte>(raw);
        br.Read(buffer);
        if (!BitConverter.IsLittleEndian) _BpByteSwap(buffer, unit_size);

        data.AddRange(raw);
    }


    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteByte(this BinaryWriter bw, byte data) => bw.Write(data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteSByte(this BinaryWriter bw, sbyte data) => bw.Write(data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteUInt16(this BinaryWriter bw, UInt16 data) => bw.Write(data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteInt16(this BinaryWriter bw, Int16 data) => bw.Write(data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteUInt32(this BinaryWriter bw, UInt32 data) => bw.Write(data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteInt32(this BinaryWriter bw, Int32 data) => bw.Write(data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteUInt64(this BinaryWriter bw, UInt64 data) => bw.Write(data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteInt64(this BinaryWriter bw, Int64 data) => bw.Write(data);

    public static void _BpSkip(this BinaryWriter bw, long offset) {
        if (offset == 0) return;
        bw.BaseStream.Seek(offset - 1, SeekOrigin.Current);
        bw.Write((byte)0);
    }

    public static void _BpWriteString(this BinaryWriter bw, string strl) {
        var rawstr = Encoding.UTF8.GetBytes(strl);
        UInt32 length = (UInt32)rawstr.Length;
        bw._BpWriteUInt32(length);
        bw.Write(rawstr);
    }


    public static void _BpWriteNumberTuple<TItem>(this BinaryWriter bw, ref TItem[] data, int unit_size) where TItem : struct {
        Span<byte> buffer;
        if (BitConverter.IsLittleEndian) {
            buffer = MemoryMarshal.Cast<TItem, byte>(data.AsSpan());
        } else {
            var intermediary = (TItem[])data.Clone();
            buffer = MemoryMarshal.Cast<TItem, byte>(intermediary.AsSpan());
            _BpByteSwap(buffer, unit_size);
        }

        bw.Write(buffer);
    }

    public static void _BpWriteNumberList<TItem>(this BinaryWriter bw, ref List<TItem> data, int unit_size) where TItem : struct {
        var buffer = MemoryMarshal.Cast<TItem, byte>(data.ToArray().AsSpan());
        if (!BitConverter.IsLittleEndian) _BpByteSwap(buffer, unit_size);
        bw.Write(buffer);
    }
}
