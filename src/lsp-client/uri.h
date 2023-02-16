//
// Created by Alex on 2020/1/28.
//

#ifndef LSP_URI_H
#define LSP_URI_H

#include <cstddef>
#include <cstring>
#include <string>
#include <regex>
#include "json.hpp"
using json = nlohmann::json;
class string_ref {
public:
    static const size_t npos = ~size_t(0);
    using iterator = const char *;
    using const_iterator = const char *;
    using size_type = size_t;
private:
    const char *m_ref = nullptr;
    size_t m_length = 0;
public:
    constexpr string_ref() : m_ref(nullptr), m_length(0) {}
    constexpr string_ref(std::nullptr_t) : m_ref(nullptr), m_length(0) {}
    constexpr string_ref(const char *ref, size_t length) : m_ref(ref), m_length(length) {}
    string_ref(const char *ref) : m_ref(ref), m_length(std::strlen(ref)) {}
    string_ref(const std::string &string) : m_ref(string.c_str()), m_length(string.length()) {}
    inline operator const char*() const { return m_ref; }
    inline std::string str() const { return std::string(m_ref, m_length); }
    inline bool operator==(const string_ref &ref) const {
        return m_length == ref.m_length && strcmp(m_ref, ref.m_ref) == 0;
    }
    inline bool operator==(const char *ref) const {
        return strcmp(m_ref, ref) == 0;
    }
    inline bool operator>(const string_ref &ref) const { return m_length > ref.m_length; }
    inline bool operator<(const string_ref &ref) const { return m_length < ref.m_length; }
    inline const char *c_str() const { return m_ref; }
    inline bool empty() const { return m_length == 0; }
    iterator begin() const { return m_ref; }
    iterator end() const { return m_ref + m_length; }
    inline const char *data() const { return m_ref; }
    inline size_t size() const { return m_length; }
    inline size_t length() const { return m_length; }
    char front() const { return m_ref[0]; }
    char back() const { return m_ref[m_length - 1]; }
    char operator[](size_t index) const { return m_ref[index]; }

};
template <typename T>
class option {
public:
    T fStorage = T();
    bool fHas = false;
    constexpr option() = default;
    option(const T &y) : fStorage(y), fHas(true) {}
    option(T &&y) : fStorage(std::move(y)), fHas(true) {}
    option &operator=(T &&v) {
        fStorage = std::move(v);
        fHas = true;
        return *this;
    }
    option &operator=(const T &v) {
        fStorage = v;
        fHas = true;
        return *this;
    }
    const T *ptr() const { return &fStorage; }
    T *ptr() { return &fStorage; }
    const T &value() const { return fStorage; }
    T &value() { return fStorage; }
    bool has() const { return fHas; }
    const T *operator->() const { return ptr(); }
    T *operator->() { return ptr(); }
    const T &operator*() const { return value(); }
    T &operator*() { return value(); }
    explicit operator bool() const { return fHas; }
};
namespace nlohmann {
    template <typename T>
    struct adl_serializer<option<T>> {
        static void to_json(json& j, const option<T>& opt) {
            if (opt.has()) {
                j = opt.value();
            } else {
                j = nullptr;
            }
        }
        static void from_json(const json& j, option<T>& opt) {
            if (j.is_null()) {
                opt = option<T>();
            } else {
                opt = option<T>(j.get<T>());
            }
        }
    };
}

inline uint8_t FromHex(const char digit) {
    if (digit >= '0' && digit <= '9')
        return digit - '0';
    if (digit >= 'a' && digit <= 'f')
        return digit - 'a' + 10;
    if (digit >= 'A' && digit <= 'F')
        return digit - 'A' + 10;
    return 0;
}
inline uint8_t FromHex(const char n1, const char n2) {
    return (FromHex(n1) << 4) + FromHex(n2);
}

struct URIForFile {
    std::string file;
    static std::string UriEncode(string_ref ref) {
//        static const char *hexs = "0123456789ABCDEF";
//        static const char *symbol = "._-*/:";
//        std::string result;
//        for (uint8_t ch : ref) {
//            if (ch == '\\') {
//                ch = '/';
//            }
//            if (std::isalnum(ch) || strchr(symbol, ch)) {
//                if (ch == '/' && result.back() == '/') {
//                    continue;
//                }
//                result += ch;
//            } else if (ch == ' ') {
//                result += '+';
//            } else {
//                result += '%';
//                result += hexs[ch >> 4];
//                result += hexs[ch & 0xF];
//            }
//        }
//        return std::move(result);
return "";
    }
    explicit operator bool() const { return !file.empty(); }
    friend bool operator==(const URIForFile &LHS, const URIForFile &RHS) {
        return LHS.file == RHS.file;
    }
    friend bool operator!=(const URIForFile &LHS, const URIForFile &RHS) {
        return !(LHS == RHS);
    }
    friend bool operator<(const URIForFile &LHS, const URIForFile &RHS) {
        return LHS.file < RHS.file;
    }
    void from(string_ref path) {
        file = "file:///" + UriEncode(path);
    }
    explicit URIForFile(const char *str) : file(str) {}
    URIForFile() = default;
    inline std::string &str() { return file; }
};
using DocumentUri = string_ref;

class URI {
public:
    using sview = string_ref;
    static std::string Encode(sview input) {
        static const char *hexs = "0123456789ABCDEF";
        static const char *unreserved = "-._~";
        std::string res;
        res.reserve(input.size());
        for (auto chr : input) {
            if (std::isalnum(chr) || std::strchr(unreserved, chr)) {
                res += chr;
            } else {
                res += '%';
                res += hexs[chr >> 4];
                res += hexs[chr & 0xF];
            }
        }
        return res;
    }
    static std::string Decode(sview input) {
        //static const char *reserved = ":/?#[]@!$&'()*+,;=";
        std::string res;
        res.reserve(input.size());
        for (auto iter = input.begin(), end = input.end(); iter != end; ++iter) {
            if (*iter == '%') {
                const uint8_t n1 = (*(++iter));
                const uint8_t n2 = (*(++iter));
                res += static_cast<char>(FromHex(n1, n2));
            } else if (*iter == '+') {
                res += ' ';
            } else {
                res += *iter;
            }
        }
        return res;
    }
public:
    void parse(sview uri) {
        static const std::regex pattern{
                "^([a-zA-Z]+[\\w\\+\\-\\.]+)?(\\://)?" //< scheme
                "(([^:@]+)(\\:([^@]+))?@)?"            //< username && password
                "([^/:?#]+)?(\\:(\\d+))?"              //< hostname && port
                "([^?#]+)"                             //< path
                "(\\?([^#]*))?"                        //< query
                "(#(.*))?$"                            //< fragment
        };
        static std::cmatch parts;
        m_uri = Decode(uri);
        if (std::regex_match(m_uri.c_str(), parts, pattern)) {
            m_path = sview(m_uri.c_str() + parts.position(10), parts.length(10));
            m_scheme = parts.length(1) ?
                       sview(m_uri.c_str() + parts.position(1), parts.length(1)) : sview{};
            m_userinfo = parts.length(3) ?
                         sview(m_uri.data() + parts.position(3), parts.length(3)) : sview{};
            m_host = parts.length(7) ?
                     sview(m_uri.data() + parts.position(7), parts.length(7)) : sview{};
            m_port = parts.length(9) ?
                     sview(m_uri.data() + parts.position(9), parts.length(9)) : sview{};
            m_query = parts.length(11) ?
                      sview(m_uri.data() + parts.position(11), parts.length(11)) : sview{};
            m_fragment = parts.length(13) ?
                         sview(m_uri.data() + parts.position(13), parts.length(13)) : sview{};
        }
    }
    sview path() { return m_path; }
    sview scheme() { return m_scheme; }
    sview userinfo() { return m_userinfo; }
    sview host() { return m_host; }
    sview port() { return m_port; }
    sview query() { return m_query; }
    sview fragment() { return m_fragment; }
    sview query(sview key) {
/*
        static const std::regex query_token_pattern {"[^?=&]+"};
        auto query = query_.to_string();
        auto it  = std::sregex_iterator(query.cbegin(), query.cend(), query_token_pattern);
        auto end = std::sregex_iterator();
        while (it not_eq end) {
            auto key = it->str();
            if (++it not_eq end) {
                queries_[key] = it->str();
            } else {
                queries_[key];
            }
        }
*/

        std::regex regex("([a-zA-Z_]+)=");
        std::cmatch part;
        if (std::regex_match(m_uri.c_str(), part, regex)) {

        }

        return {};
    }
    std::string uri() {
        return Encode(m_uri);
    }

private:
    std::string m_uri;
    sview m_path;
    sview m_scheme;
    sview m_userinfo;
    sview m_host;
    sview m_port;
    sview m_query;
    sview m_fragment;

};

#endif //LSP_URI_H
