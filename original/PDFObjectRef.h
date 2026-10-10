#ifndef PDF_OBJECT_REF_H
#define PDF_OBJECT_REF_H

class PDFObjectRef {
private:
	int m_ObjectNumber;
	int m_ByteOffset;
	int m_GenerationNumber;
public:
	PDFObjectRef(void);
	PDFObjectRef(int ObjectNumber, int ByteOffset, int GenerationNumber);
	PDFObjectRef(const PDFObjectRef&);
	virtual ~PDFObjectRef();
	PDFObjectRef& operator=(const PDFObjectRef&);
	int operator==(const PDFObjectRef&);
	int operator>(const PDFObjectRef&);
	int operator<(const PDFObjectRef&);
	int operator>=(const PDFObjectRef&);
	int operator<=(const PDFObjectRef&);
	int operator!=(const PDFObjectRef&);
	int GetObjectNumber(void) const;
	int GetByteOffset(void) const;
	int GetGenerationNumber(void) const;
};

#endif 
