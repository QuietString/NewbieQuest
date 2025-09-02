#pragma once

#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <locale>

enum class BasicKind { Bool, Int, Float, Struct };

template <typename T> struct TypeTraits;
template <> struct TypeTraits<bool>  { static constexpr BasicKind Kind = BasicKind::Bool;  static const char* Name(){ return "bool";  } };
template <> struct TypeTraits<int>   { static constexpr BasicKind Kind = BasicKind::Int;   static const char* Name(){ return "int";   } };
template <> struct TypeTraits<float> { static constexpr BasicKind Kind = BasicKind::Float; static const char* Name(){ return "float"; } };

inline std::string ToString(bool v)  { return v ? "true" : "false"; }
inline std::string ToString(int v)   { return std::to_string(v); }
inline std::string ToString(float v) { std::ostringstream oss; oss.imbue(std::locale::classic()); oss << v; return oss.str(); }

inline bool FromString(const std::string& s, bool& out) {
    std::string t = s; std::ranges::transform(t, t.begin(), ::tolower);
    if (t=="true" || t=="1")  { out = true;  return true; }
    if (t=="false"|| t=="0")  { out = false; return true; }
    return false;
}
inline bool FromString(const std::string& s, int& out)   { try { out = std::stoi(s); return true; } catch(...) { return false; } }
inline bool FromString(const std::string& s, float& out) { try { out = std::stof(s); return true; } catch(...) { return false; } }

