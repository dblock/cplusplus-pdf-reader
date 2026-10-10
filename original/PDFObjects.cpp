#include "PDFObjects.h"
#include <math.h>
#include <time.h>
#include <cdate/cdate.h>

//#define _PDF_DEBUG_ON
#ifdef _PDF_DEBUG_ON
#define _PDF_DEBUG(x) cout << "[" << __LINE__ << "] " << x
#else
#define _PDF_DEBUG(x) 
#endif

#include <zlib/zlibengn.h>

PDFObjects EmptyPDFObjects;
ZlibEngine ZLIB;

PDFObjects::PDFObjects() {	
	m_Data = NULL;
	m_ObjectType = o_Undefined;
	curPos = 0;
}

PDFObjects::~PDFObjects() {	
	if (m_Data) delete m_Data;	
}

PDFObjects::PDFObjects(const PDFObjects& Other) {	
	(* this) = Other;		
}

PDFObjects& PDFObjects::operator=(const PDFObjects& Other){	
	m_ObjectType = Other.m_ObjectType;
	if (Other.m_Data){
		switch(Other.m_ObjectType){
			case o_Null:	
			case o_Integer:	
			case o_Boolean:		m_Data = new int; (* (int *) m_Data) = (* (int *) Other.m_Data); break;
			case o_Float:		m_Data = new float;	(* (float *) m_Data) = (* (float *) Other.m_Data); break;
			case o_MixedArray:	m_Data = new CVector<PDFObjects>; (* (CVector<PDFObjects> *) m_Data) = (* (CVector<PDFObjects> *) Other.m_Data); break;
			case o_IndirectRef:
			case o_IntArray:	m_Data = new CVector<int>; (* (CVector<int> *) m_Data) = (* (CVector<int> *) Other.m_Data);	break;		
			case o_StringArray:	m_Data = new CVector<CString>; (* (CVector<CString> *) m_Data) = (* (CVector<CString> *) Other.m_Data);	break;
			case o_Stream:
			case o_String:		m_Data = new CString; (* (CString *) m_Data) = (* (CString *) Other.m_Data); break;					
			case o_Indirect:	break;
			case o_Dictionary:	m_Data = new CHashTable<PDFObjects>(EmptyPDFObjects); (* (CHashTable<PDFObjects> *) m_Data) = (* (CHashTable<PDFObjects> *) Other.m_Data); break;
			default: m_Data = NULL;
		}
	} else m_Data = NULL;
	curPos = Other.curPos;
	m_Definition = Other.m_Definition;
	m_Name = Other.m_Name;
	return (* this);
}

int PDFObjects::operator==(const PDFObjects& Other) const {
	return (Other.m_Name.isSame(m_Name));
}

int PDFObjects::operator>(const PDFObjects& Other) const {
	return (m_Name > Other.m_Name);
}

int PDFObjects::operator<(const PDFObjects& Other) const {
	return (m_Name < Other.m_Name);
}

int PDFObjects::operator>=(const PDFObjects& Other) const {
	return (m_Name > Other.m_Name) || (m_Name.isSame(Other.m_Name));
}

int PDFObjects::operator<=(const PDFObjects& Other) const {
	return (m_Name < Other.m_Name) || (m_Name.isSame(Other.m_Name));
}

int PDFObjects::operator!=(const PDFObjects& Other) const {
	return (m_Name != Other.m_Name);
}


PDFObjects::PDFObjects(const CString& Stream, int Pos){
	m_Data = NULL;
	m_ObjectType = o_Undefined;
	curPos = Pos;
	m_Definition = Stream;	
	m_Name = ReadName(m_Definition, curPos);
	if (m_Name.StrLength()) _PDF_DEBUG("PDFObjects::ReadName() - " << m_Name << endl);	
}

void PDFObjects::ParseData(PDFObjectType ObjectType){
	m_ObjectType = ObjectType;
	switch(m_ObjectType) {
	case o_Boolean:			m_Data = new int; (* (int *) m_Data) = ReadBoolean(m_Definition, curPos); break;
	case o_Integer:			m_Data = new int; (* (int *) m_Data) = ReadInteger(m_Definition, curPos); break;
	case o_Float:			m_Data = new float;	(* (float *) m_Data) = ReadFloat(m_Definition, curPos);	break;
	case o_MixedArray:		m_Data = new CVector<PDFObjects>; (* (CVector<PDFObjects> *) m_Data) = ReadMixedArray(m_Definition, curPos); break;		
	case o_IntArray:		m_Data = new CVector<int>;	(* (CVector<int> *) m_Data) = ReadIntArray(m_Definition, curPos); break;		
	case o_StringArray:		m_Data = new CVector<CString>; (* (CVector<CString> *) m_Data) = ReadStringArray(m_Definition, curPos);	break;
	case o_Stream:			cout << "ERROR! Streams are part of dictionary objects only" << endl; break;
	case o_String:			m_Data = new CString; (* (CString *) m_Data) = ReadString(m_Definition, curPos); break;
	case o_Null:			m_Data = new int; (* (int *) m_Data) = ReadNull(m_Definition, curPos); break;
	case o_IndirectRef:		m_Data = new CVector<int>; (* (CVector<int> *) m_Data) = ReadIndirectRef(m_Definition, curPos);	break;
	case o_Indirect:		break;
	case o_Dictionary:		m_Data = new CHashTable<PDFObjects>(EmptyPDFObjects); (* (CHashTable<PDFObjects> *) m_Data) = ReadDictionary(m_Definition, curPos); break;
	}
}

PDFObjectType PDFObjects::GetPDFType(void) const {
	return m_ObjectType;
}

CString PDFObjects::GetName(void) const {
	return m_Name;
}

void PDFObjects::SkipSpaces(const CString& Stream, int& curPos) const {
	while ((curPos < Stream.StrLength()) && (Stream[curPos] == ' ')) curPos++;
}

char PDFObjects::ReadOctal(const CString& Stream, int& curPos, int Len) const {
	int prevPos = curPos;
	char Result = 0;
	while ((curPos < Stream.StrLength()) && (curPos - prevPos < Len)){
		char aDigit = Stream[curPos];		
		if ((aDigit >= '0')&&(aDigit <= '7')){
			Result *= 8;
			Result += aDigit - '0';
			curPos++;
		} else break;
	}
	return Result;
}

char PDFObjects::ReadHex(const CString& Stream, int& curPos, int Len) const {
	int prevPos = curPos;
	char Result = 0;
	while ((curPos < Stream.StrLength()) && (curPos - prevPos < Len)){
		char aDigit = toupper(Stream[curPos]);
		if (((aDigit >= '0')&&(aDigit <= '9')) || ((aDigit >= 'A') && (aDigit <= 'F'))) {
			Result *= 16;
			Result += (aDigit >= 'A' ? ((aDigit & 0xdf) - 'A')+10 : (aDigit - '0'));
		} else break;
	}
	return Result;
}

/* read a null object - null (0) -1*/
int PDFObjects::ReadNull(const CString& Stream, int& curPos) const {
	SkipSpaces(Stream, curPos);
	if (Stream.Copy(curPos, strlen("null")).isSame("null")) {
		curPos += strlen("null");
		return 0;
	} else return -1;
}

/*	read a boolean object - "true" or "false" */
int PDFObjects::ReadBoolean(const CString& Stream, int& curPos) const {	
	SkipSpaces(Stream, curPos);	
	if (Stream.Copy(curPos, strlen("true")).isSame("true")) {
		curPos += strlen("true");
		return 1;
	} else if (Stream.Copy(curPos, strlen("false")).isSame("false")) {
		curPos += strlen("false");
		return 0;
	} else return -1;
}

CString PDFObjects::ReadWord(const CString& Stream, int& curPos) const {	
	SkipSpaces(Stream, curPos);	
	int prevPos = curPos;
	while ((curPos < Stream.StrLength())&&(isalpha(Stream[curPos]))) curPos++;
	return Stream.Copy(prevPos, curPos-prevPos);
}


/*	read an integer objects - signed or unsigned integer value */
int PDFObjects::ReadInteger(const CString& Stream, int& curPos) const {
	SkipSpaces(Stream, curPos);
	int Sign = 1;
	while (curPos < Stream.StrLength()){		
		if (Stream[curPos] == '+') {
			Sign = 1;
			curPos++;
		} else if (Stream[curPos] == '-') {
			Sign = -1;			
			curPos++;			
		} else break;
	}	
	int Result = 0;
	while ((curPos < Stream.StrLength())&&(isdigit(Stream[curPos]))){
		Result *= 10;
		Result += Stream[curPos] - '0';		
		curPos++;
	}	
	return Result * Sign;
}

/*	read a float object - in decimal format only */
float PDFObjects::ReadFloat(const CString& Stream, int& curPos) const {
	int Major = ReadInteger(Stream, curPos);	
	if (Stream[curPos] == '.'){
		curPos++;
		if (isdigit(Stream[curPos])) {
			int prevPos = curPos;
			int Minor = ReadInteger(Stream, curPos);
			float Result = (float) Major;
			if (Result < 0) Result -= (float)(((double)Minor)/pow(10, curPos-prevPos));
			else Result += (float)(((double)Minor)/pow(10, curPos-prevPos));
			return Result;
		}
	}
	return (float) Major;
}

/* read a numer - an integer or a float object */
double PDFObjects::ReadNumber(const CString& Stream, int& curPos) const {
	return ReadFloat(Stream, curPos);
} 

/*
	read a string, a sequence of chars delimited by parenthesis
	ex: ( This is a string )
		( This is \
	    a string split)

    escape sequences:
		\n: linefeed
		\r: carriage return
		\t: horizontal tab
		\b: backspace
		\f: formfeed
		\\: backslash
		\(: left parenthesis
		\): right parenthesis
		\ddd : character code (octal) (1, 2 or three digits)
		hex representation by pairs, an odd number of digits assumes a trailing 0
			<901fa3> : three char string 90 1f a3
			<901fa> : three char string 90 1f a0
		unicode: 
			always start by a unicode byte order marker
			<FE FF> (thorn ydieresis)
			high order byte appears first
		escape sequence:
			U+001B + 2 ASCII codes for the lang identifiers + [2 ASCII country code] + U+001B
	*/	  

CString PDFObjects::ReadString(const CString& Stream, int& curPos) const {	
	SkipSpaces(Stream, curPos);	
	CString Result;		
	if (Stream[curPos] == '<'){		
		curPos++;
		while (curPos < Stream.StrLength()) {			
			if (Stream[curPos] == '>') {
				curPos++;
				break;
			}
			char fDigit = toupper(Stream[curPos]);			
			if ((isdigit(fDigit)) || ((fDigit >= 'A') && (fDigit <= 'F'))) {				
				curPos++;
				char sDigit = toupper(Stream[curPos]);
				if ((isdigit(sDigit)) || ((sDigit >= 'A') && (sDigit <= 'F'))) {				
					curPos++;
				} else sDigit = '0';			    
				char digit;
				digit = (fDigit >= 'A' ? ((fDigit & 0xdf) - 'A')+10 : (fDigit - '0'));
				digit *= 16;
				digit += (sDigit >= 'A' ? ((sDigit & 0xdf) - 'A')+10 : (sDigit - '0'));
				Result += (char) digit;
			} else break;
		}
	} else if (Stream[curPos] == '(') {
		//cout << "entering:[" << Stream.Copy(curPos, 10) << "]" << endl;
		curPos++;
		while (curPos < Stream.StrLength()) {			
			if (Stream[curPos] == ')') {
				curPos++;
				break;
			}
			switch(Stream[curPos]){
				case '\\':
					curPos++;
					if ((Stream[curPos] < ' ')&&(Stream[curPos] > 0)) {
						while ((Stream[curPos] < ' ')&&(Stream[curPos] > 0)) curPos++;
						break;
					} else {
						switch(Stream[curPos]){
						case 'n': Result += (char) 13; curPos++; break;
						case 'r': Result += (char) 10; curPos++; break;
						case 't': Result += (char) '\t'; curPos++; break;
						case 'b': Result.StrDelete(Result.StrLength()-1, 1); curPos++; break;
						case 'f': Result += (char) '\f'; curPos++; break;
						case '\\': Result += (char) '\\'; curPos++; break;
						case '(': Result += (char) '('; curPos++; break;
						case ')': Result += (char) ')'; curPos++; break;
						default:
							if ((Stream[curPos] >= '0')&&(Stream[curPos] <= '7')) {
								Result += (char) ReadOctal(Stream, curPos, 3);
							}
						}
						curPos++;
					}
					break;
				default:
					Result += (char) Stream[curPos];
					curPos++;
					break;
			}
		}
	} 	
	//cout << "returning string:[" << Result << "]" << endl;
	return Result;
}


/*
	a sequence of characters
	beginning by a slash
	followed by a sequence of ASCII chars in the range ! (<21>) through ~ (<7E>)
	except %()<>[]{}/#
	ex: /Name1
		/A;Name_With-various***Characters?.
		/.notdef
	including hex codes except 00 starting by a #
	ex:	/A#42 = /AB
	--> maximum length 127, ex: /A#20B has 4 chars, not six
	*/
CString PDFObjects::ReadName(const CString& Stream, int& curPos) const {	
	SkipSpaces(Stream, curPos);		
	if ((curPos < Stream.StrLength()) && (Stream[curPos] == '/')) {				
		CString Result; //("/");
		curPos++;
		while (curPos < Stream.StrLength()){			
			if (Stream[curPos] == '#'){
				curPos++;
				Result += ReadHex(Stream, curPos, 2);
			} else if ((Stream[curPos] >= 0x21) && (Stream[curPos] <= 0x7E)) {
				if ((Stream[curPos] == '%')||
					(Stream[curPos] == '(')||
					(Stream[curPos] == ')')||
					(Stream[curPos] == '<')||
					(Stream[curPos] == '>')||
					(Stream[curPos] == '[')||
					(Stream[curPos] == ']')||
					(Stream[curPos] == '{')||
					(Stream[curPos] == '}')||
					(Stream[curPos] == '/')) break;
				else {					
					Result += Stream[curPos];					
					curPos++;
				}
			} else break;
		}		
		return Result;
	} else return "";	
}


/*	array of strings */
CVector<CString> PDFObjects::ReadStringArray(const CString& Stream, int& curPos) const {
	CVector<CString> Result;
	SkipSpaces(Stream, curPos);
	if (Stream[curPos] == '[') {
		curPos++;
		while (curPos < Stream.StrLength()){		
			if (Stream[curPos] == ']') {
				curPos++;
				break;
			} else {
				CString newString = ReadString(Stream, curPos);
				if (newString.StrLength()) {
					_PDF_DEBUG("PDFObjects::ReadStringArray() - " << newString << endl);
					Result+=newString;
				} else break;			
			}
		}		
	}
	return Result;
}

/* array of integers */
CVector<int> PDFObjects::ReadIntArray(const CString& Stream, int& curPos) const {
	CVector<int> Result;
	SkipSpaces(Stream, curPos);
	if (Stream[curPos] == '[') {
		curPos++;
		while (curPos < Stream.StrLength()){		
			if (Stream[curPos] == ']') {
				curPos++;
				break;
			} else {
				int newInt = ReadInteger(Stream, curPos);
				if (newInt != -1) {
					_PDF_DEBUG("PDFObjects::ReadIntArray() - " << newInt << endl);
					Result+=newInt;
				} else break;			
			}
		}		
	}
	return Result;
}


/* indirect reference 
	<inderect reference> ::=
		<object number>
		<generation number>
		R
		*/

CVector<int> PDFObjects::ReadIndirectRef(const CString& Stream, int& curPos) const {
	CVector<int> Result;
	Result += ReadInteger(Stream, curPos);
	Result += ReadInteger(Stream, curPos);
	_PDF_DEBUG("PDFObjects::ReadIndirectArray() - " << Result[0] << "/" << Result[1] << endl);
	SkipSpaces(Stream, curPos);	
	if ((curPos >= Stream.StrLength()) || (Stream[curPos] != 'R')) {
		_PDF_DEBUG("PDFObjects::ReadIndirectArray() - ERROR!" << endl);
	}	
	return Result;
}


int PDFObjects::asInteger(void) const {
	if (m_ObjectType != o_Integer) {
		return -1;
	} else return (* (int *) m_Data);
}

CString PDFObjects::asString(void) const {
	if ((m_ObjectType == o_String) || (m_ObjectType == o_Stream)) {
		return (* (CString *) m_Data);
	} else if (m_ObjectType == o_Float) {
		CString Result; Result += (* (float *) m_Data);
		return Result;
	} else if (m_ObjectType == o_Integer) {
		CString Result; Result += (* (int *) m_Data);
		return Result;
	} else return "";
}

CVector<CString> PDFObjects::asStringVector(void) const {
	if (m_ObjectType != o_StringArray) {
		CVector<CString> Result;
		return Result;
	} else return (* (CVector<CString> *) m_Data);
}

CVector<int> PDFObjects::asIntVector(void) const {
	if ((m_ObjectType != o_IntArray)&&(m_ObjectType != o_IndirectRef)) {
		CVector<int> Result;
		return Result;
	} else return (* (CVector<int> *) m_Data);
}

void PDFObjects::SkipCarriageSpaces(const CString& Stream, int& curPos) const {
	while ((curPos < Stream.StrLength()) && (Stream[curPos] <= ' ')) curPos++;
}

void PDFObjects::ReadAny(void) {
	// m_Definition + curPos		
	SkipCarriageSpaces(m_Definition, curPos);
	/*
		attempt to indetify the object type	
		*/
	if (curPos < m_Definition.StrLength()){
		_PDF_DEBUG("PDFObjects::ReadAny() - Switching "  << m_Definition[curPos] << endl);
		if (m_Definition[curPos] == '/') {
			// name
			_PDF_DEBUG("PDFObjects::ReadAny() - NAME" << endl);
			m_ObjectType = o_String;
			m_Data = new CString;
			(* (CString *) m_Data) = ReadName(m_Definition, curPos);			
			_PDF_DEBUG("PDFObjects::ReadAny() - NAME read " << (* (CString *) m_Data) << endl);
		} else if (m_Definition[curPos] == '('){
			// string
			_PDF_DEBUG("PDFObjects::ReadAny() - Defined as STRING" << endl);				
			m_ObjectType = o_String;
			m_Data = new CString;
			(* (CString *) m_Data) = ReadString(m_Definition, curPos);
			_PDF_DEBUG("PDFObjects::ReadAny() - STRING read " << (* (CString *) m_Data) << endl);
		} else if (m_Definition[curPos] == '<'){		
			if ((curPos+1 < m_Definition.StrLength())&&(m_Definition[curPos+1] == '<')){
				// dictionary		
				_PDF_DEBUG("PDFObjects::ReadAny() - Defined as DICTIONARY" << endl);				
				m_ObjectType = o_Dictionary;
				m_Data = new CHashTable<PDFObjects>(EmptyPDFObjects);								
				(* (CHashTable<PDFObjects> *) m_Data) = ReadDictionary(m_Definition, curPos);				
				_PDF_DEBUG("PDFObjects::ReadAny() - DICTIONARY read " << (* (CHashTable<PDFObjects> *) m_Data).Count() << " objects" << endl);				
			} else {
				// string				
				_PDF_DEBUG("PDFObjects::ReadAny() - Defined as STRING" << endl);
				m_ObjectType = o_String;
				m_Data = new CString;
				(* (CString *) m_Data) = ReadString(m_Definition, curPos);
				_PDF_DEBUG("PDFObjects::ReadAny() - STRING read " << (* (CString *) m_Data) << endl);
			}
		} else if (m_Definition[curPos] == '[') {
			// array
			_PDF_DEBUG("PDFObjects::ReadAny() - Defined as ARRAY" << endl);
			m_ObjectType = o_MixedArray;
			m_Data = new CVector<PDFObjects>;			
			(* (CVector<PDFObjects> *) m_Data) = ReadMixedArray(m_Definition, curPos);
			_PDF_DEBUG("PDFObjects::ReadAny() - ARRAY read " << (* (CVector<PDFObjects> *) m_Data).Count() << " objects " << endl);
		} else if ((isdigit(m_Definition[curPos])) || (m_Definition[curPos] == '+') || (m_Definition[curPos] == '-')){			
			_PDF_DEBUG("PDFObjects::ReadAny() - Defined as INT/FLOAT" << endl);				
			float FirstValue = ReadFloat(m_Definition, curPos);									
			SkipSpaces(m_Definition, curPos);
			int savePos = curPos;
			float SecondValue = -1;
			if (isdigit(m_Definition[curPos])) SecondValue = ReadFloat(m_Definition, curPos);				
			if (SecondValue >= 0) {
				SkipSpaces(m_Definition, curPos);
				switch(m_Definition[curPos]){
				case 'R':
					// reference
					_PDF_DEBUG("PDFObjects::ReadAny() - Defined as INDIRECT REFERENCE" << endl);
					m_ObjectType = o_IndirectRef;
					m_Data = new CVector<int>;
					(* (CVector<int> *) m_Data)+=(int)FirstValue;
					(* (CVector<int> *) m_Data)+=(int)SecondValue;
					_PDF_DEBUG("PDFObjects::ReadAny() - INDIRECT REFERENCE read " << FirstValue << "/" << SecondValue << endl);
					curPos++;
					break;
				default:
					curPos = savePos;
					if (FirstValue == (int) FirstValue){
						//integre
						_PDF_DEBUG("PDFObjects::ReadAny() - Defined as INTEGER" << endl);
						m_ObjectType = o_Integer;
						m_Data = new int;
						(* (int *) m_Data) = (int) FirstValue;
						_PDF_DEBUG("PDFObjects::ReadAny() - INTEGER read " << FirstValue << endl);
					} else {
						//float
						_PDF_DEBUG("PDFObjects::ReadAny() - Defined as FLOAT" << endl);
						m_ObjectType = o_Float;
						m_Data = new float;
						(* (float *) m_Data) = FirstValue;
						_PDF_DEBUG("PDFObjects::ReadAny() - FLOAT read " << FirstValue << endl);
					}									
				}
			} else {
				// float value
				_PDF_DEBUG("PDFObjects::ReadAny() - Defined as FLOAT (2)" << endl);				
				if (FirstValue == (int) FirstValue){
					//integre
					_PDF_DEBUG("PDFObjects::ReadAny() - Defined as INTEGER" << endl);
					m_ObjectType = o_Integer;
					m_Data = new int;
					(* (int *) m_Data) = (int) FirstValue;
					_PDF_DEBUG("PDFObjects::ReadAny() - INTEGER read " << FirstValue << endl);
				} else {
					//float
					_PDF_DEBUG("PDFObjects::ReadAny() - Defined as FLOAT" << endl);
					m_ObjectType = o_Float;
					m_Data = new float;
					(* (float *) m_Data) = FirstValue;
					_PDF_DEBUG("PDFObjects::ReadAny() - FLOAT read " << FirstValue << endl);
				}									
			}
		} else if (isalpha(m_Definition[curPos])){
			// null ?				
			int MaybeValue;
			if ((MaybeValue = ReadNull(m_Definition, curPos)) == 0) {
				m_ObjectType = o_Null;
				m_Data = new int;
				(* (int *) m_Data) = MaybeValue;
				_PDF_DEBUG("PDFObjects::ReadAny() - NULL read " << endl);					
			} else if ((MaybeValue = ReadBoolean(m_Definition, curPos)) != -1) {
				m_ObjectType = o_Boolean;
				m_Data = new int;
				(* (int *) m_Data) = MaybeValue;
				_PDF_DEBUG("PDFObjects::ReadAny() - BOOLEAN read " << MaybeValue << endl);					
			} else {
				cout << "dunno know what da is" << endl;
				exit(0);
			}			
		} else {
			cout << "dunno know what da is" << endl;
			exit(0);
		}
	}	
}

/*
	key-value pairs, a key must be a name
	<< sequence of key-value pairs >>
	ex: << /Type /Example /Key2 12 /Key3 (a string) >>
	can be on many levels:
	ex:
		<<	/Type /AnExample
			/MyInfo
			<<
				/Item1 0.4
				/Item2 true
			>>
		>>
	convention: 
	/Type key specifies the type of object
	/Subtype key specifies a specialization of an object
	*/
CHashTable<PDFObjects> PDFObjects::ReadDictionary(const CString& Stream, int& curPos) const {	
	CHashTable<PDFObjects> Result(EmptyPDFObjects);	
	/*
		read the << boundary
		*/
	SkipCarriageSpaces(Stream, curPos);
	if (Stream.Copy(curPos, 2) == "<<") {
		curPos+=2;
		SkipCarriageSpaces(Stream, curPos);		
		while ((curPos < Stream.StrLength()) && (Stream[curPos] == '/')) {
			PDFObjects AnyObject(Stream, curPos);			
			AnyObject.ReadAny();
			Result.Add(AnyObject.GetName(), AnyObject);			
			curPos = AnyObject.curPos;
			SkipCarriageSpaces(Stream, curPos);							
		}
		if (Stream.Copy(curPos, 2) == ">>") curPos+=2;
		SkipCarriageSpaces(Stream, curPos);
		/*
			decode the stream attached
			*/
		ReadStream(Stream, curPos, Result);		
	}	
	return Result;
}

CVector<PDFObjects> PDFObjects::ReadMixedArray(const CString& Stream, int& curPos) const {
	CVector<PDFObjects> Result;	
	if ((curPos < Stream.StrLength()) && (Stream[curPos] == '[')) {
		curPos++;
		SkipCarriageSpaces(Stream, curPos);
		while ((curPos < Stream.StrLength()) && (Stream[curPos] != ']')){
			PDFObjects AnyObject(Stream, curPos);
			AnyObject.ReadAny();
			curPos = AnyObject.curPos;
			Result += AnyObject;
			SkipCarriageSpaces(Stream, curPos);			
		}
		curPos++;
	}
	return Result;
}

int PDFObjects::DictionaryGetValueObjectsVectorCount(void) const {
	return (* (CVector<PDFObjects> *) m_Data).Count();
}

/* assuming data is a dictionary, get an objects ref */
PDFObjects PDFObjects::DictionaryGetValueObjectsVector(int Index) const {
	if ((Index >= 0)&&(Index < (* (CVector<PDFObjects> *) m_Data).Count())){
		return (* (CVector<PDFObjects> *) m_Data)[Index];
	} else {
		PDFObjects Dummy;
		return Dummy;
	}
}

/*	assuming data is a dictionary, get an integer value */
int PDFObjects::DictionaryGetValueInt(const CString& Name) const {
	if (m_ObjectType != o_Dictionary) return -1;	
	PDFObjects * ARef = (* (CHashTable<PDFObjects> *) m_Data).SearchRef(Name);
	if (ARef) {
		return ARef->asInteger();
	} else return -1;
}

/*	assuming data is a dictionary, get a string value */
CString PDFObjects::DictionaryGetValueString(const CString& Name) const {
	if (m_ObjectType != o_Dictionary) return "";
	PDFObjects * ARef = (* (CHashTable<PDFObjects> *) m_Data).SearchRef(Name);
	if (ARef) {
		return ARef->asString();
	} else return "";
}

CVector<int> PDFObjects::DictionaryGetValueIntVector(const CString& Name) const {
	if (m_ObjectType != o_Dictionary) {
		CVector<int> Result;
		return Result;	
	}
	PDFObjects * ARef = (* (CHashTable<PDFObjects> *) m_Data).SearchRef(Name);
	if (ARef) {
		return ARef->asIntVector();
	} else return -1;
}

CVector<CString> PDFObjects::DictionaryGetValueStringVector(const CString& Name) const {
	if (m_ObjectType != o_Dictionary) return -1;	
	PDFObjects * ARef = (* (CHashTable<PDFObjects> *) m_Data).SearchRef(Name);
	if (ARef) {
		return ARef->asStringVector();
	} else return -1;
}

PDFObjects PDFObjects::DictionaryGet(const CString& Name) const {	
	if (m_ObjectType != o_Dictionary) {		
		PDFObjects Result;
		return Result;
	} else return (* (CHashTable<PDFObjects> *) m_Data).Search(Name);	
}

CString PDFObjects::DictionaryGetValueDate(const CString& Name) const {
	CString m_Date = DictionaryGetValueString(Name);
	struct tm m_tm;
	int m_gmt = 0;
	DecodeDate(m_Date, &m_tm, m_gmt);
	CDate m_DateObject(&m_tm);
	CString m_DateResult = m_DateObject.Map("$DAYENG, $DAY $MONTHENG $YEAR $HOUR:$MIN:$SEC GMT");
	if (m_gmt != 0) {
		if (m_gmt > 0) m_DateResult += "+";
		m_DateResult += m_gmt;
	}
	return m_DateResult;
}

void PDFObjects::DecodeDate(const CString& Date, struct tm * m_tm, int& m_gmt) const {
	_PDF_DEBUG("PDFObjects::DecodeDate() - " << Date << endl);	
	if ((Date.StrLength() >= 16)&&(Date.Copy(0, 2).isSame("D:"))){		
		m_tm->tm_year = Date.Copy(2, 4).Val() - 1900;
		m_tm->tm_mon = (Date.Copy(6, 2).Val() - 1) % 12;
		m_tm->tm_mday = Date.Copy(8, 2).Val() % 31;
		m_tm->tm_hour = Date.Copy(10, 2).Val() % 24;
		m_tm->tm_min = Date.Copy(12, 2).Val() % 60;
		m_tm->tm_sec = Date.Copy(14, 2).Val() % 60;
		if (Date.StrLength() > 16) {
			m_gmt = Date.Copy(17, 2).Val();
			switch (Date[14]){
				case '+':
				break;
			case '-':
				m_gmt = -m_gmt;
				break;
			case 'Z':
				m_gmt = 0;
				break;
			}
		} else m_gmt = 0;		
	} else {
		time_t l_time; time(&l_time); * m_tm = * localtime(&l_time);
	}
}

/*
	a PDF data stream - a sequence of chars reperesented by a dictionary
	<dictionary>
	stream (followed by cr+lf or just lf)
	{ lines of chars } *
	endstream

	stream attrubutes:
		Length (integer) : required, #chars after the line containing stream keyword to the endstream
		Filter (name or array of names) : optional, filters to be applied in processing the stream
				a single name or an array of names of filters to apply, ex: /Filter [/ASCII85Decode /LZWDecode]
		DecodeParams (various) : params used by the decoding filters specified by with the Filter key
		F (file) : filename - ignore chars between stream and endstream, length should be valid still
			(usually zero)
		FFilter (name or array of names), optional, filters to be applied in processing 
			data found in the external file
		FDecodeParams: (variable), params used with FFilter
	*/	

void PDFObjects::ReadStream(const CString& Stream, int& curPos, CHashTable<PDFObjects>& Dictionary) const {
	if (Stream.Copy(curPos, strlen("stream")).isSame("stream")){		
		PDFObjects NewStream(Stream, curPos);
		curPos+=strlen("stream");
		SkipCarriageSpaces(Stream, curPos);
		if (Dictionary.SearchRef("F")) {
			cout << "WARNING! external files are not supported (yet)" << endl;
		} else {
			int StreamLen = Dictionary.Search("Length").asInteger();
			_PDF_DEBUG("PDFObjects::ReadDictionary() - stream of length " << StreamLen << endl);
			NewStream.m_Name = "Stream";
			NewStream.m_Data = new CString(Stream.asString()+curPos);			
			NewStream.m_ObjectType = o_Stream;							
		}
		/*
			see what the filter looks like
			*/
		PDFObjects * FilterRef = Dictionary.SearchRef("Filter");
		int i;
		if (FilterRef){
			switch (FilterRef->m_ObjectType){
			case o_String:
				ApplyFilter(FilterRef->asString(), * (CString *) NewStream.m_Data, Dictionary, Dictionary.Search("DecodeParams"));
				break;
			case o_MixedArray:
				for (i=0;i<FilterRef->asStringVector().Count();i++)
					ApplyFilter(FilterRef->asStringVector()[i], * (CString *) NewStream.m_Data, Dictionary, Dictionary.Search("DecodeParams").DictionaryGetValueObjectsVector(i));
				break;
			default:					
				cout << "ERROR! Unknown filter format." << endl;
				break;
			}
		} //else process error
		Dictionary.Add("Stream", NewStream);
	} 
}

/*
	should digest data retrieved with the PDF data stream
	ASCIIHexDecode (no params) Decodes binary data in an ASCII hex representation
	ASCII85Decode (no params) Decodes binary data in an ASCII base 85 representation
	LZWDecode (dictionary, optional) Decompress text or binary data using LZW adaptive compression
	RunLengthDecode (no params) Decompress binary data using a byte-oriented run-length decoding
	CCITTFaxDecode (dictionary, optional) Decompress binary data using a bit-oriented decoding, the CCITT fax std
	DCTDecode (dictionary, options) Decompress sampled image data using a descrete cosine transform tech based on JPEG standard
	FlateDecode (dictionary, optional) Decompress text or binary data using the Flate decomp method

	filters are explained on p 49 of the PDFRef with their params
	*/
void PDFObjects::ApplyFilter(CString Filter, CString& Data, CHashTable<PDFObjects>& Dictionary, PDFObjects& Params) const {
	_PDF_DEBUG("Applying " << Filter << " on " << Data.StrLength() << " bytes of data." << endl);
	if (Filter.isSame("FlateDecode")){		
		/*
		zlib/deflate compresion RFC 1950/1951
		additional operators:
			Predictor : if =1 -> normal deflate, otherwise data was pre-differentiated
			Columns : if (Predictor > 1) No. of samples in a sampled row 
			Colors
			BitsPerComponent
		*/		
		if (Params.m_ObjectType != o_Undefined){
			cout << "ERROR! ZLib/deflate operators unsupported (yet)" << endl;
			return;
		}		
		Data = ZLIB.DecompressString(Data);										
	} else cout << "ERROR! " << Filter << " unsupported (yet)" << endl;
}

CString PDFObjects::StreamToString(void){
	CString Result;	
	if (m_ObjectType == o_Stream) {
		//Result += "\n=================\n";
		/*
			<text object> ::=
				BT
				<text operator or graphics state operator>
				ET
			*/

		CVector<float> FloatContext;
		CVector<CString> NameContext;
		CVector<CString> StringContext;
		CVector<PDFObjects> MixedArray;

		int curPos = 0;
		int BT_Processing = 0;
		while (curPos < (* (CString *) m_Data).StrLength()){
			SkipCarriageSpaces((* (CString *) m_Data), curPos);
			if ((isdigit((* (CString *) m_Data)[curPos])|| ((* (CString *) m_Data)[curPos] == '+') || ((* (CString *) m_Data)[curPos] == '-'))) {	
				FloatContext += (float) ReadNumber((* (CString *) m_Data), curPos);
				//cout << "float::" << FloatContext[FloatContext.Count()-1] << endl;
			} else if ((* (CString *) m_Data)[curPos] == '/') {
				NameContext += ReadName((* (CString *) m_Data), curPos);
				//cout << "name::" << NameContext[NameContext.Count()-1] << endl;
			} else if ((* (CString *) m_Data)[curPos] == '(') {
				StringContext += ReadString((* (CString *) m_Data), curPos);
				//cout << "string::" << StringContext[StringContext.Count()-1] << endl;
			} else if ((* (CString *) m_Data)[curPos] == '[') {
				MixedArray = ReadMixedArray((* (CString *) m_Data), curPos);								
			} else {
				CString Operator = ReadWord((* (CString *) m_Data), curPos);
				if (!Operator.StrLength()) curPos++;
				//cout << "operator::" << Operator << endl;
				if (Operator.isSame("BT")){
					BT_Processing = 1;
				} else if (Operator.isSame("ET")){
					BT_Processing = 0;
				} else if (BT_Processing) {
					/*
						text object
						*/
					if (Operator.isSame("Td")){
						/*
							(tx ty)
							*/						
						Result += "\n";
						FloatContext.Clear();
					} else if (Operator.isSame("TD")){
						/*
							(tx ty)							
							*/
						Result += "\n";
						FloatContext.Clear();
					} else if (Operator.isSame("Tm")){
						/* (a b c d x y) */
						FloatContext.Clear();
					} else if (Operator.isSame("T*")){
						/* () */
						FloatContext.Clear();
						Result += "\n";
					} else if (Operator.isSame("Tj")){
						/* show */
						if (StringContext.Count()) {
							//Result += " ";
							Result += StringContext[StringContext.Count()-1];
						}
						StringContext.Clear();
					} else if (Operator.isSame("\'")){
						if (StringContext.Count()) {
							//Result += " ";
							Result += StringContext[StringContext.Count()-1];
						}
						Result += "\n";
						StringContext.Clear();						
					} else if (Operator.isSame("\"")){
						if (StringContext.Count()) {
							//Result += " ";
							Result += StringContext[StringContext.Count()-1];
						}
						Result += "\n";
						StringContext.Clear();
					} else if (Operator.isSame("TJ")){
						for (int i=0;i<MixedArray.Count();i+=2){
							Result+=" ";
							Result+=MixedArray[i].asString();								
						}
						MixedArray.Clear();
					} else {
						//cout << "unknown operator " << Operator << endl;
						//ignoring operator
						FloatContext.Clear();
						NameContext.Clear();
						StringContext.Clear();				
						MixedArray.Clear();
					}

				} else {
					FloatContext.Clear();
					NameContext.Clear();
					StringContext.Clear();				
					MixedArray.Clear();
				}
			}
			
		}

			/*
		Result+=("\n---------------\n");
		Result += * (CString *) m_Data;
		Result += "\n=================\n";
		*/
		
		
		return Result;


	} else if (m_ObjectType == o_String) return * (CString *) m_Data;
	else return "";
}


#include <chashtable/chashtable.cpp>
template class CHashTable<PDFObjects>;
#include <cvector/cvector.cpp>
template class CVector<PDFObjects>;