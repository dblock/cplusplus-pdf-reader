#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pdf {

class Error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

namespace limits {
constexpr std::size_t fileBytes = 64 * 1024 * 1024;
constexpr std::size_t streamBytes = 32 * 1024 * 1024;
constexpr std::size_t totalDecodedBytes = 64 * 1024 * 1024;
constexpr std::size_t outputBytes = 64 * 1024 * 1024;
constexpr std::size_t nodes = 1000000;
constexpr std::size_t objects = 100000;
constexpr std::size_t depth = 64;
constexpr std::size_t revisions = 128;
}

struct PDFObjectRef {
    std::int64_t number = 0;
    std::int64_t generation = 0;
};

struct PDFObjects {
    enum class Type { Null, Boolean, Integer, Real, Name, String, Array, Dictionary, Reference };
    Type type = Type::Null;
    bool boolean = false;
    std::int64_t integer = 0;
    double real = 0;
    std::string text;
    std::vector<PDFObjects> array;
    std::map<std::string, PDFObjects> dictionary;
    PDFObjectRef reference;
    bool hasStream = false;
    std::string_view stream;

    const PDFObjects& get(const std::string& name) const;
    bool has(const std::string& name) const;
    bool isName(std::string_view name) const;
    bool isNumber() const;
};

class Parser {
public:
    Parser(std::string_view input, std::size_t& nodes, std::size_t position = 0);
    void skip();
    bool end();
    bool take(std::string_view keyword);
    void expect(std::string_view keyword);
    std::string_view word();
    std::int64_t unsignedInteger();
    PDFObjects value(std::size_t depth = 0, bool references = true);
    std::size_t position() const { return position_; }
    void position(std::size_t position);
    void streamEol();
    void endStream();
    void revisionEnd(std::size_t xrefOffset);
    char peek();
    [[noreturn]] void fail(const std::string& message) const;
    static bool whitespace(char c);
    static bool delimiter(char c);

private:
    std::string_view input_;
    std::size_t& nodes_;
    std::size_t position_;
    std::string literalString();
    std::string hexString();
    std::string name();
};

void appendBounded(std::string& target, std::string_view value,
                   std::size_t maximum = limits::outputBytes);
std::string metadataString(std::string_view bytes);
std::string StreamToString(std::string_view stream, std::size_t& nodes);

}
