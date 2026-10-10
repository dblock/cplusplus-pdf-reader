#include "PDFObjects.h"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace pdf {
namespace {
const PDFObjects nullObject;

int hexDigit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool digit(char c) { return c >= '0' && c <= '9'; }

void utf8(std::string& result, std::uint32_t c) {
    if (c < 0x80) result += static_cast<char>(c);
    else if (c < 0x800) {
        result += static_cast<char>(0xc0 | (c >> 6));
        result += static_cast<char>(0x80 | (c & 63));
    } else if (c < 0x10000) {
        result += static_cast<char>(0xe0 | (c >> 12));
        result += static_cast<char>(0x80 | ((c >> 6) & 63));
        result += static_cast<char>(0x80 | (c & 63));
    } else {
        result += static_cast<char>(0xf0 | (c >> 18));
        result += static_cast<char>(0x80 | ((c >> 12) & 63));
        result += static_cast<char>(0x80 | ((c >> 6) & 63));
        result += static_cast<char>(0x80 | (c & 63));
    }
}
}

const PDFObjects& PDFObjects::get(const std::string& name) const {
    const auto found = dictionary.find(name);
    return found == dictionary.end() ? nullObject : found->second;
}

bool PDFObjects::has(const std::string& name) const { return dictionary.count(name) != 0; }
bool PDFObjects::isName(std::string_view name) const { return type == Type::Name && text == name; }
bool PDFObjects::isNumber() const { return type == Type::Integer || type == Type::Real; }

Parser::Parser(std::string_view input, std::size_t& nodes, std::size_t position)
    : input_(input), nodes_(nodes), position_(position) {
    if (position > input.size()) fail("offset outside input");
}

bool Parser::whitespace(char c) {
    return c == 0 || c == '\t' || c == '\n' || c == '\f' || c == '\r' || c == ' ';
}

bool Parser::delimiter(char c) {
    return whitespace(c) || c == '(' || c == ')' || c == '<' || c == '>' ||
           c == '[' || c == ']' || c == '{' || c == '}' || c == '/' || c == '%';
}

void Parser::skip() {
    while (position_ < input_.size()) {
        if (whitespace(input_[position_])) ++position_;
        else if (input_[position_] == '%') {
            while (position_ < input_.size() && input_[position_] != '\r' &&
                   input_[position_] != '\n') ++position_;
        } else break;
    }
}

bool Parser::end() { skip(); return position_ == input_.size(); }
char Parser::peek() { skip(); return position_ < input_.size() ? input_[position_] : '\0'; }

bool Parser::take(std::string_view keyword) {
    skip();
    if (input_.substr(position_, keyword.size()) != keyword) return false;
    const auto next = position_ + keyword.size();
    if (!keyword.empty() && !delimiter(keyword.back()) &&
        next < input_.size() && !delimiter(input_[next])) return false;
    position_ = next;
    return true;
}

void Parser::expect(std::string_view keyword) {
    if (!take(keyword)) fail("expected " + std::string(keyword));
}

std::string_view Parser::word() {
    skip();
    const auto start = position_;
    while (position_ < input_.size() && !delimiter(input_[position_])) ++position_;
    if (start == position_) fail("expected token");
    if (position_ - start > 1024) fail("token too long");
    return input_.substr(start, position_ - start);
}

std::int64_t Parser::unsignedInteger() {
    const auto token = word();
    if (token.empty() || !digit(token.front())) fail("expected nonnegative integer");
    std::int64_t result = 0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), result);
    if (parsed.ec != std::errc() || parsed.ptr != token.data() + token.size())
        fail("invalid or out-of-range integer");
    return result;
}

void Parser::position(std::size_t position) {
    if (position > input_.size()) fail("offset outside input");
    position_ = position;
}

void Parser::streamEol() {
    if (position_ >= input_.size()) fail("missing stream line ending");
    if (input_[position_] == '\r') {
        ++position_;
        if (position_ < input_.size() && input_[position_] == '\n') ++position_;
    } else if (input_[position_] == '\n') ++position_;
    else fail("stream must be followed immediately by CR, LF, or CRLF");
}

void Parser::endStream() {
    if (position_ < input_.size() && (input_[position_] == '\r' || input_[position_] == '\n'))
        streamEol();
    constexpr std::string_view marker = "endstream";
    if (input_.substr(position_, marker.size()) != marker)
        fail("stream /Length does not lead to endstream");
    position_ += marker.size();
    if (position_ < input_.size() && !delimiter(input_[position_]))
        fail("invalid endstream token");
}

void Parser::revisionEnd(std::size_t xrefOffset) {
    expect("startxref");
    if (static_cast<std::uint64_t>(unsignedInteger()) != xrefOffset)
        fail("startxref does not match its xref table");
    while (position_ < input_.size() && whitespace(input_[position_])) ++position_;
    if (input_.substr(position_, 5) != "%%EOF") fail("missing revision %%EOF");
    position_ += 5;
    if (position_ < input_.size() && !whitespace(input_[position_]))
        fail("invalid revision %%EOF marker");
}

[[noreturn]] void Parser::fail(const std::string& message) const {
    throw Error(message + " at byte " + std::to_string(position_));
}

std::string Parser::name() {
    ++position_;
    std::string result;
    while (position_ < input_.size() && !delimiter(input_[position_])) {
        char c = input_[position_++];
        if (c == '#') {
            if (input_.size() - position_ < 2) fail("incomplete name escape");
            const int a = hexDigit(input_[position_++]);
            const int b = hexDigit(input_[position_++]);
            if (a < 0 || b < 0 || (a == 0 && b == 0)) fail("invalid name escape");
            c = static_cast<char>(a * 16 + b);
        }
        result += c;
        if (result.size() > 4096) fail("name too long");
    }
    return result;
}

std::string Parser::hexString() {
    ++position_;
    std::string result;
    int high = -1;
    while (position_ < input_.size()) {
        const char c = input_[position_++];
        if (c == '>') {
            if (high >= 0) result += static_cast<char>(high * 16);
            return result;
        }
        if (whitespace(c)) continue;
        const int d = hexDigit(c);
        if (d < 0) fail("invalid hexadecimal string");
        if (high < 0) high = d;
        else { result += static_cast<char>(high * 16 + d); high = -1; }
    }
    fail("unterminated hexadecimal string");
}

std::string Parser::literalString() {
    ++position_;
    std::size_t nesting = 1;
    std::string result;
    while (position_ < input_.size()) {
        char c = input_[position_++];
        if (c == '(') {
            if (++nesting > limits::depth) fail("string nesting limit exceeded");
            result += c;
        } else if (c == ')') {
            if (--nesting == 0) return result;
            result += c;
        } else if (c == '\\') {
            if (position_ == input_.size()) fail("unterminated string escape");
            c = input_[position_++];
            switch (c) {
            case 'n': result += '\n'; break;
            case 'r': result += '\r'; break;
            case 't': result += '\t'; break;
            case 'b': result += '\b'; break;
            case 'f': result += '\f'; break;
            case '\r':
                if (position_ < input_.size() && input_[position_] == '\n') ++position_;
                break;
            case '\n': break;
            default:
                if (c >= '0' && c <= '7') {
                    int code = c - '0';
                    for (int i = 1; i < 3 && position_ < input_.size() &&
                         input_[position_] >= '0' && input_[position_] <= '7'; ++i)
                        code = code * 8 + input_[position_++] - '0';
                    result += static_cast<char>(code & 255);
                } else result += c;
            }
        } else {
            if (c == '\r') {
                if (position_ < input_.size() && input_[position_] == '\n') ++position_;
                c = '\n';
            }
            result += c;
        }
    }
    fail("unterminated literal string");
}

PDFObjects Parser::value(std::size_t depth, bool references) {
    if (depth >= limits::depth) fail("object nesting limit exceeded");
    if (++nodes_ > limits::nodes) fail("parsed object limit exceeded");
    skip();
    if (position_ == input_.size()) fail("unexpected end of input");
    PDFObjects result;
    const char c = input_[position_];
    if (c == '/') {
        result.type = PDFObjects::Type::Name;
        result.text = name();
    } else if (c == '(') {
        result.type = PDFObjects::Type::String;
        result.text = literalString();
    } else if (take("<<")) {
        result.type = PDFObjects::Type::Dictionary;
        while (!take(">>")) {
            if (peek() != '/') fail("dictionary key must be a name");
            const auto key = name();
            auto item = value(depth + 1, references);
            if (!result.dictionary.emplace(key, std::move(item)).second)
                fail("duplicate dictionary key /" + key);
        }
    } else if (c == '<') {
        result.type = PDFObjects::Type::String;
        result.text = hexString();
    } else if (take("[")) {
        result.type = PDFObjects::Type::Array;
        while (!take("]")) result.array.push_back(value(depth + 1, references));
    } else if (take("true")) {
        result.type = PDFObjects::Type::Boolean;
        result.boolean = true;
    } else if (take("false")) {
        result.type = PDFObjects::Type::Boolean;
    } else if (take("null")) {
        result.type = PDFObjects::Type::Null;
    } else if (digit(c) || c == '+' || c == '-' || c == '.') {
        const auto token = word();
        std::size_t i = (token.front() == '+' || token.front() == '-') ? 1 : 0;
        bool dot = false, anyDigit = false;
        for (; i < token.size(); ++i) {
            if (digit(token[i])) anyDigit = true;
            else if (token[i] == '.' && !dot) dot = true;
            else fail("invalid PDF number");
        }
        if (!anyDigit) fail("invalid PDF number");
        if (dot) {
            const std::string copy(token);
            char* finish = nullptr;
            result.real = std::strtod(copy.c_str(), &finish);
            if (finish != copy.c_str() + copy.size() || !std::isfinite(result.real))
                fail("out-of-range real number");
            result.type = PDFObjects::Type::Real;
        } else {
            auto integerToken = token;
            if (integerToken.front() == '+') integerToken.remove_prefix(1);
            const auto parsed = std::from_chars(integerToken.data(),
                integerToken.data() + integerToken.size(), result.integer);
            if (parsed.ec != std::errc() || parsed.ptr != integerToken.data() + integerToken.size())
                fail("out-of-range integer");
            result.type = PDFObjects::Type::Integer;
            const auto saved = position_;
            if (references && result.integer >= 0 && digit(peek())) {
                const auto second = word();
                std::int64_t generation = 0;
                const auto parsedGeneration = std::from_chars(second.data(),
                    second.data() + second.size(), generation);
                const bool isInteger = parsedGeneration.ec == std::errc() &&
                    parsedGeneration.ptr == second.data() + second.size();
                if (isInteger && take("R")) {
                    if (result.integer == 0 || result.integer >= static_cast<std::int64_t>(limits::objects) ||
                        generation > 65535) fail("invalid object reference");
                    result.type = PDFObjects::Type::Reference;
                    result.reference = {result.integer, generation};
                } else position_ = saved;
            }
        }
    } else fail("unexpected object token");
    const auto weight = result.text.size() / 32;
    if (weight > limits::nodes - nodes_) fail("parsed string memory limit exceeded");
    nodes_ += weight;
    return result;
}

void appendBounded(std::string& target, std::string_view value, std::size_t maximum) {
    if (target.size() > maximum || value.size() > maximum - target.size())
        throw Error("output or combined content limit exceeded");
    target.append(value);
}

std::string metadataString(std::string_view bytes) {
    if (bytes.size() < 2) return std::string(bytes);
    const auto first = static_cast<unsigned char>(bytes[0]);
    const auto second = static_cast<unsigned char>(bytes[1]);
    const bool big = first == 0xfe && second == 0xff;
    const bool little = first == 0xff && second == 0xfe;
    if (!big && !little) return std::string(bytes);
    if (bytes.size() % 2 != 0) throw Error("odd-length UTF-16 metadata string");
    const auto unit = [&](std::size_t i) {
        const auto a = static_cast<unsigned char>(bytes[i]);
        const auto b = static_cast<unsigned char>(bytes[i + 1]);
        return static_cast<std::uint32_t>(big ? a * 256 + b : b * 256 + a);
    };
    std::string result;
    for (std::size_t i = 2; i < bytes.size(); i += 2) {
        auto code = unit(i);
        if (code >= 0xd800 && code <= 0xdbff) {
            if (i + 3 >= bytes.size()) throw Error("incomplete UTF-16 surrogate");
            const auto low = unit(i + 2);
            if (low < 0xdc00 || low > 0xdfff) throw Error("invalid UTF-16 surrogate");
            code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
            i += 2;
        } else if (code >= 0xdc00 && code <= 0xdfff) throw Error("invalid UTF-16 surrogate");
        utf8(result, code);
    }
    return result;
}

std::string StreamToString(std::string_view stream, std::size_t& nodes) {
    Parser parser(stream, nodes);
    std::vector<PDFObjects> operands;
    std::string result;
    bool inText = false;
    const auto requireCount = [&](std::size_t count) {
        if (operands.size() != count) parser.fail("wrong text operator operand count");
    };
    const auto requireNumbers = [&](std::size_t count) {
        requireCount(count);
        for (const auto& operand : operands)
            if (!operand.isNumber()) parser.fail("text positioning operand must be a number");
    };
    const auto show = [&](const PDFObjects& item) {
        if (item.type != PDFObjects::Type::String) parser.fail("text operand must be a string");
        appendBounded(result, item.text);
    };
    while (!parser.end()) {
        const char c = parser.peek();
        if (digit(c) || c == '+' || c == '-' || c == '.' || c == '/' ||
            c == '(' || c == '<' || c == '[') {
            operands.push_back(parser.value(0, false));
            if (operands.size() > 1024) parser.fail("content operand limit exceeded");
            continue;
        }
        if (parser.take("null") || parser.take("true") || parser.take("false")) {
            if (++nodes > limits::nodes) parser.fail("parsed object limit exceeded");
            operands.emplace_back();
            if (operands.size() > 1024) parser.fail("content operand limit exceeded");
            continue;
        }
        const auto op = parser.word();
        if (op == "BI" || op == "ID" || op == "EI") parser.fail("inline images are unsupported");
        if (op == "Do") parser.fail("XObject content is unsupported (including Form text)");
        if (op == "BT") {
            requireCount(0);
            if (inText) parser.fail("nested BT");
            inText = true;
        } else if (op == "ET") {
            requireCount(0);
            if (!inText) parser.fail("ET outside text object");
            inText = false;
        } else if (op == "Tj" || op == "TJ" || op == "'" || op == "\"" ||
                   op == "Td" || op == "TD" || op == "Tm" || op == "T*") {
            if (!inText) parser.fail("text operator outside BT/ET");
            if (op == "Tj") {
                requireCount(1); show(operands[0]);
            } else if (op == "TJ") {
                requireCount(1);
                if (operands[0].type != PDFObjects::Type::Array) parser.fail("TJ requires an array");
                for (const auto& item : operands[0].array) {
                    if (item.type == PDFObjects::Type::String) show(item);
                    else if (!item.isNumber()) parser.fail("TJ array must contain strings or numbers");
                }
            } else if (op == "'" || op == "\"") {
                requireCount(op == "'" ? 1 : 3);
                if (op == "\"" && (!operands[0].isNumber() || !operands[1].isNumber()))
                    parser.fail("quote spacing operands must be numbers");
                appendBounded(result, "\n");
                show(operands.back());
            } else if (op == "T*") {
                requireCount(0); appendBounded(result, "\n");
            } else {
                requireNumbers(op == "Tm" ? 6 : 2);
                if (op != "Tm") appendBounded(result, "\n");
            }
        }
        operands.clear();
    }
    if (inText) parser.fail("unterminated BT/ET text object");
    if (!operands.empty()) parser.fail("dangling content operands");
    return result;
}

}
