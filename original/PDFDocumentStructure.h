/*
	PDF Support (c) Daniel Doubrovkine - 1998/99 All Rights Reserved
	================================================================
	PDF Document is formed of
		
		Objects 
		File Structure 
		Document Structure <------------------------ this class
		Page Description

	specifies how the basic object types are used to represent components of a PDF
	document: pages, annotations, hypertext links, fonts and more

	*/

#ifndef _PDF_DOCUMENT_STRUCTURE_H
#define _PDF_DOCUMENT_STRUCTURE_H

class PDFDocumentStructure {
public:
	PDFDocumentStructure();
	virtual ~PDFDocumentStructure();

};

#endif
