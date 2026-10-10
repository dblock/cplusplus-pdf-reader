/*
	PDF Support (c) Daniel Doubrovkine - 1998/99 All Rights Reserved
	================================================================
	*/

#ifndef _PDF_FILE_H
#define _PDF_FILE_H

#define PDF_NOERROR 0
#define PDF_ERROR_OPEN 1
#define PDF_ERROR_INVALID_FORMAT 2
#define PDF_ERROR_UNSUPPORTED_VERSION 3

#define STD_LINE_LEN 256

#include <cstring/cstring.h>
#include <cvector/cvector.h>
#include <iostream.h>

#include "PDFObjectRef.h"
#include "PDFObjects.h"
#include <chashtable/chashtable.h>
#include <cmd5/cmd5.h>
#include <zlib/zlibengn.h>

class PDFFile {
private:	
	CMD5 MD5;	
	int m_LastError;
	int m_StreamSize;
	CString m_FileName;
	/*
		pure PDF structure
		*/
	CString m_Header;
	float m_Version;
	CVector<PDFObjectRef> CrossRefs;
	CHashTable<PDFObjects> m_RegisteredObjects;
	PDFObjects PDFInfo;
	PDFObjects PDFCatalog;
	PDFObjects PDFEncryptDico;
	/*
		PDF helper functions
		*/	
	void ReadHeader(ifstream& Stream);
	void ReadBody(ifstream& Stream);
	int ReadCrossReference(ifstream& Stream);
	void ReadTrailer(ifstream& Stream);
	int ReadCrossReferenceSubsection(ifstream& Stream);
	int ReadTrailerSection(ifstream& Stream);
	void LoadFile(void);	
	int RegisterObject(const PDFObjects& Cadidate);
	PDFObjects ResolveObject(const PDFObjects& IndirectRef, ifstream& PDFStream);
	void ProcessPDF(ifstream& PDFStream);
	int Seek(ifstream&, streamoff, ios::seek_dir);
	CString ReadFullObject(ifstream& Stream);
	/*	
		PDF Speicific Dico Processing
		*/
	void ProcessInfoBlock(PDFObjects * Trailer, ifstream& Stream);
	void ProcessCatalogBlock(PDFObjects * Trailer, ifstream& Stream);
	void ProcessEncryptBlock(PDFObjects * Trailer, ifstream& Stream);	
	void ProcessOutlines(PDFObjects * Outlines, ifstream& Steream);
	void ProcessOutlineLeaf(PDFObjects& PDFOutlineCurrent, ifstream& Stream, int Level);
	void ProcessPages(PDFObjects * Pages, ifstream& Stream);
	void ProcessPageLeaf(PDFObjects& PageObject, ifstream& Stream, int Level);	
public:
	PDFFile(const CString&); // creates a PDF file reader
	PDFFile();
	virtual ~PDFFile();
	static CString ReadLine(ifstream& Stream);
	static CString ReadLineBackward(ifstream& Stream);
	static int ReadNumber(ifstream& Stream);
	static CString ReadFixed(ifstream& Stream, int Length);
};

#endif 
