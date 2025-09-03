#pragma once

#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>

enum class BasicKind : std::uint8_t { Bool, Int, Float, Struct };

template <typename T> struct TypeTraits;
template <> struct TypeTraits<bool>  { static constexpr BasicKind Kind = BasicKind::Bool;  static const char* Name(){ return "bool";  } };
template <> struct TypeTraits<int>   { static constexpr BasicKind Kind = BasicKind::Int;   static const char* Name(){ return "int";   } };
template <> struct TypeTraits<float> { static constexpr BasicKind Kind = BasicKind::Float; static const char* Name(){ return "float"; } };

inline std::string ToString(bool v)  { return v ? "true" : "false"; }
inline std::string ToString(int v)   { return std::to_string(v); }
inline std::string ToString(float v) { std::ostringstream Oss; Oss.imbue(std::locale::classic()); Oss << v; return Oss.str(); }

inline bool FromString(const std::string& s, bool& Out) {
    std::string t = s; std::ranges::transform(t, t.begin(), ::tolower);
    if (t=="true" || t=="1")  { Out = true;  return true; }
    if (t=="false"|| t=="0")  { Out = false; return true; }
    return false;
}
inline bool FromString(const std::string& s, int& Out)   { try { Out = std::stoi(s); return true; } catch(...) { return false; } }
inline bool FromString(const std::string& s, float& Out) { try { Out = std::stof(s); return true; } catch(...) { return false; } }

