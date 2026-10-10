#pragma once

#include "PDFObjects.h"

#include <set>

namespace pdf {

class PDFFile {
public:
    explicit PDFFile(const std::string& filename);
    PDFFile(const PDFFile&) = delete;
    PDFFile& operator=(const PDFFile&) = delete;
    PDFFile(PDFFile&&) = delete;
    PDFFile& operator=(PDFFile&&) = delete;
    std::string ProcessPDF();

private:
    struct XRef {
        std::size_t offset;
        std::int64_t generation;
        bool inUse;
    };
    std::string data_;
    std::size_t nodes_ = 0;
    std::size_t decodedBytes_ = 0;
    std::size_t visits_ = 0;
    std::size_t pageNumber_ = 0;
    std::map<std::int64_t, XRef> crossRefs_;
    std::map<std::int64_t, PDFObjects> registeredObjects_;
    std::map<const PDFObjects*, std::string> decodedStreams_;
    std::set<std::int64_t> loading_;
    std::set<const PDFObjects*> pageVisited_;
    std::set<const PDFObjects*> outlineVisited_;
    PDFObjects trailer_;

    void ReadHeader();
    void ReadTrailer();
    std::int64_t ReadCrossReference(std::size_t offset, bool latest);
    const PDFObjects& LoadObject(PDFObjectRef reference);
    const PDFObjects& ResolveObject(const PDFObjects& object);
    void ValidateObject(const PDFObjects& object);
    void ValidateStream(const PDFObjects& object);
    const std::string& DecodeStream(const PDFObjects& object);
    void ProcessInfoBlock(std::string& result);
    void ProcessPages(const PDFObjects& pages, std::string& result, std::size_t depth);
    void ProcessOutlineLeaf(const PDFObjects& outline, std::string& result, std::size_t depth);
    std::string InfoString(const PDFObjects& value);
    std::int64_t Integer(const PDFObjects& value, const std::string& context);
    void Visit(std::size_t depth);
};

}
