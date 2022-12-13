
public static partial class _Helper {
    public static _OpCode _BpPeekOpCode(this BinaryReader br) {
        _OpCode code = (_OpCode)br.ReadUInt32();
        br.BaseStream.Seek(4, SeekOrigin.Current);
        return code;
    }
    public static _OpCode _BpReadOpCode(this BinaryReader br) => (_OpCode)br.ReadUInt32();
    public static void _BpWriteOpCode(this BinaryWriter bw, _OpCode code) => bw.Write((UInt32)code);

    public static TInt[] _CastEnumArray2IntArray<TEnum, TInt>(TEnum[] d) where TInt : struct
        where TEnum : Enum => d.Cast<TInt>().ToArray();
    public static TEnum[] _CastIntArray2EnumArray<TEnum, TInt>(TInt[] d) where TInt : struct
        where TEnum : Enum => d.Cast<TEnum>().ToArray();

    public static byte _BpReadByte(this BinaryReader br) => br.ReadByte();
    public static sbyte _BpReadSByte(this BinaryReader br) => br.ReadSByte();
    public static UInt16 _BpReadUInt16(this BinaryReader br) => br.ReadUInt16();
    public static Int16 _BpReadInt16(this BinaryReader br) => br.ReadInt16();
    public static UInt32 _BpReadUInt32(this BinaryReader br) => br.ReadUInt32();
    public static Int32 _BpReadInt32(this BinaryReader br) => br.ReadInt32();
    public static UInt64 _BpReadUInt64(this BinaryReader br) => br.ReadUInt64();
    public static Int64 _BpReadInt64(this BinaryReader br) => br.ReadInt64();

    public static string _BpReadString(this BinaryReader br) {
        UInt32 length = br._BpReadUInt32();
        return Encoding.UTF8.GetString(br.ReadBytes((int)length));
    }

    public static Span<TInt> _BpReadAbstractIntArray<TInt>(this BinaryReader br, int count, int int_size) where TInt : struct {
        Span<byte> raw_span = br.ReadBytes(count * int_size).AsSpan();
        return MemoryMarshal.Cast<byte, TInt>(raw_span);
    }
    public static byte[] _BpReadByteArray(this BinaryReader br, int count) {
        return br.ReadBytes(count);
    }
    public static sbyte[] _BpReadSByteArray(this BinaryReader br, int count) {
        return br._BpReadAbstractIntArray<sbyte>(count, 1).ToArray();
    }
    public static UInt16[] _BpReadUInt16Array(this BinaryReader br, int count) {
        Span<UInt16> endian_span = br._BpReadAbstractIntArray<UInt16>(count, 2);
        if (!BitConverter.IsLittleEndian)
            for (int i = 0; i < endian_span.Length; ++i) {
                endian_span[i] = BinaryPrimitives.ReverseEndianness(endian_span[i]);
            }
        return endian_span.ToArray();
    }
    public static Int16[] _BpReadInt16Array(this BinaryReader br, int count) {
        Span<Int16> endian_span = br._BpReadAbstractIntArray<Int16>(count, 2);
        if (!BitConverter.IsLittleEndian)
            for (int i = 0; i < endian_span.Length; ++i) {
                endian_span[i] = BinaryPrimitives.ReverseEndianness(endian_span[i]);
            }
        return endian_span.ToArray();
    }
    public static UInt32[] _BpReadUInt32Array(this BinaryReader br, int count) {
        Span<UInt32> endian_span = br._BpReadAbstractIntArray<UInt32>(count, 4);
        if (!BitConverter.IsLittleEndian)
            for (int i = 0; i < endian_span.Length; ++i) {
                endian_span[i] = BinaryPrimitives.ReverseEndianness(endian_span[i]);
            }
        return endian_span.ToArray();
    }
    public static Int32[] _BpReadInt32Array(this BinaryReader br, int count) {
        Span<Int32> endian_span = br._BpReadAbstractIntArray<Int32>(count, 4);
        if (!BitConverter.IsLittleEndian)
            for (int i = 0; i < endian_span.Length; ++i) {
                endian_span[i] = BinaryPrimitives.ReverseEndianness(endian_span[i]);
            }
        return endian_span.ToArray();
    }
    public static UInt64[] _BpReadUInt64Array(this BinaryReader br, int count) {
        Span<UInt64> endian_span = br._BpReadAbstractIntArray<UInt64>(count, 8);
        if (!BitConverter.IsLittleEndian)
            for (int i = 0; i < endian_span.Length; ++i) {
                endian_span[i] = BinaryPrimitives.ReverseEndianness(endian_span[i]);
            }
        return endian_span.ToArray();
    }
    public static Int64[] _BpReadInt64Array(this BinaryReader br, int count) {
        Span<Int64> endian_span = br._BpReadAbstractIntArray<Int64>(count, 8);
        if (!BitConverter.IsLittleEndian)
            for (int i = 0; i < endian_span.Length; ++i) {
                endian_span[i] = BinaryPrimitives.ReverseEndianness(endian_span[i]);
            }
        return endian_span.ToArray();
    }


    public static void _BpWriteByte(this BinaryWriter bw, byte data) => bw.Write(data);
    public static void _BpWriteSByte(this BinaryWriter bw, sbyte data) => bw.Write(data);
    public static void _BpWriteUInt16(this BinaryWriter bw, UInt16 data) => bw.Write(data);
    public static void _BpWriteInt16(this BinaryWriter bw, Int16 data) => bw.Write(data);
    public static void _BpWriteUInt32(this BinaryWriter bw, UInt32 data) => bw.Write(data);
    public static void _BpWriteInt32(this BinaryWriter bw, Int32 data) => bw.Write(data);
    public static void _BpWriteUInt64(this BinaryWriter bw, UInt64 data) => bw.Write(data);
    public static void _BpWriteInt64(this BinaryWriter bw, Int64 data) => bw.Write(data);

    public static void _BpWriteString(this BinaryWriter bw, string strl) {
        var rawstr = Encoding.UTF8.GetBytes(strl);
        UInt32 length = (UInt32)rawstr.Length;
        bw._BpWriteUInt32(length);
        bw.Write(rawstr);
    }

    public static void _BpWriteAbstractIntArray<TInt>(this BinaryWriter bw, Span<TInt> endian_span, int int_size) where TInt : struct {
        Span<byte> raw_span = MemoryMarshal.Cast<TInt, byte>(endian_span);
        bw.Write(raw_span.ToArray());
    }
    public static void _BpWriteByteArray(this BinaryWriter bw, ref byte[] data) {
        bw.Write(data);
    }
    public static void _BpWriteSByteArray(this BinaryWriter bw, ref sbyte[] data) {
        bw._BpWriteAbstractIntArray<sbyte>(data.AsSpan(), 1);
    }
    public static void _BpWriteUInt16Array(this BinaryWriter bw, ref UInt16[] data) {
        Span<UInt16> endian_span;
        if (!BitConverter.IsLittleEndian) {
            endian_span = new Span<UInt16>(new UInt16[data.Length]);
            data.AsSpan().CopyTo(endian_span);
            for (int i = 0; i < endian_span.Length; ++i) {
                endian_span[i] = BinaryPrimitives.ReverseEndianness(endian_span[i]);
            }
        } else {
            endian_span = data.AsSpan();
        }
        
        bw._BpWriteAbstractIntArray<UInt16>(endian_span, 2);
    }
    public static void _BpWriteInt16Array(this BinaryWriter bw, ref Int16[] data) {
        Span<Int16> endian_span;
        if (!BitConverter.IsLittleEndian) {
            endian_span = new Span<Int16>(new Int16[data.Length]);
            data.AsSpan().CopyTo(endian_span);
            for (int i = 0; i < endian_span.Length; ++i) {
                endian_span[i] = BinaryPrimitives.ReverseEndianness(endian_span[i]);
            }
        } else {
            endian_span = data.AsSpan();
        }

        bw._BpWriteAbstractIntArray<Int16>(endian_span, 2);
    }
    public static void _BpWriteUInt32Array(this BinaryWriter bw, ref UInt32[] data) {
        Span<UInt32> endian_span;
        if (!BitConverter.IsLittleEndian) {
            endian_span = new Span<UInt32>(new UInt32[data.Length]);
            data.AsSpan().CopyTo(endian_span);
            for (int i = 0; i < endian_span.Length; ++i) {
                endian_span[i] = BinaryPrimitives.ReverseEndianness(endian_span[i]);
            }
        } else {
            endian_span = data.AsSpan();
        }

        bw._BpWriteAbstractIntArray<UInt32>(endian_span, 4);
    }
    public static void _BpWriteInt32Array(this BinaryWriter bw, ref Int32[] data) {
        Span<Int32> endian_span;
        if (!BitConverter.IsLittleEndian) {
            endian_span = new Span<Int32>(new Int32[data.Length]);
            data.AsSpan().CopyTo(endian_span);
            for (int i = 0; i < endian_span.Length; ++i) {
                endian_span[i] = BinaryPrimitives.ReverseEndianness(endian_span[i]);
            }
        } else {
            endian_span = data.AsSpan();
        }

        bw._BpWriteAbstractIntArray<Int32>(endian_span, 4);
    }
    public static void _BpWriteUInt64Array(this BinaryWriter bw, ref UInt64[] data) {
        Span<UInt64> endian_span;
        if (!BitConverter.IsLittleEndian) {
            endian_span = new Span<UInt64>(new UInt64[data.Length]);
            data.AsSpan().CopyTo(endian_span);
            for (int i = 0; i < endian_span.Length; ++i) {
                endian_span[i] = BinaryPrimitives.ReverseEndianness(endian_span[i]);
            }
        } else {
            endian_span = data.AsSpan();
        }

        bw._BpWriteAbstractIntArray<UInt64>(endian_span, 8);
    }
    public static void _BpWriteInt64Array(this BinaryWriter bw, ref Int64[] data) {
        Span<Int64> endian_span;
        if (!BitConverter.IsLittleEndian) {
            endian_span = new Span<Int64>(new Int64[data.Length]);
            data.AsSpan().CopyTo(endian_span);
            for (int i = 0; i < endian_span.Length; ++i) {
                endian_span[i] = BinaryPrimitives.ReverseEndianness(endian_span[i]);
            }
        } else {
            endian_span = data.AsSpan();
        }

        bw._BpWriteAbstractIntArray<Int64>(endian_span, 8);
    }
}

public abstract class _BpMessage {
    public virtual _OpCode GetOpCode() { throw new NotImplementedException(); }
    public virtual bool GetIsReliable() { throw new NotImplementedException(); }
    public virtual void Serialize(BinaryWriter _bw) { throw new NotImplementedException(); }
    public virtual void Deserialize(BinaryReader _br) { throw new NotImplementedException(); }

}
