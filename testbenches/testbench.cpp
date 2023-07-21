#include "CodeGenTest.hpp"
#include <cstdio>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <array>
#include <sstream>

namespace BP = TestNamespace::EmptyName1::EmptyName2;

namespace CppTestbench {

	const std::string DATA_FOLDER_NAME = "TestbenchData";
	const std::string THIS_LANG = "cpp";
	const std::string REF_LANG = "py";

	constexpr const uint8_t DEFAULT_INT = UINT8_C(114);
	constexpr const float DEFAULT_FLOAT = 114.514f;
	const std::string DEFAULT_STR = "shit";
	constexpr const uint32_t DEFAULT_LIST_LEN = UINT32_C(5);

	constexpr const int BENCHMARK_TIMES = 100;

	static void* GetPayloadPtr(const std::string& instance_t, void* msgcls) {
		auto search = BP::_BPTestbench_StructLayouts.find(instance_t);
		if (search == BP::_BPTestbench_StructLayouts.end()) throw std::runtime_error("invalid struct name!");

		return reinterpret_cast<char*>(msgcls) + search->second.mPayloadOffset;
	}

	static const std::vector<BP::_BPTestbench_VariableProperty>& PickVariableProperty(const std::string& instance_t) {
		auto search = BP::_BPTestbench_StructLayouts.find(instance_t);
		if (search == BP::_BPTestbench_StructLayouts.end()) throw std::runtime_error("invalid struct name!");
		return search->second.mVariableProperties;
	}

	static void Stream2File(std::stringstream& ss, std::filesystem::path filepath) {
		std::ofstream fs;
		fs.open(filepath, std::ios_base::out | std::ios_base::binary);

		ss.seekg(0, std::ios_base::beg);
		ss.seekp(0, std::ios_base::beg);
		auto inbuf = ss.str();
		fs.write(inbuf.c_str(), inbuf.size());

		fs.close();
	}

	static void File2Stream(std::stringstream& ss, std::filesystem::path filepath) {
		std::ifstream fs;
		fs.open(filepath, std::ios_base::in | std::ios_base::binary);

		std::string obuf;
		fs.seekg(0, std::ios_base::end);
		obuf.resize(static_cast<size_t>(fs.tellg()));
		fs.seekg(0, std::ios_base::beg);
		fs.read(obuf.data(), obuf.size());

		ss.str(obuf);
		ss.seekg(0, std::ios_base::beg);
		ss.seekp(0, std::ios_base::beg);
		ss.clear();
		fs.close();
	}

	static void AssignVariablesValue(const std::string& instance_t, void* instance) {
		auto& entry = PickVariableProperty(instance_t);

		for (const auto& item : entry) {
			// find data ptr according to container type
			void* dataptr = reinterpret_cast<char*>(instance) + item.mOffset;
			size_t datalen = 1;
			switch (item.mContainerType) {
				case BP::_BPTestbench_ContainerType::Single:
					datalen = 1;
					break;
				case BP::_BPTestbench_ContainerType::Tuple:
					datalen = item.mTupleSize;
					break;	// these are plain type.
				case BP::_BPTestbench_ContainerType::List:
					// resize it first
					item.mListResizeFuncPtr(dataptr, static_cast<BP::_BPTestbench_VecSize_t>(DEFAULT_LIST_LEN));
					// calling list function to get addr.
					dataptr = item.mListDataFuncPtr(dataptr);
					datalen = static_cast<size_t>(DEFAULT_LIST_LEN);
					break;
				default: throw std::runtime_error("assert not reach!");
			}

			// assign value according to different type
			for (size_t i = 0; i < datalen; ++i) {
				switch (item.mBasicType) {
					case BP::_BPTestbench_BasicType::u8:
					case BP::_BPTestbench_BasicType::i8:
						*reinterpret_cast<uint8_t*>(dataptr) = static_cast<uint8_t>(DEFAULT_INT);
						break;
					case BP::_BPTestbench_BasicType::u16:
					case BP::_BPTestbench_BasicType::i16:
						*reinterpret_cast<uint16_t*>(dataptr) = static_cast<uint16_t>(DEFAULT_INT);
						break;
					case BP::_BPTestbench_BasicType::u32:
					case BP::_BPTestbench_BasicType::i32:
						*reinterpret_cast<uint32_t*>(dataptr) = static_cast<uint32_t>(DEFAULT_INT);
						break;
					case BP::_BPTestbench_BasicType::u64:
					case BP::_BPTestbench_BasicType::i64:
						*reinterpret_cast<uint64_t*>(dataptr) = static_cast<uint64_t>(DEFAULT_INT);
						break;
					case BP::_BPTestbench_BasicType::f32:
						*reinterpret_cast<float*>(dataptr) = static_cast<float>(DEFAULT_FLOAT);
						break;
					case BP::_BPTestbench_BasicType::f64:
						*reinterpret_cast<double*>(dataptr) = static_cast<double>(DEFAULT_FLOAT);
						break;
					case BP::_BPTestbench_BasicType::bpstr:
						*reinterpret_cast<std::string*>(dataptr) = DEFAULT_STR;
						break;
					case BP::_BPTestbench_BasicType::bpstruct:
						// recursive calling this function
						AssignVariablesValue(item.mComplexType, dataptr);
						break;
					default: throw std::runtime_error("assert not reach!");
				}

				// inc data ptr
				dataptr = reinterpret_cast<char*>(dataptr) + item.mUnitSizeof;
			}
		}
	}

	static bool CompareInstance(const std::string& instance_t, void* instance1, void* instance2) {
		auto& entry = PickVariableProperty(instance_t);

		for (const auto& item : entry) {
			// find data ptr according to container type
			void* dataptr1 = reinterpret_cast<char*>(instance1) + item.mOffset;
			void* dataptr2 = reinterpret_cast<char*>(instance2) + item.mOffset;

			size_t datalen = 1;
			switch (item.mContainerType) {
				case BP::_BPTestbench_ContainerType::Single:
					datalen = 1;
					break;
				case BP::_BPTestbench_ContainerType::Tuple:
					datalen = item.mTupleSize;
					break;	// these are plain type.
				case BP::_BPTestbench_ContainerType::List:
				{
					// check whether their size is equal
					auto listlen1 = item.mListSizeFuncPtr(dataptr1);
					auto listlen2 = item.mListSizeFuncPtr(dataptr2);
					if (listlen1 != listlen2) {
						return false;
					}

					// now, we can safely set data ptr
					datalen = listlen1;
					dataptr1 = item.mListDataFuncPtr(dataptr1);
					dataptr2 = item.mListDataFuncPtr(dataptr2);

					break;
				}
				default: throw std::runtime_error("assert not reach!");
			}

			// compare value according to different type
			for (size_t i = 0; i < datalen; ++i) {
				switch (item.mBasicType) {
					case BP::_BPTestbench_BasicType::u8:
					case BP::_BPTestbench_BasicType::i8:
						if (*reinterpret_cast<uint8_t*>(dataptr1) != *reinterpret_cast<uint8_t*>(dataptr2))
							return false;
						break;
					case BP::_BPTestbench_BasicType::u16:
					case BP::_BPTestbench_BasicType::i16:
						if (*reinterpret_cast<uint16_t*>(dataptr1) != *reinterpret_cast<uint16_t*>(dataptr2))
							return false;
						break;
					case BP::_BPTestbench_BasicType::u32:
					case BP::_BPTestbench_BasicType::i32:
						if (*reinterpret_cast<uint32_t*>(dataptr1) != *reinterpret_cast<uint32_t*>(dataptr2))
							return false;
						break;
					case BP::_BPTestbench_BasicType::u64:
					case BP::_BPTestbench_BasicType::i64:
						if (*reinterpret_cast<uint64_t*>(dataptr1) != *reinterpret_cast<uint64_t*>(dataptr2))
							return false;
						break;
					case BP::_BPTestbench_BasicType::f32:
						if (*reinterpret_cast<float*>(dataptr1) != *reinterpret_cast<float*>(dataptr2))
							return false;
						break;
					case BP::_BPTestbench_BasicType::f64:
						if (*reinterpret_cast<double*>(dataptr1) != *reinterpret_cast<double*>(dataptr2))
							return false;
						break;
					case BP::_BPTestbench_BasicType::bpstr:
						if (*reinterpret_cast<std::string*>(dataptr1) != *reinterpret_cast<std::string*>(dataptr2))
							return false;
						break;
					case BP::_BPTestbench_BasicType::bpstruct:
						// recursive calling this function
						if (!CompareInstance(item.mComplexType, dataptr1, dataptr2))
							return false;
						break;
					default: throw std::runtime_error("assert not reach!");
				}

				// inc data ptr
				dataptr1 = reinterpret_cast<char*>(dataptr1) + item.mUnitSizeof;
				dataptr2 = reinterpret_cast<char*>(dataptr2) + item.mUnitSizeof;
			}

		}

		return true;
	}

	static void BenchmarkTest() {
		std::stringstream buffer;

		for (uint32_t id = 0; id < BP::_BPTestbench_MessageList.size(); ++id) {
			// output info
			printf("Benchmarking %s...\n", BP::_BPTestbench_MessageList[id].c_str());

			// create exmaple instance
			BP::BpMessage* instance = BP::BPHelper::MessageFactory(static_cast<BP::OpCode>(id));
			void* payload = GetPayloadPtr(BP::_BPTestbench_MessageList[id], instance);
			AssignVariablesValue(BP::_BPTestbench_MessageList[id], payload);

			std::chrono::steady_clock::time_point time_start, time_end;

			// pure benchmark
			time_start = std::chrono::steady_clock::now();
			for (int i = 0; i < BENCHMARK_TIMES; ++i) {
				instance->Serialize(buffer);
				buffer.seekp(0, std::ios_base::beg);
			}
			time_end = std::chrono::steady_clock::now();
			double pure_ser_usec = std::chrono::duration<double, std::micro>(time_end - time_start).count() / BENCHMARK_TIMES;

			time_start = std::chrono::steady_clock::now();
			for (int i = 0; i < BENCHMARK_TIMES; ++i) {
				instance->Deserialize(buffer);
				buffer.seekg(0, std::ios_base::beg);
			}
			time_end = std::chrono::steady_clock::now();
			double pure_deser_usec = std::chrono::duration<double, std::micro>(time_end - time_start).count() / BENCHMARK_TIMES;

			// restore instance and clear buffer
			AssignVariablesValue(BP::_BPTestbench_MessageList[id], payload);
			buffer.str("");
			buffer.clear();
			// uniform benchmark
			time_start = std::chrono::steady_clock::now();
			for (int i = 0; i < BENCHMARK_TIMES; ++i) {
				BP::BPHelper::UniformSerialize(buffer, instance);
				buffer.seekp(0, std::ios_base::beg);
			}
			time_end = std::chrono::steady_clock::now();
			double ser_usec = std::chrono::duration<double, std::micro>(time_end - time_start).count() / BENCHMARK_TIMES;

			std::vector<BP::BpMessage*> temp;
			temp.resize(BENCHMARK_TIMES);
			time_start = std::chrono::steady_clock::now();
			for (int i = 0; i < BENCHMARK_TIMES; ++i) {
				temp[i] = BP::BPHelper::UniformDeserialize(buffer);		// it will produce useless instance. free it after benchmark
				buffer.seekg(0, std::ios_base::beg);
			}
			time_end = std::chrono::steady_clock::now();
			for (const auto& ptr : temp) {
				delete ptr;
			}
			double deser_usec = std::chrono::duration<double, std::micro>(time_end - time_start).count() / BENCHMARK_TIMES;

			// output result
			printf("Serialize (uniform/spec) %.7f/%.7f usec. Deserialize (uniform/spec) %.7f/%.7f usec.\n",
				ser_usec, pure_ser_usec, deser_usec, pure_deser_usec
			);

			// clear
			delete instance;
			buffer.str("");
			buffer.clear();
		}
	}

	static std::array<bool, 2> g_ForceBigEndianTuple{ true, false };
	static void LangInteractionTest() {
		std::filesystem::path wd(DATA_FOLDER_NAME);

		// create a list. write standard msg. and write them to file
		std::vector<BP::BpMessage*> standard;
		for (uint32_t id = 0; id < BP::_BPTestbench_MessageList.size(); ++id) {
			auto& msgname = BP::_BPTestbench_MessageList[id];

			BP::BpMessage* instance = BP::BPHelper::MessageFactory(static_cast<BP::OpCode>(id));
			void* payload = GetPayloadPtr(msgname, instance);
			AssignVariablesValue(msgname, payload);
			
			// add into list
			standard.emplace_back(instance);

			// write into file with 2 different endian
			for (auto& is_bigendian : g_ForceBigEndianTuple) {
				// set endian
				BP::BPHelper::ByteSwap::g_ForceBigEndian = is_bigendian;

				// write file
				std::ofstream fs;
				fs.open(wd / THIS_LANG / ((is_bigendian ? "BE_" : "LE_") + msgname + ".bin"), std::ios_base::out | std::ios_base::binary);
				bool hr = instance->Serialize(fs);
				fs.close();
				if (!hr) {
					throw std::runtime_error("unexpected failed on serialization!");
				}

				// reset endian
				BP::BPHelper::ByteSwap::g_ForceBigEndian = false;
			}
		}

		// read from file. and compare it with standard
		for (uint32_t id = 0; id < BP::_BPTestbench_MessageList.size(); ++id) {
			auto& msgname = BP::_BPTestbench_MessageList[id];

			printf("Checking %s data correction...\n", msgname.c_str());

			// read from file with 2 different endian
			for (auto& is_bigendian : g_ForceBigEndianTuple) {
				// set endian
				BP::BPHelper::ByteSwap::g_ForceBigEndian = is_bigendian;

				// read from file
				BP::BpMessage* instance = BP::BPHelper::MessageFactory(static_cast<BP::OpCode>(id));

				std::ifstream fs;
				fs.open(wd / REF_LANG / ((is_bigendian ? "BE_" : "LE_") + msgname + ".bin"), std::ios_base::in | std::ios_base::binary);
				bool hr = instance->Deserialize(fs);
				fs.close();

				// compare data
				if (!hr || !CompareInstance(msgname, GetPayloadPtr(msgname, instance), GetPayloadPtr(msgname, standard[id]))) {
					printf("Failed on data correction check!\n");
				}

				// reset endian
				BP::BPHelper::ByteSwap::g_ForceBigEndian = false;

				// clear buf
				delete instance;
			}
		}

		// free standard
		for (const auto& ptr : standard) {
			delete ptr;
		}
	}

}


int main(int argc, char* args[]) {

	puts("===== Serialize & Deserialize Benchmark =====\n");
	CppTestbench::BenchmarkTest();

	puts("===== Language Interaction =====\n");
	CppTestbench::LangInteractionTest();

	return 0;
}
