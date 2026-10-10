#include "PDFFile.h"

#include <iostream>

int main(int argc, char** argv) {
    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        std::cout << "Usage: pdf-reader FILE.pdf\n"
                     "Print legacy PDF 1.0-1.2 metadata, page text, and outline titles.\n";
        return 0;
    }
    if (argc != 2) {
        std::cerr << "Usage: pdf-reader FILE.pdf\n";
        return 2;
    }
    try {
        pdf::PDFFile document(argv[1]);
        const auto report = document.ProcessPDF();
        std::cout.write(report.data(), static_cast<std::streamsize>(report.size()));
        if (!std::cout) throw pdf::Error("cannot write output");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "pdf-reader: " << error.what() << '\n';
        return 1;
    }
}
