#include "PDFFile.h"

#include <array>
#include <fstream>
#include <zlib.h>

namespace pdf {
namespace {
void requireDictionary(const PDFObjects& object, const std::string& context) {
    if (object.type != PDFObjects::Type::Dictionary)
        throw Error(context + " must be a dictionary");
}

std::size_t offsetValue(std::int64_t value, std::size_t size, const std::string& context) {
    if (value < 0 || static_cast<std::uint64_t>(value) >= size)
        throw Error(context + " offset outside file");
    return static_cast<std::size_t>(value);
}
}

PDFFile::PDFFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) throw Error("cannot open " + filename);
    const auto size = file.tellg();
    if (size < 0 || static_cast<std::uint64_t>(size) > limits::fileBytes)
        throw Error("file size limit exceeded (64 MiB)");
    data_.resize(static_cast<std::size_t>(size));
    file.seekg(0);
    if (!file.read(data_.data(), static_cast<std::streamsize>(data_.size())))
        throw Error("cannot read " + filename);
    ReadHeader();
    ReadTrailer();
    if (trailer_.has("Encrypt")) throw Error("encrypted PDFs are unsupported");
    // Check all active objects, not just page contents, so unsupported streams cannot go unnoticed.
    for (const auto& entry : crossRefs_) {
        if (entry.second.inUse) LoadObject({entry.first, entry.second.generation});
    }
    for (const auto& entry : registeredObjects_) {
        ValidateObject(entry.second);
        const auto& type = ResolveObject(entry.second.get("Type"));
        if (type.isName("ObjStm") || type.isName("XRef"))
            throw Error("object streams and xref streams are unsupported");
        if (entry.second.hasStream) DecodeStream(entry.second);
    }
}

void PDFFile::ReadHeader() {
    if (data_.size() < 9 || data_.compare(0, 5, "%PDF-") != 0)
        throw Error("invalid PDF header");
    if (data_.compare(5, 2, "1.") != 0 || data_[7] < '0' || data_[7] > '2')
        throw Error("unsupported PDF version (only 1.0, 1.1, and 1.2)");
    if (data_[8] != '\r' && data_[8] != '\n') throw Error("invalid PDF header line ending");
}

void PDFFile::ReadTrailer() {
    std::size_t end = data_.size();
    while (end && Parser::whitespace(data_[end - 1])) --end;
    if (end < 5 || data_.compare(end - 5, 5, "%%EOF") != 0)
        throw Error("missing final %%EOF");
    const auto start = data_.rfind("startxref", end - 5);
    if (start == std::string::npos) throw Error("missing startxref");
    Parser tail(std::string_view(data_).substr(0, end - 5), nodes_, start);
    tail.expect("startxref");
    auto offset = tail.unsignedInteger();
    if (!tail.end()) tail.fail("unexpected data after startxref");
    std::set<std::size_t> seen;
    bool latest = true;
    while (offset >= 0) {
        const auto position = offsetValue(offset, data_.size(), "xref");
        if (!seen.insert(position).second) throw Error("cyclic /Prev xref chain");
        if (seen.size() > limits::revisions) throw Error("xref revision limit exceeded");
        const auto previous = ReadCrossReference(position, latest);
        if (previous >= 0 && static_cast<std::uint64_t>(previous) >= position)
            throw Error("/Prev must point to an earlier xref section");
        offset = previous;
        latest = false;
    }
    if (!trailer_.has("Root")) throw Error("trailer is missing /Root");
    if (trailer_.get("Root").type != PDFObjects::Type::Reference)
        throw Error("trailer /Root must be an indirect reference");
    const auto size = Integer(trailer_.get("Size"), "trailer /Size");
    if (size <= 0 || size > static_cast<std::int64_t>(limits::objects))
        throw Error("trailer /Size outside supported object limit");
    if (!crossRefs_.empty() && crossRefs_.rbegin()->first >= size)
        throw Error("xref entry exceeds trailer /Size");
}

std::int64_t PDFFile::ReadCrossReference(std::size_t offset, bool latest) {
    Parser parser(data_, nodes_, offset);
    if (!parser.take("xref"))
        throw Error("expected classic xref table; xref streams are unsupported");
    std::set<std::int64_t> sectionEntries;
    while (!parser.take("trailer")) {
        const auto first = parser.unsignedInteger();
        const auto count = parser.unsignedInteger();
        if (first >= static_cast<std::int64_t>(limits::objects) || count <= 0 ||
            count > static_cast<std::int64_t>(limits::objects) - first)
            parser.fail("xref subsection exceeds object limit");
        for (std::int64_t i = 0; i < count; ++i) {
            if (++nodes_ > limits::nodes) parser.fail("xref entry budget exceeded");
            const auto position = parser.unsignedInteger();
            const auto generation = parser.unsignedInteger();
            const auto state = parser.word();
            if (generation > 65535 || (state != "n" && state != "f"))
                parser.fail("invalid xref entry");
            if (first + i == 0 && state != "f") parser.fail("object zero must be free");
            if (!sectionEntries.insert(first + i).second) parser.fail("overlapping xref subsections");
            std::size_t byteOffset = 0;
            if (state == "n") byteOffset = offsetValue(position, data_.size(), "object");
            crossRefs_.emplace(first + i, XRef{byteOffset, generation, state == "n"});
        }
    }
    auto trailer = parser.value();
    requireDictionary(trailer, "trailer");
    parser.revisionEnd(offset);
    if (trailer.has("Encrypt")) throw Error("encrypted PDFs are unsupported");
    if (trailer.has("XRefStm")) throw Error("hybrid xref streams are unsupported");
    const auto& size = trailer.get("Size");
    if (size.type != PDFObjects::Type::Integer || size.integer <= 0 ||
        size.integer > static_cast<std::int64_t>(limits::objects))
        parser.fail("invalid trailer /Size");
    for (const auto objectNumber : sectionEntries)
        if (objectNumber >= size.integer) parser.fail("xref entry exceeds section /Size");
    if (latest) trailer_ = trailer;
    else {
        for (const auto& item : trailer.dictionary) {
            if (item.first != "Prev") trailer_.dictionary.emplace(item.first, item.second);
        }
    }
    if (!trailer.has("Prev")) return -1;
    const auto& previous = trailer.get("Prev");
    if (previous.type != PDFObjects::Type::Integer || previous.integer < 0)
        parser.fail("invalid trailer /Prev");
    return previous.integer;
}

const PDFObjects& PDFFile::LoadObject(PDFObjectRef reference) {
    const auto found = crossRefs_.find(reference.number);
    if (found == crossRefs_.end() || !found->second.inUse ||
        found->second.generation != reference.generation)
        throw Error("unresolved reference " + std::to_string(reference.number) + " " +
                    std::to_string(reference.generation) + " R");
    const auto cached = registeredObjects_.find(reference.number);
    if (cached != registeredObjects_.end()) return cached->second;
    if (loading_.size() >= limits::depth || !loading_.insert(reference.number).second)
        throw Error("cyclic reference or object loading depth limit exceeded");
    Parser parser(data_, nodes_, found->second.offset);
    if (parser.unsignedInteger() != reference.number ||
        parser.unsignedInteger() != reference.generation)
        parser.fail("xref does not match object number/generation");
    parser.expect("obj");
    auto object = parser.value();
    if (object.type == PDFObjects::Type::Dictionary && parser.take("stream")) {
        parser.streamEol();
        const auto length = Integer(object.get("Length"), "stream /Length");
        if (length < 0 || static_cast<std::uint64_t>(length) > limits::streamBytes)
            throw Error("stream /Length outside supported limit (32 MiB)");
        const auto start = parser.position();
        if (static_cast<std::uint64_t>(length) > data_.size() - start)
            parser.fail("truncated stream");
        object.hasStream = true;
        object.stream = std::string_view(data_).substr(start, static_cast<std::size_t>(length));
        parser.position(start + static_cast<std::size_t>(length));
        parser.endStream();
    }
    parser.expect("endobj");
    loading_.erase(reference.number);
    return registeredObjects_.emplace(reference.number, std::move(object)).first->second;
}

const PDFObjects& PDFFile::ResolveObject(const PDFObjects& object) {
    const PDFObjects* current = &object;
    std::set<std::int64_t> seen;
    while (current->type == PDFObjects::Type::Reference) {
        if (seen.size() >= limits::depth || !seen.insert(current->reference.number).second)
            throw Error("cyclic reference or reference depth limit exceeded");
        current = &LoadObject(current->reference);
    }
    return *current;
}

std::int64_t PDFFile::Integer(const PDFObjects& value, const std::string& context) {
    const auto& resolved = ResolveObject(value);
    if (resolved.type != PDFObjects::Type::Integer) throw Error(context + " must be an integer");
    return resolved.integer;
}

void PDFFile::ValidateObject(const PDFObjects& object) {
    if (object.type == PDFObjects::Type::Reference) {
        const auto found = crossRefs_.find(object.reference.number);
        if (found == crossRefs_.end() || !found->second.inUse ||
            found->second.generation != object.reference.generation)
            throw Error("unresolved reference " + std::to_string(object.reference.number) + " " +
                        std::to_string(object.reference.generation) + " R");
    }
    for (const auto& item : object.array) ValidateObject(item);
    for (const auto& item : object.dictionary) ValidateObject(item.second);
}

void PDFFile::ValidateStream(const PDFObjects& object) {
    if (object.has("F") || object.has("FFilter") || object.has("FDecodeParms"))
        throw Error("external streams are unsupported");
    const auto& filter = ResolveObject(object.get("Filter"));
    std::vector<const PDFObjects*> filters;
    if (filter.type == PDFObjects::Type::Name) filters.push_back(&filter);
    else if (filter.type == PDFObjects::Type::Array) {
        for (const auto& entry : filter.array) filters.push_back(&ResolveObject(entry));
        if (filters.empty()) throw Error("empty /Filter array is unsupported");
    } else if (filter.type != PDFObjects::Type::Null) throw Error("invalid stream /Filter");
    if (filters.size() > 8) throw Error("stream filter chain limit exceeded");
    for (const auto* item : filters) {
        if (!item->isName("FlateDecode"))
            throw Error("unsupported stream filter /" + item->text + " (only FlateDecode)");
    }
    if (object.has("DecodeParams")) throw Error("invalid /DecodeParams; use /DecodeParms");
    const auto& params = ResolveObject(object.get("DecodeParms"));
    std::vector<const PDFObjects*> parameters;
    if (params.type == PDFObjects::Type::Array) {
        if (filter.type != PDFObjects::Type::Array || params.array.size() != filters.size())
            throw Error("/DecodeParms array must match /Filter array");
        for (const auto& entry : params.array) parameters.push_back(&ResolveObject(entry));
    } else if (params.type == PDFObjects::Type::Dictionary) {
        if (filters.size() != 1 || filter.type == PDFObjects::Type::Array)
            throw Error("/DecodeParms dictionary requires a single /Filter name");
        parameters.push_back(&params);
    } else if (params.type != PDFObjects::Type::Null) throw Error("invalid /DecodeParms");
    for (const auto* parameter : parameters) {
        if (parameter->type == PDFObjects::Type::Null) continue;
        requireDictionary(*parameter, "/DecodeParms");
        for (const auto& entry : parameter->dictionary) {
            if (entry.first == "Predictor") {
                if (Integer(entry.second, "/Predictor") != 1)
                    throw Error("stream predictors are unsupported (only Predictor 1)");
            } else throw Error("unsupported Flate parameter /" + entry.first);
        }
    }
}

const std::string& PDFFile::DecodeStream(const PDFObjects& object) {
    if (!object.hasStream) throw Error("page /Contents must reference a stream");
    const auto cached = decodedStreams_.find(&object);
    if (cached != decodedStreams_.end()) return cached->second;
    ValidateStream(object);
    auto data = std::string(object.stream);
    const auto& filter = ResolveObject(object.get("Filter"));
    const auto count = filter.type == PDFObjects::Type::Array ? filter.array.size() :
                       filter.type == PDFObjects::Type::Name ? 1u : 0u;
    if (count == 0) {
        if (data.size() > limits::totalDecodedBytes - decodedBytes_)
            throw Error("total decoded stream limit exceeded (64 MiB)");
        decodedBytes_ += data.size();
    }
    for (std::size_t i = 0; i < count; ++i) {
        z_stream stream{};
        if (inflateInit(&stream) != Z_OK) throw Error("cannot initialize zlib");
        struct Guard {
            z_stream& stream;
            ~Guard() { inflateEnd(&stream); }
        } guard{stream};
        stream.next_in = reinterpret_cast<Bytef*>(data.data());
        stream.avail_in = static_cast<uInt>(data.size());
        std::array<char, 16384> buffer{};
        std::string decoded;
        for (;;) {
            stream.next_out = reinterpret_cast<Bytef*>(buffer.data());
            stream.avail_out = static_cast<uInt>(buffer.size());
            const auto before = stream.avail_in;
            const auto status = inflate(&stream, Z_NO_FLUSH);
            const auto produced = buffer.size() - stream.avail_out;
            if (produced > limits::totalDecodedBytes - decodedBytes_)
                throw Error("total decompression limit exceeded (64 MiB)");
            decodedBytes_ += produced;
            appendBounded(decoded, std::string_view(buffer.data(), produced), limits::streamBytes);
            if (status == Z_STREAM_END) {
                if (stream.avail_in != 0) throw Error("trailing bytes in Flate stream");
                break;
            }
            if (status != Z_OK || (before == stream.avail_in && produced == 0))
                throw Error("invalid or truncated Flate stream");
        }
        data = std::move(decoded);
    }
    return decodedStreams_.emplace(&object, std::move(data)).first->second;
}

std::string PDFFile::InfoString(const PDFObjects& value) {
    const auto& item = ResolveObject(value);
    if (item.type == PDFObjects::Type::Null) return "";
    if (item.type != PDFObjects::Type::String) throw Error("metadata value must be a string");
    return metadataString(item.text);
}

void PDFFile::ProcessInfoBlock(std::string& result) {
    const auto& info = ResolveObject(trailer_.get("Info"));
    if (info.type != PDFObjects::Type::Null) requireDictionary(info, "/Info");
    const std::pair<const char*, const char*> fields[] = {
        {"Author", "Author"}, {"Creation Date", "CreationDate"}, {"Modification Date", "ModDate"},
        {"Creator", "Creator"}, {"Producer", "Producer"}, {"Title", "Title"},
        {"Subject", "Subject"}, {"Keywords", "Keywords"}
    };
    for (const auto& field : fields) {
        appendBounded(result, field.first);
        appendBounded(result, ": ");
        appendBounded(result, InfoString(info.get(field.second)));
        appendBounded(result, "\n");
    }
}

void PDFFile::Visit(std::size_t depth) {
    if (depth >= limits::depth) throw Error("document tree depth limit exceeded");
    if (++visits_ > limits::objects) throw Error("document tree visit limit exceeded");
}

void PDFFile::ProcessPages(const PDFObjects& pages, std::string& result, std::size_t depth) {
    Visit(depth);
    const auto& page = ResolveObject(pages);
    requireDictionary(page, "page tree node");
    if (!pageVisited_.insert(&page).second) throw Error("cyclic or repeated page tree node");
    if (ResolveObject(page.get("Type")).isName("Page")) {
        appendBounded(result, "\nPage " + std::to_string(++pageNumber_) + ":\n");
        std::string content;
        const auto& contents = ResolveObject(page.get("Contents"));
        if (contents.type == PDFObjects::Type::Array) {
            for (const auto& entry : contents.array) {
                appendBounded(content, DecodeStream(ResolveObject(entry)), limits::streamBytes);
                appendBounded(content, "\n", limits::streamBytes);
            }
        } else if (contents.type != PDFObjects::Type::Null)
            content = DecodeStream(contents);
        const auto text = StreamToString(content, nodes_);
        appendBounded(result, text);
        if (text.empty() || text.back() != '\n') appendBounded(result, "\n");
    } else if (ResolveObject(page.get("Type")).isName("Pages")) {
        const auto& kids = ResolveObject(page.get("Kids"));
        if (kids.type != PDFObjects::Type::Array) throw Error("/Pages /Kids must be an array");
        const auto count = Integer(page.get("Count"), "/Pages /Count");
        if (count < 0 || count > static_cast<std::int64_t>(limits::objects))
            throw Error("invalid /Pages /Count");
        const auto before = pageNumber_;
        for (const auto& kid : kids.array) ProcessPages(kid, result, depth + 1);
        if (pageNumber_ - before != static_cast<std::size_t>(count))
            throw Error("/Pages /Count does not match descendant pages");
    } else throw Error("page tree node must have /Type /Page or /Pages");
}

void PDFFile::ProcessOutlineLeaf(const PDFObjects& outline, std::string& result, std::size_t depth) {
    const PDFObjects* current = &ResolveObject(outline);
    while (current->type != PDFObjects::Type::Null) {
        Visit(depth);
        requireDictionary(*current, "outline item");
        if (!outlineVisited_.insert(current).second) throw Error("cyclic or repeated outline item");
        if (!current->has("Title")) throw Error("outline item is missing /Title");
        appendBounded(result, std::string(depth * 2, ' '));
        appendBounded(result, InfoString(current->get("Title")));
        appendBounded(result, "\n");
        if (current->has("First")) ProcessOutlineLeaf(current->get("First"), result, depth + 1);
        current = &ResolveObject(current->get("Next"));
    }
}

std::string PDFFile::ProcessPDF() {
    std::string result;
    ProcessInfoBlock(result);
    const auto& catalog = ResolveObject(trailer_.get("Root"));
    requireDictionary(catalog, "/Root");
    if (!ResolveObject(catalog.get("Type")).isName("Catalog"))
        throw Error("/Root must have /Type /Catalog");
    if (catalog.has("Version")) throw Error("catalog version overrides are unsupported");
    ProcessPages(catalog.get("Pages"), result, 0);
    const auto& outlines = ResolveObject(catalog.get("Outlines"));
    if (outlines.type != PDFObjects::Type::Null) {
        requireDictionary(outlines, "/Outlines");
        appendBounded(result, "\nOutlines:\n");
        ProcessOutlineLeaf(outlines.get("First"), result, 0);
    }
    return result;
}

}
