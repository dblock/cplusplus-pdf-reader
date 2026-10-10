/*
	PDF Support (c) Daniel Doubrovkine - 1998/99 All Rights Reserved
	================================================================
	PDF Document is formed of
		
		Objects 
		File Structure 
		Document Structure
		Page Description  <------------------------ this class

	part of a PDF Page object, has only limited interaction  with other parts
	of a PDF document

	*/

#ifndef _PDF_PAGE_DESCRIPTION_H
#define _PDF_PAGE_DESCRIPTION_H

class PDFPageDescription {
public:
	PDFPageDescription();
	virtual ~PDFPageDescription();

};

#endif
