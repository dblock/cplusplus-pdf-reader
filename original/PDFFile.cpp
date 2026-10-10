#include "PDFFile.h"

//#define _PDF_DEBUG_ON
#ifdef _PDF_DEBUG_ON
#define _PDF_DEBUG(x) cout << "[" << __LINE__ << "] " << x
#else
#define _PDF_DEBUG(x) 
#endif

#include "PDFObjects.h"

extern PDFObjects EmptyPDFObjects;

/*
	Canonical PDF File Represenation:
	<PDFFile> ::=
		<header>
		<body>
		<cross-ref table>
		<trailer>

	PDF 1.0:	7bit ASCII chars, binary data encoded in ASCII, ASCIIHex and ASCII85
				are supported, no line may be over 255 chars
				CR: <0D> LF: <0A>

	<Updated PDF file> ::=
		<PDF file>
		{<update>}*

	<update>::=
		<body>
		<cross-ref section>
		<trailer>		
	*/

PDFFile::PDFFile() : m_RegisteredObjects(EmptyPDFObjects) {

}

PDFFile::~PDFFile() {

}

PDFFile::PDFFile(const CString& FileName) : m_RegisteredObjects(EmptyPDFObjects) {
	_PDF_DEBUG("PDFFile::PDFFile() - " << FileName << endl);
	m_FileName = FileName;
	m_LastError = PDF_NOERROR;
	LoadFile();
}

void PDFFile::LoadFile(void){
	_PDF_DEBUG("PDFFile::LoadFile() - loading " << m_FileName << endl);
	ifstream PDFStream(m_FileName.asString(), ios::in | ios::binary, filebuf::openprot);
	if (!PDFStream.bad()) {
		ReadHeader(PDFStream);
		if (!m_LastError) ReadTrailer(PDFStream);
		if (!m_LastError) ProcessPDF(PDFStream);
	} else m_LastError = PDF_ERROR_OPEN;
	
}

void PDFFile::ProcessInfoBlock(PDFObjects * Trailer, ifstream& Stream){
	_PDF_DEBUG("Resolving /Info :: ");
	PDFInfo = ResolveObject(Trailer->DictionaryGet("Info"), Stream);
	cout << "Author: " << PDFInfo.DictionaryGetValueString("Author") << endl;
	cout << "Creation Date: " << PDFInfo.DictionaryGetValueDate("CreationDate") << endl;
	cout << "Modification Date: " << PDFInfo.DictionaryGetValueDate("ModDate") << endl;
	cout << "Creator: " << PDFInfo.DictionaryGetValueString("Creator") << endl;
	cout << "Producer: " << PDFInfo.DictionaryGetValueString("Producer") << endl;
	cout << "Title: " << PDFInfo.DictionaryGetValueString("Title") << endl;
	cout << "Subject: " << PDFInfo.DictionaryGetValueString("Subject") << endl;
	cout << "Keywords: " << PDFInfo.DictionaryGetValueString("Keywords") << endl;
}

void PDFFile::ProcessCatalogBlock(PDFObjects * Trailer, ifstream& Stream){
	_PDF_DEBUG("Resolving /Root :: ");
	PDFCatalog = ResolveObject(Trailer->DictionaryGet("Root"), Stream);	
	/*
		documnent threads
		*/
	ProcessPages(&PDFCatalog.DictionaryGet("Pages"), Stream);
	ProcessOutlines(&PDFCatalog.DictionaryGet("Outlines"), Stream);	
}

void PDFFile::ProcessOutlineLeaf(PDFObjects& PDFOutlineCurrent, ifstream& Stream, int Level){
	while (PDFOutlineCurrent.GetPDFType() != o_Undefined) {
		for (int i=0;i<Level;i++) cout << "  ";
		cout << PDFOutlineCurrent.DictionaryGetValueString("Title") << endl;		
		PDFObjects Child = ResolveObject(PDFOutlineCurrent.DictionaryGet("First"), Stream);
		if (Child.GetPDFType() != o_Undefined)
			ProcessOutlineLeaf(Child, Stream, Level+1);
		PDFOutlineCurrent = ResolveObject(PDFOutlineCurrent.DictionaryGet("Next"), Stream);
	}
}

void PDFFile::ProcessOutlines(PDFObjects * Outlines, ifstream& Stream){
	PDFObjects PDFOutlineDirect = ResolveObject(* Outlines, Stream);	
	PDFObjects PDFOutlineCurrent = ResolveObject(PDFOutlineDirect.DictionaryGet("First"), Stream);	
	ProcessOutlineLeaf(PDFOutlineCurrent, Stream, 0);
}


void PDFFile::ProcessPageLeaf(PDFObjects& PageObject, ifstream& Stream, int Level){
	if (PageObject.DictionaryGet("Type").asString().isSame("Page")){		
		PDFObjects ContentsRef = PageObject.DictionaryGet("Contents");
		if (ContentsRef.GetPDFType() != o_Undefined){
			PDFObjects Contents = ResolveObject(ContentsRef, Stream);
			if (Contents.GetPDFType() != o_Undefined){
				//cout << Contents.DictionaryGetValueString("Stream").StrLength() << endl;
				cout << Contents.DictionaryGet("Stream").StreamToString();
			}			
		}	

		/*
		PDFObjects ResourcesRef = PageObject.DictionaryGet("Resources");
		if (ResourcesRef.GetPDFType() != o_Undefined){
			PDFObjects Resources = ResolveObject(ResourcesRef, Stream);
			if (Resources.GetPDFType() != o_Undefined){
				
			}			
		}
		*/

	} else {
		PDFObjects Kids = PageObject.DictionaryGet("Kids");
		if (Kids.GetPDFType() != o_Undefined) {
			int KidsCount = Kids.DictionaryGetValueObjectsVectorCount();						
			for (int k=0;k<KidsCount;k++){
				PDFObjects KidPage = ResolveObject(Kids.DictionaryGetValueObjectsVector(k), Stream);
				ProcessPageLeaf(KidPage, Stream, Level+1);						
			}			
			cout << endl;
		}		
		
	}
	
}


void PDFFile::ProcessPages(PDFObjects * Pages, ifstream& Stream){
	PDFObjects PDFPageParent = ResolveObject(* Pages, Stream);		
	ProcessPageLeaf(PDFPageParent, Stream, 0);
}

void PDFFile::ProcessEncryptBlock(PDFObjects * Trailer, ifstream& Stream){
	_PDF_DEBUG("Resolving /Encrypt :: ");
	PDFEncryptDico = ResolveObject(Trailer->DictionaryGet("Encrypt"), Stream);	
	/*
		Encrypt Dictionary:
		Filter - Name of Security Handler (Standard is the std PDF)
		R (Revision) - Number (Stardard is 2)
		O (Owner) - String, data describing password to gain full access
		U (User) - String, data describing password to open the file
		P (Permissions) - Integer, collection of tags describing permissions granted
			bits: 1-2 (reserved, 0) 3 (printing) 4 (changing other than notes) 
			5 (copying) 6 (changing text notes) 7-32 (reserved, 1)
		*/
	/*
		compute the encryption key, assume empty key as we don't access protected documents
		*/
	return;
	/*
	static int PadPassword[] = {
		0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41, 0x64, 0x00, 0x4E, 
		0x56, 0xFF, 0xFA, 0x01, 0x08, 0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68, 
		0x3E, 0x80, 0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A };
	
	unsigned char password[33];
	int iLength = 0;

	for (int i=1;i<(32 - iLength);i++)
		password[iLength+i-1] = PadPassword[i];
	password[32] = 0;

	// calculate password's MD5 	
	unsigned char * md5_password = MD5.MD5Calculate(password);
	unsigned char * md5_owner = MD5.MD5Calculate((unsigned char *) PDFEncryptDico.DictionaryGetValueString("O").asString());
	delete md5_password;
	delete md5_owner;
	*/
}

void PDFFile::ProcessPDF(ifstream& Stream){
	PDFObjects * Trailer = m_RegisteredObjects.SearchRef("/Trailer");
	if (Trailer){
		_PDF_DEBUG("PDFFile::ProcessPDF() - trailer ok & " << CrossRefs.Count() << " cross-refs." << endl);		
		ProcessEncryptBlock(Trailer, Stream);
		ProcessInfoBlock(Trailer, Stream);
		ProcessCatalogBlock(Trailer, Stream);
	} else m_LastError = PDF_ERROR_INVALID_FORMAT;
}


/*
	<header> ::= <PDF version>
	ex: %PDF-1.0
	*/
void PDFFile::ReadHeader(ifstream& Stream) {
	_PDF_DEBUG("PDFFile::ReadHeader()" << endl);
	m_Header = ReadLine(Stream);
	_PDF_DEBUG("PDFFile::ReadHeader() - " << m_Header << endl);
	if ((!m_Header.StrLength()) || (m_Header[0] != '%')) {		
		m_LastError = PDF_ERROR_INVALID_FORMAT;
		_PDF_DEBUG("PDFFile::ReadHeader() - invalid file format (%)" << endl);
	} else if (!m_Header.Copy(1, 4).isSame("PDF-")) {		
		_PDF_DEBUG("PDFFile::ReadHeader() - invalid file format (PDF-)" << endl);
		m_LastError = PDF_ERROR_INVALID_FORMAT;
	} else {
		m_Version = m_Header.Copy(5, m_Header.StrLength()).ValFloat() * 10;
		if ((m_Version != 10)&&(m_Version != 11)&&(m_Version != 12)) {
			m_LastError = PDF_ERROR_UNSUPPORTED_VERSION;
			_PDF_DEBUG("PDFFile::ReadHeader() - unsupported version " << m_Version << endl);
		} else {	
			m_Version /= 10;
			_PDF_DEBUG("PDFFile::ReadHeader() - Version: " << m_Version << endl);
		}
	}
}

/*
	read PDF <body> section
	sequence of inderect objects
	comments start with a % at any position of a line
	*/
void PDFFile::ReadBody(ifstream& Stream) {
	
}

/*
	read PDF <crossref> section
	allows random access to objects in the PDF file

	<cross-reference-section> ::= 
		xref
		<cross-refenrece-subsection>+

	<cross-reference-subsection> ::=
		<object number of first entry in subsection>
		<number of entries in subsection>
		<cross-reference-entry>+

	exactly 20 chars long incl. eol marker

	<cross-ref-entry> ::=
		<in-use-entry> |
		<free entry>

	<in-use-entry> ::= <byte offset><space><generation number><space>n
		byte offset: #bytes from the beginning of file, ten-digit number, padded with leading zeros
		generation number: five digits, padded with zeros
	
	<free-etry> ::= <object number of next free object>
				    <generation number> f

	max generation number: 65635, once reached, it is not reused
	free objects form a linked list
	*/

int PDFFile::ReadCrossReferenceSubsection(ifstream& Stream){
	_PDF_DEBUG("PDFFile::ReadCrossReferenceSubsection()" << endl);	
	int m_ObjectNumber = ReadNumber(Stream);	
	if (m_ObjectNumber < 0) return 0;
	_PDF_DEBUG("PDFFile::ReadCrossReference() - First ObjectNumber = " << m_ObjectNumber << endl);
	int m_ObjectCount = ReadNumber(Stream);
	_PDF_DEBUG("PDFFile::ReadCrossReference() - Object Count = " << m_ObjectCount << endl);			
	for (int i=m_ObjectNumber;i<(m_ObjectNumber+m_ObjectCount);i++){				
		CString m_Entry = ReadFixed(Stream, 20).StrTrim32();						
		CVector<CString> m_AnyEntry = m_Entry.Tokenizer(' ');
		if (m_AnyEntry.Count() != 3) {
			m_LastError = PDF_ERROR_INVALID_FORMAT;			
			return 0;
		}
		//if (m_AnyEntry[2][0] == 'n') {					
		PDFObjectRef Reference(i, m_AnyEntry[0].Val(), m_AnyEntry[1].Val());
		CrossRefs += Reference;
		/*
		} else if (m_AnyEntry[2][0] == 'f') {
			
			// ignore free entries
		}		
		cout << m_AnyEntry[0].Val() << "/" << m_AnyEntry[1].Val() << endl;
		*/
	}	
	return 1;
}

/*
	returns offset of previous cross-ref or -1
	*/
int PDFFile::ReadCrossReference(ifstream& Stream){
	_PDF_DEBUG("PDFFile::ReadCrossReference()" << endl);
	CString m_XRef = ReadLine(Stream);
	_PDF_DEBUG("PDFFile::ReadCrossReference() - xref = " << m_XRef << endl);
	if (!m_XRef.isSame("xref")) {
		m_LastError = PDF_ERROR_INVALID_FORMAT;
		return -1;
	}
	if (ReadCrossReferenceSubsection(Stream))
		return ReadTrailerSection(Stream);
	else return -1;
}

/*
	purpose: quickly find the cross-reference section
	PDF file should be read from the end

	<trailer> ::=
		trailer
		<<
		<trailer key-value pair>+
		>>
		startxref
		<cross-ref-table start address>
		%%EOF

	trailer attributes:
		Size (integer): required, total number of entries in the files's cross-ref table
						including the original table and all updates
		Prev (integer): present only if file has more than one cross-ref section
						byte offset from the beginning of file to the location of the
						previous cross-ref section
		Root (dictionary): (required, must be indirect)
						catalog object of the document
		Info (dictionary): (optional, must be indirect)
						info dictionary for the document
		ID (array)		optional, an array of two strings, creation and modification ID
		Encrypt (dictionary) : (required if document is encrypted)
						info used to decrypt a document
	*/
	/*
		RETURNS previous section offset or -1
		*/
int PDFFile::ReadTrailerSection(ifstream& Stream){
	_PDF_DEBUG("PDFFile::ReadTrailerSection()" << endl);
	CString m_Trailer = ReadLine(Stream);
	_PDF_DEBUG("PDFFile::ReadTrailerSection() - trailer = " << m_Trailer << endl);
	
	CString AnyObjectDef("/Trailer");
	CString m_Current;
	do {		
		m_Current = ReadLine(Stream);
		AnyObjectDef += m_Current;
		AnyObjectDef += "\n";
	} while ((!Stream.eof())&&(m_Current != ">>"));
	
	PDFObjects TrailerObject(AnyObjectDef);
	TrailerObject.ReadAny();
	if (!m_RegisteredObjects.SearchRef("/Trailer")) m_RegisteredObjects.Add("/Trailer", TrailerObject);
	return TrailerObject.DictionaryGetValueInt("Prev");	
}

/* object registration in the glob htable */
int PDFFile::RegisterObject(const PDFObjects& Candidate){
	/*
	PDFObjects * OldVersion = m_RegisteredObjects.SearchRef(Candidate.GetName());
	if (OldVersion) {
	
	}
	*/
	m_RegisteredObjects.Set(Candidate.GetName(), Candidate);	
	return 1;
}

/*
	trailer pending
	
		startxref
		<cross-ref-table start address>
		%%EOF
	*/
void PDFFile::ReadTrailer(ifstream& Stream) {
	_PDF_DEBUG("PDFFile::ReadTrailer()" << endl);
	if (!Seek(Stream, 0L, ios::end)) return;
	m_StreamSize = Stream.tellg();

	// %%EOF

	CString m_Eof = ReadLineBackward(Stream);	
	_PDF_DEBUG("PDFFile::ReadTrailer() - EOF (should be %%EOF) - " << m_Eof << endl);
	if (!m_Eof.isSame("%%EOF")){
		_PDF_DEBUG("PDFFile::ReadHeader() - invalid file format (PDF-)" << endl);
		m_LastError = PDF_ERROR_INVALID_FORMAT;
		return;
	} 
		
	// <cross-ref-table start address>
	int m_CrossRefSa = ReadLineBackward(Stream).Val();
	_PDF_DEBUG("PDFFile::ReadTrailer() - <cross-ref-table start address> = " << m_CrossRefSa << endl);	
	
	// startxref
	CString m_StartxRef = ReadLineBackward(Stream);
	_PDF_DEBUG("PDFFile::ReadTrailer() - startxref = " << m_StartxRef << endl);	
	if (!m_StartxRef.isSame("startxref")) {
		m_LastError = PDF_ERROR_INVALID_FORMAT;
	}

	// read the xref section
	while (m_CrossRefSa != -1) {
		_PDF_DEBUG("PDFFile::ReadXREFSeek() - startxref = " << m_CrossRefSa << endl);			
		if (!Seek(Stream, m_CrossRefSa, ios::beg)) return;
		m_CrossRefSa = ReadCrossReference(Stream);
	}
	CrossRefs.QSort();
}

CString PDFFile::ReadLineBackward(ifstream& Stream) {
	CString Result;
	char buffer[STD_LINE_LEN];
	int iPos = 0; char c;	
	while (Stream.tellg() != 0){
		if (iPos == STD_LINE_LEN-1){
			buffer[iPos] = 0;
			Result += buffer;
			iPos = 0;
		}		
		Stream.seekg(-2L, ios::cur); 
		Stream.get(c);		
		if (c == 13) {
			buffer[iPos] = 0;			
			break;
		} else if (c != 10) {
			buffer[iPos] = c;
			iPos++;			
		}
	}
	if (iPos){
		buffer[iPos] = 0;
		Result += buffer;
	}
	CString FinalResult(Result);
	for (int i=0;i<FinalResult.StrLength();i++) FinalResult[i]=Result[Result.StrLength()-i-1];
	return FinalResult;
}

CString PDFFile::ReadLine(ifstream& Stream) {
	CString Result;
	char buffer[STD_LINE_LEN];
	int iPos = 0; char c;
	while (!Stream.eof()){		
		if (iPos == STD_LINE_LEN-1){
			buffer[iPos] = 0;
			Result.StrAppend(buffer, iPos);
			iPos = 0;
		}
		Stream.get(c);
		if (c == 13) {
			buffer[iPos] = 0;			
			break;
		} else if (c != 10) {
			buffer[iPos] = c;
			iPos++;			
		}
	}
	if (iPos){
		buffer[iPos] = 0;
		Result.StrAppend(buffer, iPos);
	}
	return Result;
}

int PDFFile::ReadNumber(ifstream& Stream) {	
	CString Result;
	char buffer[STD_LINE_LEN];
	int iPos = 0; char c;	
	while (!Stream.eof()){		
		if (iPos == STD_LINE_LEN-1){
			buffer[iPos] = 0;
			Result.StrAppend(buffer, iPos);
			iPos = 0;
		}
		Stream.get(c);		
		if ((c == 13)||(c == ' ')) {
			buffer[iPos] = 0;			
			break;
		} else if (isdigit(c)) {
			buffer[iPos] = c;
			iPos++;			
		}
	}
	if (iPos){
		buffer[iPos] = 0;
		Result.StrAppend(buffer, iPos);
	}
	if (Result.StrTrim32().StrLength()) return Result.Val();
	else return -1;
}

CString PDFFile::ReadFixed(ifstream& Stream, int Length){
	
	/*
	char * buffer = new char[Length+1];
	Stream.read(buffer, Length);	
	buffer[Length] = 0;
	CString Result(buffer);
	delete buffer;
	return Result;	
	*/

	int i = 0;
	char * buffer = new char[Length+1];
	while ((!Stream.eof())&&(i < Length)) {
		Stream.get(buffer[i]);
		//if ((buffer[i] > 0)&&(buffer[i] < ' ')) {
		if (buffer[i] == 10) {
			if (i) break;
		} else i++;
	}
	buffer[i] = 0;
	CString Result(buffer);
	delete buffer;
	return Result;
}

/*
	PDF object resolver out of an indirect reference, using cross-ref table
	*/
PDFObjects PDFFile::ResolveObject(const PDFObjects& IndirectRef, ifstream& Stream){
	// <object numer> <generation number>		
	CVector<int> IndirectVector = IndirectRef.asIntVector();
	if (IndirectVector.Count() < 2) {
		_PDF_DEBUG("OBJECT UNRESOLVED" << endl);
	} else for (int k=IndirectVector[0]; k<CrossRefs.Count(); k++){
		if ((CrossRefs[k].GetObjectNumber() == IndirectVector[0])&&
			(CrossRefs[k].GetGenerationNumber() == IndirectVector[1])) {
			_PDF_DEBUG("RESOLVED at offset " << CrossRefs[k].GetByteOffset() << endl);
			
			if (!Seek(Stream, CrossRefs[k].GetByteOffset(), ios::beg)) break;
			
			int ObjectNumber = ReadNumber(Stream);
			// check object number and generation value
			if (ObjectNumber != CrossRefs[k].GetObjectNumber()) {
				m_LastError = PDF_ERROR_INVALID_FORMAT;
				_PDF_DEBUG("Object Number " << ObjectNumber << " does not match " << CrossRefs[k].GetObjectNumber() << endl);
			} else if (ReadNumber(Stream) != CrossRefs[k].GetGenerationNumber()) {
				//_PDF_DEBUG("Object GenNumber does not match" << endl);
			} else {				
				CString FullObject = ReadFullObject(Stream);
				if (FullObject.StrLength()){
					PDFObjects GoodResult(FullObject);
					GoodResult.ReadAny();		
					return GoodResult;
				} else m_LastError = PDF_ERROR_INVALID_FORMAT;
			}
			break;
		} else if ((CrossRefs[k].GetObjectNumber() > IndirectVector[0])){
			_PDF_DEBUG("OBJECT NUMBER " << IndirectVector[0] << " UNRESOLVED" << endl);
			break;
		}
	}
	PDFObjects Result;
	return Result;
}

#define sEndObj 6

CString PDFFile::ReadFullObject(ifstream& Stream){
	unsigned char obj[3];	
	Stream.read(obj, 3);
	if (memcmp(obj, "obj", 3)) return "";

	CString Result;
	char buffer[STD_LINE_LEN];
	int iPos = 0; char c;	
	while (!Stream.eof()){		
		if (iPos == STD_LINE_LEN-1){
			buffer[iPos] = 0;
			Result.StrAppend(buffer, iPos);
			iPos = 0;
		}
		Stream.get(c);		
		buffer[iPos] = c;
		if ((iPos >= sEndObj) && (!memcmp(buffer+iPos-sEndObj, "endobj", sEndObj))) {
			iPos-=sEndObj;
			break;
		}
		iPos++;			
	}
	if (iPos){
		buffer[iPos] = 0;
		Result.StrAppend(buffer, iPos);
	}
	
	return Result;

		/*
	CString Result;
	char c;	
	while (!(Stream.eof())){
		Stream.get(c);		
		Result.StrAppend(&c, 1);
		if (Result.Copy(Result.StrLength() - strlen("endobj"), Result.StrLength()).isSame("endobj")) {
			Result.StrDelete(Result.StrLength() - strlen("endobj"), Result.StrLength());
			break;
		}			
		
	}	
	return Result;
	*/
}

int PDFFile::Seek(ifstream& Stream, streamoff Offset, ios::seek_dir Direction) {
	/*
	if (Stream.eof()){		
		Stream.close();
		Stream.open(m_FileName.asString(), ios::in, filebuf::openprot);
		if (Stream.bad()) {
			m_LastError = PDF_ERROR_OPEN;
			return 0;
		}
	}*/
	Stream.seekg(Offset, Direction);
	return 1;
}