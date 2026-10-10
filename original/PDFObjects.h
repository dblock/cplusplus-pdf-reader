/*
	PDF Support (c) Daniel Doubrovkine - 1998/99 All Rights Reserved
	================================================================
	PDF Document is formed of
		
		Objects <------------------------ this class
			
			  used as a name-value pair

		File Structure
		Document Structure
		Page Description

	set of basic object types to represent PDF objects

	*/
#ifndef _PDF_OBJECTS_H
#define _PDF_OBJECTS_H

#include <cstring/cstring.h>
#include <chashtable/chashtable.h>

typedef enum 
	_PDFObjectType {
		o_Undefined, o_Boolean, 
		o_Integer, o_Float, 
		o_MixedArray, o_IntArray, o_StringArray,
		o_Stream, o_Null, o_String,
		o_Indirect, o_IndirectRef,
		o_Dictionary } PDFObjectType;


class PDFObjects {
private:	
	PDFObjectType m_ObjectType;
	CString m_Name;
	CString m_Definition;
	void * m_Data;
	int curPos;

	CString ReadWord(const CString& Stream, int& curPos) const;
	char ReadHex(const CString& Stream, int& curPos, int Len) const; // read a hex number
	char ReadOctal(const CString& Stream, int& curPos, int Len) const; // read an octal number
	void SkipSpaces(const CString& Stream, int& curPos) const; // skip spaces
	void SkipCarriageSpaces(const CString& Stream, int& curPos) const; // skip spaces and carriage returns
	int ReadBoolean(const CString& Stream, int& curPos) const; // read a boolean object
	int ReadNull(const CString& Stream, int& curPos) const; // read a null object
	int ReadInteger(const CString& Stream, int& curPos) const; // read an integer object
	float ReadFloat(const CString& Stream, int& curPos) const; // read a float object
	double ReadNumber(const CString& Stream, int& curPos) const; // read a number
	CString ReadString(const CString& Stream, int& curPos) const;	// read a string
	CString ReadName(const CString& Stream, int& curPos) const; // read a name
	CVector<CString> ReadStringArray(const CString& Stream, int& curPos) const;
	CVector<int> ReadIntArray(const CString& Stream, int& curPos) const;
	CVector<int> ReadIndirectRef(const CString& Stream, int& curPos) const;
	CHashTable<PDFObjects> ReadDictionary(const CString& Stream, int& curPos) const; // read a dictionary
	CVector<PDFObjects> ReadMixedArray(const CString& Stream, int& curPos) const; // read a mixed array
	void DecodeDate(const CString& Date, struct tm * m_tm, int& m_gmt) const;
	void ReadStream(const CString& Stream, int& curPos, CHashTable<PDFObjects>& Dictionary) const;
	void ApplyFilter(CString Filter, CString& Data, CHashTable<PDFObjects>& Dictionary, PDFObjects& Params) const;	
public:
	PDFObjects();
	PDFObjects(const CString&, int Pos = 0);
	void ParseData(PDFObjectType m_ObjectType);
	PDFObjectType GetPDFType(void) const;
	CString GetName(void) const;
	virtual ~PDFObjects();

	PDFObjects(const PDFObjects&);
	PDFObjects& operator=(const PDFObjects&);
	int operator==(const PDFObjects&) const;
	int operator>(const PDFObjects&) const;
	int operator<(const PDFObjects&) const;
	int operator>=(const PDFObjects&) const;
	int operator<=(const PDFObjects&) const;
	int operator!=(const PDFObjects&) const;

	int asInteger(void) const;
	CString asString(void) const;
	CVector<CString> asStringVector(void) const;
	CVector<int> asIntVector(void) const;

	int DictionaryGetValueInt(const CString& Name) const;
	CString DictionaryGetValueString(const CString& Name) const;
	CVector<int> DictionaryGetValueIntVector(const CString& Name) const;
	CVector<CString> DictionaryGetValueStringVector(const CString& Name) const;
	PDFObjects DictionaryGet(const CString& Name) const;
	CString DictionaryGetValueDate(const CString& Name) const;
	int DictionaryGetValueObjectsVectorCount(void) const;
	PDFObjects DictionaryGetValueObjectsVector(int Index) const;	
	void ReadAny(void);
	/*	
		stream processing
		*/
	CString StreamToString(void);
};

#endif
