# C++ PDF Reader

A PDF-to-text converter I wrote in 1998 for a search engine called Alkaline. I needed self-contained PDF-to-text conversion for indexing, with no external runtime dependencies beyond the platform runtime.

## The 1998 Original

The source was recovered from a backup, with a verified date of **December 26, 1998**. Its headers say `PDF Support (c) Daniel Doubrovkine - 1998/99 All Rights Reserved`. It is preserved in [`original/`](original/) without changes to its contents, including the Visual C++ 6 project files, support libraries, and bundled zlib source. The former `PdfReader` subdirectory has been flattened into `original/`.

The goal was to read PDF files and convert them to text on the console. Give it a PDF filename, and it attempted to print document metadata, page text, and bookmark titles.

I was learning the PDF format by implementing a parser. Adobe had already published the format; the source refers explicitly to the PDF Reference, including a page number for stream filters. This was not an implementation of an undocumented file format.

### What It Implemented

- PDF 1.0, 1.1, and 1.2 headers.
- Classic cross-reference tables, trailers, and indirect object lookup, with an attempt to follow incremental updates through `/Prev`.
- Dictionaries, arrays, names, numbers, booleans, null values, and literal and hexadecimal strings.
- Document metadata such as author, title, subject, keywords, creator, producer, and dates.
- Page-tree traversal and basic extraction from PDF text operators.
- Flate-compressed content streams using the bundled zlib implementation.
- Outline traversal, printing nested bookmark titles.

The program bundled my string, vector, hash-table, date, and MD5 helpers, together with zlib, to keep the converter self-contained for Alkaline. The encryption code was a stub; including an MD5 implementation did not make encrypted PDFs readable.

### What It Did Not Do

It did not render pages or images, reconstruct layout, or reliably interpret font encodings. Other stream filters were described in comments but not implemented. The document-structure and page-description classes were largely scaffolding.

The surviving parser also has incomplete handling of stream lengths, filter arrays, line endings, and some text operators. It was an unfinished converter for simple PDFs of its period, not a general-purpose PDF implementation.

The files `original/pdf.tmp` and `original/pdf.new` appear to be compressed and decompressed stream artifacts, not complete sample PDF documents.

## Modern Port

A working C++17 port lives in `src/`, completely separate from `original/`. It preserves the converter's purpose and recognizable architecture, replacing obsolete compiler interfaces and unsafe storage with standard-library types. It is not a wrapper around an external PDF library, a graphical renderer, or a full production PDF framework.

The port uses system zlib rather than building the bundled 1990s copy. This is a deliberate modern dependency, not a claim that the original required a separate zlib installation.

### Build and Run

Requirements: a C++17 compiler, `make`, and system zlib headers/library. On macOS, the compiler and zlib are available with the Command Line Tools. No CMake is needed.

```sh
make
./pdf-reader fixtures/legacy-uncompressed.pdf
./pdf-reader fixtures/legacy-flate-crlf.pdf
./pdf-reader --help
```

The Makefile defaults to `clang++`; use `make CXX=g++` for another compiler. It compiles with `-std=c++17` and links `-lz`.

Output retains the original eight information labels, then adds explicit `Page N:` and `Outlines:` headings. Missing metadata is empty. Dates are printed as stored PDF date strings: the historical code reformatted them and substituted the current time for missing dates, making results nondeterministic. The port does not convert timezones.

Exit status is `0` on success, `1` for I/O, invalid data, resource limits, or unsupported features, and `2` for incorrect arguments. Diagnostics go to stderr. The complete report is validated before stdout is written, so parsing failures do not leave misleading partial output.

### Architecture

This is a clean modernization of the original parser, not a verbatim rebuild of the historical source:

| Historical responsibility | Modern implementation |
| --- | --- |
| `PDFObjects`, primitive parsing | Typed values, recursive parser, `std::string`, `std::vector`, `std::map` in `src/PDFObjects.*` |
| `PDFObjectRef` and cross-reference lookup | Object/generation references and newest-first classic xref indexing in `src/PDFFile.*` |
| `PDFFile` document processing | Object cache, trailer, metadata, page-tree and outline traversal in `src/PDFFile.*` |
| `StreamToString` | Content-token interpreter in `src/PDFObjects.cpp` |
| Bundled zlib / `ZlibEngine` | Bounded, checked system-zlib `inflate`, with RAII cleanup |
| Constructor-based CLI | Standard `int main`, explicit diagnostics and return codes |

There is no dependency on the historical containers, MD5/RC4, Win32, or bundled zlib. In-memory binary file storage and stream views replace line-oriented scans and searches for `endobj` inside binary data. The old bundle's `original/zlib/README.TXT` identifies it as mostly zlib 1.0; the wrapper credits Mark Nelson / Tiny Software.

### Supported Legacy Features and Fixes

- PDF headers 1.0, 1.1, and 1.2; classic xref tables with multiple subsections and incremental `/Prev` chains. New entries, including free entries, mask older entries; generation numbers and object offsets are checked.
- Dictionaries, mixed arrays, booleans, null, signed integer/decimal numbers, names (including `#xx` escapes), and indirect references.
- PDF whitespace and comments; LF, CRLF, and CR line endings.
- Nested literal strings, standard escapes, octal escapes, escaped line continuations, and whitespace-separated / odd-length hexadecimal strings.
- Exact binary stream `/Length`, including indirectly referenced lengths. Leading whitespace and NUL bytes are preserved; embedded `endobj` or `endstream` bytes are not treated as delimiters.
- Uncompressed and `FlateDecode` streams, filter arrays/chains, absent/null decode parameters, and explicit `/Predictor 1`.
- Page `/Contents` streams and arrays, including an indirectly referenced array. Arrays are combined in order, preserving text state across streams.
- `BT`, `ET`, `Td`, `TD`, `Tm`, `Tj`, `TJ`, `T*`, `'`, and `"` text operators. Quotes move to the next line **before** showing text. `TJ` shows every string in order and ignores numeric kerning adjustments; it does not insert arbitrary spaces between fragments. `Td`/`TD` insert newlines, as in the original; `Tm` does not attempt geometric layout.
- The original `/Info` fields and recursively indented outline titles; UTF-16BE/LE metadata and outline strings with a BOM are converted to UTF-8.

### Deliberate Limitations and Strict Rejection

- Encryption, xref streams, object streams, hybrid `/XRefStm` tables, catalog version overrides, and versions newer than PDF 1.2 are rejected.
- Only `FlateDecode` is supported. ASCIIHex/ASCII85, LZW, image filters such as DCT/JPEG, filter abbreviations, predictors other than 1, other decode parameters, and external-file streams are rejected.
- **Every active xref object and every stream is checked**, including streams not reached by the page walker. Consequently, a legacy document containing a JPEG-filtered image is rejected even if only its text was requested. Corrupt, truncated, or trailing-data Flate streams fail explicitly.
- Inline images (`BI`/`ID`/`EI`) and XObject invocation (`Do`, including Form XObjects) are rejected rather than silently skipping potentially textual content. Ordinary non-text graphics/state operators are ignored.
- This extracts decoded string **bytes**, not rendered glyphs. There is no font encoding, PDFDocEncoding conversion, `ToUnicode`/CMap processing, ligature handling, spacing reconstruction, column detection, or reading-order inference. Non-ASCII page text or custom font encodings may not be readable. Metadata without a UTF-16 BOM is also passed through as bytes.
- No image extraction, rendering, repair/scanning fallback, passwords, annotations, attachments, interactive forms, or accessibility/marked-content interpretation. Duplicate dictionary keys and inconsistent page counts are rejected rather than repaired.

### Resource Limits

Limits are explicit constants in `src/PDFObjects.h`:

- 64 MiB input file, 32 MiB raw or decoded stream, 32 MiB combined page content;
- 64 MiB cumulative decoded stream data (including intermediate filter outputs) and 64 MiB report output;
- one million parsing-budget units; objects/xref entries cost one unit and parsed string/name payload costs an additional unit per 32 bytes;
- object numbers below 100,000, 100,000 document-tree visits, 128 xref revisions;
- 64 levels for object/string nesting, reference loading/resolution, pages, and outlines; eight filters per stream and 1,024 pending content operands.

Cyclic/repeated page or outline nodes, cyclic reference resolution, invalid stream lengths, impossible offsets, and non-progressing decompression fail with an explicit diagnostic. These conservative limits bound this small reader's work; they do not make it a comprehensive validator for every PDF specification rule.

### Sample Documents

`fixtures/legacy-uncompressed.pdf` and `fixtures/legacy-flate-crlf.pdf` are synthetic sample documents; they contain no third-party document content. Both produce `fixtures/expected.txt`. The second uses CRLF, Flate, and an indirect stream length.

`make clean` removes the executable, never `original/` or the sample documents.

## License

My code is released under the [MIT License](LICENSE). Bundled third-party code, including the historical zlib source in `original/zlib/`, retains its own license notices.
