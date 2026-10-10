#include "PDFObjectRef.h"

PDFObjectRef::PDFObjectRef() {
	m_ObjectNumber = -1;
	m_ByteOffset = -1;
	m_GenerationNumber = -1;
}

PDFObjectRef::~PDFObjectRef(){

}

PDFObjectRef::PDFObjectRef(int ObjectNumber, int ByteOffset, int GenerationNumber) {
	m_ObjectNumber = ObjectNumber;
	m_ByteOffset = ByteOffset;
	m_GenerationNumber = GenerationNumber;
}

PDFObjectRef::PDFObjectRef(const PDFObjectRef& Other){
	(* this) = Other;
}

PDFObjectRef& PDFObjectRef::operator=(const PDFObjectRef& Other){
	m_ObjectNumber = Other.m_ObjectNumber;
	m_ByteOffset = Other.m_ByteOffset;
	m_GenerationNumber = Other.m_GenerationNumber;
	return (* this);
}

int PDFObjectRef::operator==(const PDFObjectRef& Other){
	return ((m_ObjectNumber == Other.m_ObjectNumber) &&
			(m_ByteOffset == Other.m_ByteOffset) &&
			(m_GenerationNumber == Other.m_GenerationNumber));
}

int PDFObjectRef::operator>(const PDFObjectRef& Other){
	if (m_ObjectNumber > Other.m_ObjectNumber) return 1;
	else if (m_ObjectNumber == Other.m_ObjectNumber) 
		return (m_GenerationNumber > Other.m_GenerationNumber);
	else return 0;
}

int PDFObjectRef::operator<(const PDFObjectRef& Other){
	if (m_ObjectNumber < Other.m_ObjectNumber) return 1;
	else if (m_ObjectNumber == Other.m_ObjectNumber) 
		return (m_GenerationNumber < Other.m_GenerationNumber);
	else return 0;
}

int PDFObjectRef::operator>=(const PDFObjectRef& Other){
	if (m_ObjectNumber > Other.m_ObjectNumber) return 1;
	else if (m_ObjectNumber == Other.m_ObjectNumber) 
		return (m_GenerationNumber >= Other.m_GenerationNumber);
	else return 0;
}

int PDFObjectRef::operator<=(const PDFObjectRef& Other){
	if (m_ObjectNumber < Other.m_ObjectNumber) return 1;
	else if (m_ObjectNumber == Other.m_ObjectNumber) 
		return (m_GenerationNumber <= Other.m_GenerationNumber);
	else return 0;}

int PDFObjectRef::operator!=(const PDFObjectRef& Other){
	if (m_ObjectNumber != Other.m_ObjectNumber) return 1;
	else if (m_ObjectNumber != Other.m_ObjectNumber) return 1;
	else return 0;
}

int PDFObjectRef::GetObjectNumber(void) const {
	return m_ObjectNumber;
}

int PDFObjectRef::GetByteOffset(void) const {
	return m_ByteOffset;
}

int PDFObjectRef::GetGenerationNumber(void) const {
	return m_GenerationNumber;
}

#include <cvector/cvector.cpp>
template class CVector<PDFObjectRef>;