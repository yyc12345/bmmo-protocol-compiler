using System;
using System.IO;
using System.Linq;
using System.Collections.Generic;
using System.Reflection;
using System.Diagnostics;
using BP = TestNamespace.EmptyName1.EmptyName2;

namespace CSharpTestbench {
    class Program {

        private static readonly string DATA_FOLDER_NAME = "TestbenchData";
        private static readonly string THIS_LANG = "cs";
        private static readonly string REF_LANG = "py";

        private static readonly int DEFAULT_INT = 114;
        private static readonly float DEFAULT_FLOAT = 114.514f;
        private static readonly string DEFAULT_STR = "shit";
        private static readonly int DEFAULT_LIST_LEN = 5;

        private static readonly int BENCHMARK_TIMES = 100;

        static void GetStructMsgsName(ref List<Type> msgs) {
            var nsname = typeof(BP.OpCode).Namespace;
            var bpmessage_t = typeof(BP.BpMessage);
            var bpstruct_t = typeof(BP.BpStruct);
            var classes = from ty in Assembly.GetExecutingAssembly().GetTypes()
                          where ty.IsClass && ty.Namespace == nsname && ty.IsSubclassOf(bpstruct_t) && ty.Name != bpstruct_t.Name && ty.Name != bpmessage_t.Name
                          select ty;

            msgs.AddRange(from ty in classes
                          where ty.IsSubclassOf(bpmessage_t)
                          select ty);
            //structs.AddRange(from ty in classes
            //                 where (!ty.IsSubclassOf(bpmessage_t))
            //                 select ty);
        }

        static object GenerateDefaultValue(Type t) {
            // process enum first. reveal its unerlying type first
            Type ot = t;
            if (ot.IsEnum) {
                t = t.GetEnumUnderlyingType();
            }

            object instance;
            if (t == typeof(byte)) instance = (byte)DEFAULT_INT;
            else if (t == typeof(sbyte)) instance = (sbyte)DEFAULT_INT;
            else if (t == typeof(ushort)) instance = (ushort)DEFAULT_INT;
            else if (t == typeof(short)) instance = (short)DEFAULT_INT;
            else if (t == typeof(uint)) instance = (uint)DEFAULT_INT;
            else if (t == typeof(int)) instance = DEFAULT_INT;
            else if (t == typeof(ulong)) instance = (ulong)DEFAULT_INT;
            else if (t == typeof(long)) instance = (long)DEFAULT_INT;
            else if (t == typeof(float)) instance = DEFAULT_FLOAT;
            else if (t == typeof(double)) instance = (double)DEFAULT_FLOAT;
            else if (t == typeof(string)) instance = DEFAULT_STR;
            else {
                // this is a complex structure
                instance = Activator.CreateInstance(t);
                AssignVariablesValue(instance);
            }

            // process enum finally
            if (ot.IsEnum) {
                return Enum.ToObject(ot, instance);
            } else {
                return instance;
            }
        }
        static void AssignVariablesValue(object instance) {
            Type instance_t = instance.GetType();

            foreach (var member in instance_t.GetFields(BindingFlags.Public | BindingFlags.Instance)) {
                Type member_t = member.FieldType;

                if (member_t.IsGenericType) {
                    // dynamic array
                    var item_t = member_t.GetGenericArguments()[0];

                    member_t.GetMethod("Clear").Invoke(member.GetValue(instance), null);
                    for (int i = 0; i < DEFAULT_LIST_LEN; ++i) {
                        member_t.GetMethod("Add").Invoke(member.GetValue(instance), new[] { GenerateDefaultValue(item_t) });
                    }
                } else if (member_t.IsArray) {
                    // static array
                    var item_t = member_t.GetElementType();
                    var absarray = (member.GetValue(instance) as Array);

                    for (int i = 0; i < absarray.Length; ++i) {
                        absarray.SetValue(GenerateDefaultValue(item_t), i);
                    }
                } else {
                    // single
                    member.SetValue(instance, GenerateDefaultValue(member_t));
                }

            }
        }

        static bool CompareInstanceMember(Type ty, object member1, object member2) {
            // process enum first
            if (ty.IsEnum) {
                ty = ty.GetEnumUnderlyingType();
            }

            if (ty == typeof(byte)) return (byte)member1 == (byte)member2;
            else if (ty == typeof(sbyte)) return (sbyte)member1 == (sbyte)member2;
            else if (ty == typeof(ushort)) return (ushort)member1 == (ushort)member2;
            else if (ty == typeof(short)) return (short)member1 == (short)member2;
            else if (ty == typeof(uint)) return (uint)member1 == (uint)member2;
            else if (ty == typeof(int)) return (int)member1 == (int)member2;
            else if (ty == typeof(ulong)) return (ulong)member1 == (ulong)member2;
            else if (ty == typeof(long)) return (long)member1 == (long)member2;
            else if (ty == typeof(float)) return (float)member1 == (float)member2;
            else if (ty == typeof(double)) return (double)member1 == (double)member2;
            else if (ty == typeof(string)) return (string)member1 == (string)member2;
            else {
                // this is a complex structure
                return CompareInstance(ty, member1, member2);
            }
        }
        static bool CompareInstance(Type ty, object instance1, object instance2) {
            foreach (var member in ty.GetFields(BindingFlags.Public | BindingFlags.Instance)) {
                // pre check
                object instmem1 = member.GetValue(instance1);
                object instmem2 = member.GetValue(instance2);

                if (instance1.GetType() != instance2.GetType()) return false;

                // data check
                Type member_t = member.FieldType;
                if (member_t.IsGenericType) {
                    // dynamic array
                    var item_t = member_t.GetGenericArguments()[0];

                    // check List<T> length
                    int len1 = (int)member_t.InvokeMember("Count", BindingFlags.Public | BindingFlags.Instance | BindingFlags.GetProperty, null, instmem1, null);
                    int len2 = (int)member_t.InvokeMember("Count", BindingFlags.Public | BindingFlags.Instance | BindingFlags.GetProperty, null, instmem2, null);
                    if (len1 != len2) {
                        return false;
                    }

                    // check every item
                    for (int i = 0; i < len1; ++i) {
                        if (!CompareInstanceMember(item_t,
                            member_t.InvokeMember("Item", BindingFlags.Public | BindingFlags.Instance | BindingFlags.GetProperty, null, instmem1, new object[] { i }),
                            member_t.InvokeMember("Item", BindingFlags.Public | BindingFlags.Instance | BindingFlags.GetProperty, null, instmem2, new object[] { i })
                            )) {
                            return false;
                        }
                    }
                } else if (member_t.IsArray) {
                    // static array
                    var item_t = member_t.GetElementType();

                    var absarray1 = (instmem1 as Array);
                    var absarray2 = (instmem2 as Array);

                    // check length
                    int len1 = absarray1.Length;
                    int len2 = absarray2.Length;
                    if (len1 != len2) {
                        return false;
                    }

                    // check every item
                    for (int i = 0; i < len1; ++i) {
                        if (!CompareInstanceMember(item_t,
                            absarray1.GetValue(i),
                            absarray2.GetValue(i)
                            )) {
                            return false;
                        }
                    }
                } else {
                    // single
                    if (!CompareInstanceMember(member_t, instmem1, instmem2)) {
                        return false;
                    }
                }

            }

            return true;
        }

        static void BenchmarkTest(List<Type> msgs) {
            MemoryStream ms = new MemoryStream();
            ms.Capacity = 1024;

            foreach (var msg in msgs) {
                // create example first
                var instance = (BP.BpMessage)Activator.CreateInstance(msg);
                AssignVariablesValue(instance);
                Console.WriteLine($"Benchmarking {msg.Name}...");

                // create some variables for benchmark
                var watch = new Stopwatch();
                var br = new BinaryReader(ms);
                var bw = new BinaryWriter(ms);
                // pre-run to remove any possible spike
                instance.Serialize(bw);
                ms.Seek(0, SeekOrigin.Begin);
                instance.Deserialize(br);
                ms.SetLength(0);
                BP.BPHelper.UniformSerialize(instance, bw);
                ms.Seek(0, SeekOrigin.Begin);
                BP.BPHelper.UniformDeserialize(br);
                ms.SetLength(0);
                // pure benchmark
                for (int i = 0; i < BENCHMARK_TIMES; ++i) {
                    watch.Start();
                    instance.Serialize(bw);
                    watch.Stop();
                    ms.Seek(0, SeekOrigin.Begin);
                }
                var pureser_msec = watch.Elapsed.TotalMilliseconds / BENCHMARK_TIMES;
                watch.Reset();
                for (int i = 0; i < BENCHMARK_TIMES; ++i) {
                    watch.Start();
                    instance.Deserialize(br);
                    watch.Stop();
                    ms.Seek(0, SeekOrigin.Begin);
                }
                var puredeser_msec = watch.Elapsed.TotalMilliseconds / BENCHMARK_TIMES;
                watch.Reset();
                ms.SetLength(0);
                // non-pure benchmark
                for (int i = 0; i < BENCHMARK_TIMES; ++i) {
                    watch.Start();
                    BP.BPHelper.UniformSerialize(instance, bw);
                    watch.Stop();
                    ms.Seek(0, SeekOrigin.Begin);
                }
                var ser_msec = watch.Elapsed.TotalMilliseconds / BENCHMARK_TIMES;
                watch.Reset();
                for (int i = 0; i < BENCHMARK_TIMES; ++i) {
                    watch.Start();
                    BP.BPHelper.UniformDeserialize(br);
                    watch.Stop();
                    ms.Seek(0, SeekOrigin.Begin);
                }
                var deser_msec = watch.Elapsed.TotalMilliseconds / BENCHMARK_TIMES;
                watch.Reset();
                ms.SetLength(0);

                // output result
                Console.WriteLine($"Serialize (uniform/spec) {ser_msec * 1e3:f7}/{pureser_msec * 1e3:f7} usec. Deserialize (uniform/spec) {deser_msec * 1e3:f7}/{puredeser_msec * 1e3:f7} usec.");
            }
        }

        static bool[] g_EndianSwitches = new bool[] { false, true };
        static string GetEndianStr(bool is_bigendian) {
            return is_bigendian ? "BE" : "LE";
        }
        static string GetFilename(string ident_lang, string ident_msg, bool is_bigendian) {
            return System.IO.Path.Combine(DATA_FOLDER_NAME, ident_lang, GetEndianStr(is_bigendian) + "_" + ident_msg + ".bin");
        }
        static void LangInteractionTest(List<Type> msgs) {
            // create a list. write standard msg. and write them to file
            var standard = new Dictionary<string, BP.BpMessage>();
            foreach (var msg in msgs) {
                var instance = (BP.BpMessage)Activator.CreateInstance(msg);
                AssignVariablesValue(instance);
                standard.Add(msg.Name, instance);

                foreach (bool is_bigendian in g_EndianSwitches) {
                    // open switch
                    BP.BPEndianHelper.g_ForceBigEndian = is_bigendian;
                    // write file
                    using (var fs = new FileStream(GetFilename(THIS_LANG, msg.Name, is_bigendian), FileMode.Create, FileAccess.Write, FileShare.None)) {
                        instance.Serialize(new BinaryWriter(fs));
                    }
                    // reset switch
                    BP.BPEndianHelper.g_ForceBigEndian = false;
                }
            }

            // read from file. and compare it with standard
            foreach (var msg in msgs) {
                Console.WriteLine($"Checking {msg.Name} data correction...");

                // read from file
                var instance = (BP.BpMessage)Activator.CreateInstance(msg);
                foreach (bool is_bigendian in g_EndianSwitches) {
                    // open switch
                    BP.BPEndianHelper.g_ForceBigEndian = is_bigendian;
                    // read file
                    using (var fs = new FileStream(GetFilename(REF_LANG, msg.Name, is_bigendian), FileMode.Open, FileAccess.Read, FileShare.None)) {
                        instance.Deserialize(new BinaryReader(fs));
                    }
                    // reset switch
                    BP.BPEndianHelper.g_ForceBigEndian = false;
                }

                // compare data
                if (!CompareInstance(msg, instance, standard[msg.Name])) {
                    Console.WriteLine("Failed on data correction check!");
                }
            }
        }

        static void Main(string[] args) {
            // get necessary variables
            List<Type> msgs = new List<Type>();
            GetStructMsgsName(ref msgs);

            Console.WriteLine("===== Serialize & Deserialize Benchmark =====");
            BenchmarkTest(msgs);

            Console.WriteLine("===== Language Interaction =====");
            LangInteractionTest(msgs);
        }
    }
}