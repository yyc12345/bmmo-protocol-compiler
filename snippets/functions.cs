
public abstract class BpStruct {
    public virtual void Serialize(BinaryWriter _bw) { throw new NotImplementedException(); }
    public virtual void Deserialize(BinaryReader _br) { throw new NotImplementedException(); }
}

public abstract class BpMessage : BpStruct {
    public virtual OpCode GetOpCode() { throw new NotImplementedException(); }
    public virtual bool IsReliable() { throw new NotImplementedException(); }
}

public static class BPEndianHelper {

#if _ENABLE_BP_TESTBENCH
    public static bool g_ForceBigEndian = false;
#endif
    public static bool _IsLittleEndian() {
#if _ENABLE_BP_TESTBENCH
        // testbench used LE reverser
        if (g_ForceBigEndian) return !BitConverter.IsLittleEndian;
        else return BitConverter.IsLittleEndian;
#else
        return BitConverter.IsLittleEndian;
#endif
    }

#if _BP_MODERN_CSHARP
    // modern byte swap. redirect to BinaryPrimitives
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static byte _BpByteSwap(byte v) => BinaryPrimitives.ReverseEndianness(v);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static sbyte _BpByteSwap(sbyte v) => BinaryPrimitives.ReverseEndianness(v);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static ushort _BpByteSwap(ushort v) => BinaryPrimitives.ReverseEndianness(v);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static short _BpByteSwap(short v) => BinaryPrimitives.ReverseEndianness(v);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static uint _BpByteSwap(uint v) => BinaryPrimitives.ReverseEndianness(v);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static int _BpByteSwap(int v) => BinaryPrimitives.ReverseEndianness(v);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static ulong _BpByteSwap(ulong v) => BinaryPrimitives.ReverseEndianness(v);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static long _BpByteSwap(long v) => BinaryPrimitives.ReverseEndianness(v);
#else
    // legacy byte swap. polyfill according to the references of microsoft corefx code.
    // https://github.com/dotnet/runtime/blob/e05b3e752e89259454fbd919e4be34cc7c3ed937/src/libraries/System.Private.CoreLib/src/System/Buffers/Binary/BinaryPrimitives.ReverseEndianness.cs#L45
    // https://github.com/dotnet/runtime/blob/e05b3e752e89259454fbd919e4be34cc7c3ed937/src/libraries/System.Private.CoreLib/src/System/Numerics/BitOperations.cs#L752
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static byte _BpByteSwap(byte v) {
        return v;
    }
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static sbyte _BpByteSwap(sbyte v) => (sbyte)_BpByteSwap((byte)v);

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static ushort _BpByteSwap(ushort v) {
        return (ushort)((v >> 8) + (v << 8));
    }
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static short _BpByteSwap(short v) => (short)_BpByteSwap((ushort)v);

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static uint _BpByteSwap(uint v) {
        // original statement
        // return BitOperations.RotateRight(v & 0x00FF00FFu, 8)
        //     + BitOperations.RotateLeft(v & 0xFF00FF00u, 8);
        // manually expand function calling
        return (((v & 0x00FF00FFu) >> 8) | ((v & 0x00FF00FFu) << (32 - 8)))
            + (((v & 0xFF00FF00u) << 8) | ((v & 0xFF00FF00u) >> (32 - 8)));
    }
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static int _BpByteSwap(int v) => (int)_BpByteSwap((uint)v);

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static ulong _BpByteSwap(ulong v) {
        return ((ulong)_BpByteSwap((uint)v) << 32)
            + _BpByteSwap((uint)(v >> 32));
    }
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static long _BpByteSwap(long v) => (long)_BpByteSwap((ulong)v);
#endif

#if _BP_MODERN_CSHARP
    // modern byte swap
    public static void _BpByteSwapArray(Span<byte> data, int unit_size) {
        switch (unit_size) {
            case 1:
                return;     // byte do not need any swap
            case 2: {
                    var span16 = MemoryMarshal.Cast<byte, ushort>(data);
                    for (int i = 0; i < span16.Length; ++i) {
                        span16[i] = _BpByteSwap(span16[i]);
                    }
                    break;
                }
            case 4: {
                    var span32 = MemoryMarshal.Cast<byte, uint>(data);
                    for (int i = 0; i < span32.Length; ++i) {
                        span32[i] = _BpByteSwap(span32[i]);
                    }
                    break;
                }
            case 8: {
                    var span64 = MemoryMarshal.Cast<byte, ulong>(data);
                    for (int i = 0; i < span64.Length; ++i) {
                        span64[i] = _BpByteSwap(span64[i]);
                    }
                    break;
                }
            default:
                throw new ArgumentOutOfRangeException("Illegal unit size for byte swap!");
        }
    }
#else
    // legacy byte swap
    public static void _BpByteSwapArray(ref byte[] data, int unit_size) {
        switch (unit_size) {
            case 1:
                return;     // byte do not need any swap
            case 2: {
                    var span16 = new ushort[data.Length / unit_size];
                    Buffer.BlockCopy(data, 0, span16, 0, data.Length);
                    for (int i = 0; i < span16.Length; i += unit_size) {
                        span16[i] = _BpByteSwap(span16[i]);
                    }
                    Buffer.BlockCopy(span16, 0, data, 0, data.Length);
                    break;
                }
            case 4: {
                    var span32 = new uint[data.Length / unit_size];
                    Buffer.BlockCopy(data, 0, span32, 0, data.Length);
                    for (int i = 0; i < span32.Length; ++i) {
                        span32[i] = _BpByteSwap(span32[i]);
                    }
                    Buffer.BlockCopy(span32, 0, data, 0, data.Length);
                    break;
                }
            case 8: {
                    var span64 = new ulong[data.Length / unit_size];
                    Buffer.BlockCopy(data, 0, span64, 0, data.Length);
                    for (int i = 0; i < span64.Length; ++i) {
                        span64[i] = _BpByteSwap(span64[i]);
                    }
                    Buffer.BlockCopy(span64, 0, data, 0, data.Length);
                    break;
                }
            default:
                throw new ArgumentOutOfRangeException("Illegal unit size for byte swap!");
        }
    }
#endif

}

public static partial class BPHelper {

#if _ENABLE_BP_TESTBENCH
    // debug environment
    // conditional reading
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static byte _BpReadByte(this BinaryReader br) => (BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(br.ReadByte()) : br.ReadByte());
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static sbyte _BpReadSByte(this BinaryReader br) => (BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(br.ReadSByte()) : br.ReadSByte());
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static UInt16 _BpReadUInt16(this BinaryReader br) => (BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(br.ReadUInt16()) : br.ReadUInt16());
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Int16 _BpReadInt16(this BinaryReader br) => (BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(br.ReadInt16()) : br.ReadInt16());
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static UInt32 _BpReadUInt32(this BinaryReader br) => (BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(br.ReadUInt32()) : br.ReadUInt32());
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Int32 _BpReadInt32(this BinaryReader br) => (BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(br.ReadInt32()) : br.ReadInt32());
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static UInt64 _BpReadUInt64(this BinaryReader br) => (BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(br.ReadUInt64()) : br.ReadUInt64());
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Int64 _BpReadInt64(this BinaryReader br) => (BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(br.ReadInt64()) : br.ReadInt64());
#else
    // production environment
    // BinaryReader always read data in LE.
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
#endif

    public static void _BpSkip(this BinaryReader br, long offset) {
        br.BaseStream.Seek(offset, SeekOrigin.Current);
    }

    public static string _BpReadString(this BinaryReader br) {
        UInt32 length = br._BpReadUInt32();
        return Encoding.UTF8.GetString(br.ReadBytes((int)length));
    }

    public static void _BpReadNumberTuple<TItem>(this BinaryReader br, ref TItem[] data, int unit_size) where TItem : struct {
#if _BP_MODERN_CSHARP
        // modern reading
        var buffer = MemoryMarshal.Cast<TItem, byte>(data.AsSpan());
        br.Read(buffer);
        if (!BPEndianHelper._IsLittleEndian()) BPEndianHelper._BpByteSwapArray(buffer, unit_size);
#else
        // legacy reading
        var buffer = new byte[data.Length * unit_size];
        br.Read(buffer, 0, buffer.Length);
        if (!BPEndianHelper._IsLittleEndian()) BPEndianHelper._BpByteSwapArray(ref buffer, unit_size);
        if (typeof(TItem).IsEnum) {
            var intermediary = Array.CreateInstance(typeof(TItem).GetEnumUnderlyingType(), data.Length);
            Buffer.BlockCopy(buffer, 0, intermediary, 0, buffer.Length);
            Array.Copy(intermediary, data, data.Length);
        } else {
            Buffer.BlockCopy(buffer, 0, data, 0, buffer.Length);
        }
#endif
    }

    public static void _BpReadNumberList<TItem>(this BinaryReader br, ref List<TItem> data, int count, int unit_size) where TItem : struct {
        var raw = new TItem[count];
        br._BpReadNumberTuple<TItem>(ref raw, unit_size);
        data.AddRange(raw);
    }


#if _ENABLE_BP_TESTBENCH
    // debug environment
    // conditional writting
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteByte(this BinaryWriter bw, byte data) => bw.Write(BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(data) : data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteSByte(this BinaryWriter bw, sbyte data) => bw.Write(BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(data) : data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteUInt16(this BinaryWriter bw, UInt16 data) => bw.Write(BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(data) : data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteInt16(this BinaryWriter bw, Int16 data) => bw.Write(BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(data) : data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteUInt32(this BinaryWriter bw, UInt32 data) => bw.Write(BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(data) : data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteInt32(this BinaryWriter bw, Int32 data) => bw.Write(BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(data) : data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteUInt64(this BinaryWriter bw, UInt64 data) => bw.Write(BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(data) : data);
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void _BpWriteInt64(this BinaryWriter bw, Int64 data) => bw.Write(BPEndianHelper.g_ForceBigEndian ? BPEndianHelper._BpByteSwap(data) : data);
#else
    // production environment
    // BinaryWriter always write data in LE.
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
#endif

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
#if _BP_MODERN_CSHARP
        // modern writting
        Span<byte> buffer;
        if (_IsLittleEndian()) {
            buffer = MemoryMarshal.Cast<TItem, byte>(data.AsSpan());
        } else {
            // we should not affect original data. create a buffer instead.
            var intermediary = (TItem[])data.Clone();
            buffer = MemoryMarshal.Cast<TItem, byte>(intermediary.AsSpan());
            _BpByteSwapArray(buffer, unit_size);
        }

        bw.Write(buffer);
#else
        // legacy writting
        var buffer = new byte[data.Length * unit_size];
        if (typeof(TItem).IsEnum) {
            var intermediary = Array.CreateInstance(typeof(TItem).GetEnumUnderlyingType(), data.Length);
            Array.Copy(data, intermediary, data.Length);
            Buffer.BlockCopy(intermediary, 0, buffer, 0, buffer.Length);
        } else {
            Buffer.BlockCopy(data, 0, buffer, 0, buffer.Length);
        }
        if (!BPEndianHelper._IsLittleEndian()) BPEndianHelper._BpByteSwapArray(ref buffer, unit_size);
        bw.Write(buffer);
#endif
    }

    public static void _BpWriteNumberList<TItem>(this BinaryWriter bw, ref List<TItem> data, int unit_size) where TItem : struct {
#if _BP_MODERN_CSHARP
        // modern writting
        var buffer = MemoryMarshal.Cast<TItem, byte>(data.ToArray().AsSpan());
        if (!BPEndianHelper._IsLittleEndian()) BPEndianHelper._BpByteSwapArray(buffer, unit_size);
        bw.Write(buffer);
#else
        // legacy writting
        var buffer = data.ToArray();
        bw._BpWriteNumberTuple<TItem>(ref buffer, unit_size);
#endif
    }


    public static void UniformSerialize(BpMessage instance, BinaryWriter bw) {
        bw._BpWriteUInt32((UInt32)instance.GetOpCode());
        instance.Serialize(bw);
    }

}
