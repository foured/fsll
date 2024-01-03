#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <fstream>

namespace foured {
	enum class fsll_lineType {
		fsll_struct_def,
		fsll_var_def,
		fsll_struct_end,
		fsll_empty_line
	};

	struct fsll_datacell {
		fsll_datacell(std::string key, std::string value) : key(key), value(value) {}
		std::string key;
		std::string value;
	};

	class fsll {
	public:
		/// <summary>
		/// Constructor. If you create a root object, leave the arguments.
		/// </summary>
		/// <param name="name">
		/// It is advised to use: typeid(your type name / variable).name()
		/// </param>
		fsll(std::string name = "") : structName(name) {}
		template<typename T>
		void registerValue(std::string key, T val) {
			datacells.emplace_back(key, valToString(val));
		}

		void registerChildStruct(fsll child) {
			childStructs.push_back(child);
		}

		template<typename T>
		T getValue(const std::string& key, const T& defaultValue = T()) {
			for (const fsll_datacell& dc : datacells) {
				if (dc.key == key) {
					if constexpr (std::is_same_v<T, int>) {
						return std::stoi(dc.value);
					}
					else if constexpr (std::is_same_v<T, double>) {
						return std::stod(dc.value);
					}
					else if constexpr (std::is_same_v<T, std::string>) {
						return dc.value;
					}
					else if constexpr (std::is_same_v<T, char>) {
						return dc.value.empty() ? '\0' : dc.value[0];
					}
					else if constexpr (std::is_same_v<T, bool>) {
						return (dc.value == "true");
					}
				}
			}
			return defaultValue;
		}

		fsll getChildStruct(const std::string& key) {
			for (fsll cs : childStructs) {
				if (cs.structName == key) {
					return cs;
				}
			}
		}

		template<typename T>
		std::vector<fsll> getListOfStructs() {
			gen_childStructsMap();
			return childStructsMap[typeid(T).name(0)];
		}

		std::string getString() {
			std::string res = "";
			res = res + "(" + structName + ")" + "{ \n";
			for (fsll_datacell dc : datacells) {
				res = res + std::string(1, '"') + dc.key + std::string(1, '"') + ": " + dc.value + ", ";
				res += '\n';
			}

			for (fsll cu_fsll : childStructs) {
				std::vector<std::string> cu_fsll_lines = getLines(cu_fsll.getString());
				for (std::string cl : cu_fsll_lines) {
					res = res + "   " + cl + "\n";
				}
			}

			res = res +  "}\n";
			return res;
		}

		static fsll parseString(std::string str) {
			std::vector<std::string> lines = getLines(str);
			fsll cu_fsll;
			bool hadFoundDefenition = false;
			int count = 0;
			for (int i = 0, s = lines.size(); i < s; i++) {
				std::string line = lines[i];
				fsll_lineType type = getLineType(line);
				if (type == fsll_lineType::fsll_struct_def && !hadFoundDefenition) {
					cu_fsll = parseLineFor_fsll(line);
					hadFoundDefenition = true;
				}
				else if (type == fsll_lineType::fsll_struct_def && hadFoundDefenition) {
					fsll c_fsll = parseLineFor_fsll(line);
					std::string c_fsll_string = "";
					int count = 1;
					for (int j = i; j < s; j++) {
						std::string l1 = lines[j];
						count = count + isStrinContainsChar(l1, '{') - isStrinContainsChar(l1, '}');
						if (count == 0) break;
						c_fsll_string = c_fsll_string + l1 + "\n";
					}
					//std::cout << c_fsll_string << std::endl;
					cu_fsll.registerChildStruct(c_fsll.parseString(c_fsll_string));
				}
				count = count + isStrinContainsChar(line, '{') - isStrinContainsChar(line, '}');
				if (type == fsll_lineType::fsll_var_def) {
					if(count == 1)
						cu_fsll.registerValue(parseLineFor_variable(line));
				}
			}
			return cu_fsll;
		}

		void saveToFile(std::string path) {
			std::ofstream file(path);
			if (file.is_open()) {
				std::string s = getString();
				file << s << std::endl;
				file.close();
				std::cout << "File: '" + path + "' was saved. The size is " << getFileSize(path) << " byte(s)." << std::endl;
			}
			else {
				std::cout << "Error to open file: '" + path + "'." << std::endl;
			}
		}

		static fsll loadFromFile(std::string path) {
			std::ifstream file(path);

			if (file.is_open()) {
				std::stringstream buffer;
				buffer << file.rdbuf();
				std::string res = buffer.str();
				std::cout << "File save loaded: '" << path << "'." << std::endl;
				return fsll::parseString(res);
			}
			std::cout << "Error to open file: '" + path + "'." << std::endl;
			return fsll();
		}


		
	private: //private stuff
		//variables
		std::vector<fsll_datacell> datacells;
		std::vector<fsll> childStructs;
		std::string structName;
		std::map<std::string, std::vector<fsll>> childStructsMap;

		//help

		void gen_childStructsMap() {
			childStructsMap.clear();//anty multi instancing  
			for (fsll cs : childStructs) {
				childStructsMap[cs.structName].push_back(cs);
			}
		}

		std::string typeToString(fsll_lineType type) {
			if (type == fsll_lineType::fsll_struct_def) return "struct def";
			if (type == fsll_lineType::fsll_var_def) return "var def";
			if (type == fsll_lineType::fsll_struct_end) return "struct end";
			if (type == fsll_lineType::fsll_empty_line) return "empty line";
		}

		template <typename T>
		constexpr bool isVector() {
			return std::is_same_v<T, std::vector<typename T::value_type, typename T::allocator_type>> ||
				std::is_same_v<T, std::vector<typename T::value_type>>;
		}

		void registerValue(fsll_datacell datacell) {
			datacells.emplace_back(datacell);
		}

		template<typename T>
		std::string valToString(T val) {
			std::ostringstream oss;
			oss << val;
			return oss.str();
		}

		std::string getLine(std::string str, int start = 10) {
			std::string res = "";
			for (int i = start; str[i] != '\0' && str[i] != '\n'; i++) {
				res += str[i];
			}
			return res;
		}

		static fsll_lineType getLineType(std::string line) {
			line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

			if (line[0] == '"') return fsll_lineType::fsll_var_def;
			if (line[0] == '}') return fsll_lineType::fsll_struct_end;
			if (line.empty() || line == "\n") return fsll_lineType::fsll_empty_line;
			return fsll_lineType::fsll_struct_def;
		}

		static fsll parseLineFor_fsll(std::string str) {
			bool hadFoundNameStartsSumbol = false;
			std::string name;
			for (int i = 0; str[i] != '\0'; i++) {
				char c = str[i];
				if (!hadFoundNameStartsSumbol && c == '(')
					hadFoundNameStartsSumbol = true;
				else if (hadFoundNameStartsSumbol && c != '(' && c != ')')
					name += c;
				else if (c == ')')
					return fsll(name);
			}
		}

		static fsll_datacell parseLineFor_variable(std::string str) {
			str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
			bool hadFound_KeyStartSymbol = false;
			bool hadFound_Key = false;
			std::string key;
			std::string value;
			for (int i = 0; str[i] != '\0'; i++) {
				char c = str[i];
				if (!hadFound_KeyStartSymbol && !hadFound_Key && c == '"') {
					hadFound_KeyStartSymbol = true;
				}
				else if (hadFound_KeyStartSymbol && c != '"') {
					key += c;
				}
				else if (hadFound_KeyStartSymbol && c == '"') {
					hadFound_KeyStartSymbol = false;
					hadFound_Key = true;
					i ++;
				}
				else if (hadFound_Key && c != ',') {
					value += c;
				}

				if (c == ',') {
					return fsll_datacell(key, value);
				}
			}
		}

		static std::vector<std::string> getLines(const std::string& input) {
			std::vector<std::string> lines;
			std::istringstream stream(input);
			std::string line;

			while (std::getline(stream, line)) {
				lines.push_back(line);
			}

			return lines;
		}

		bool isLineEmpty(std::string line) {
			line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
			return line.empty() || line == "\n";
		}

		static bool isStrinContainsChar(std::string str, char c) {
			for (char cc : str)
				if (cc == c) return true;
			return false;
		}

		std::ifstream::pos_type getFileSize(std::string filename)
		{
			std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
			return in.tellg();
		}
	};
}
