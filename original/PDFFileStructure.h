/*
	PDF Support (c) Daniel Doubrovkine - 1998/99 All Rights Reserved
	================================================================
	PDF Document is formed of
		
		Objects 
		File Structure <------------------------ this class
		Document Structure
		Page Description

	defines how objects are stored in a PDF file, how they are accessed and how
	they are updated

	*/

#ifndef _PDF_FILE_STRUCTURE_H
#define _PDF_FILE_STRUCTURE_H

class PDFFileStructure {
public:
	PDFFileStructure();
	virtual ~PDFFileStructure();

};

#endif 
