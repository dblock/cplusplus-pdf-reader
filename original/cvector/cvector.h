#ifndef CVECTOR_H
#define CVECTOR_H

#include <iostream.h>
#include <fstream.h>
#include <assert.h>

template <class T> 
class CVector;

template <class T>
class ICVector {
private:
  int m_nArrayDim;  
  int m_nArrayFill;
  T * m_pArray;  
  int m_Refs;
  void AllocSize(int);
public:
  /*
    constructors and registration pointers
    */
  ICVector<T>(void);
  ICVector<T>(int); 
  ~ICVector(void);
  /*
    reference ops
    */
  int RefCount(void) const;
  void Reg(void);
  void UnReg(void);
  ICVector<T> * ICVectorCopy(void);   
  ICVector<T>& Grow(int);
  /*
    variant ops
    */
  void QSort(void);
  void QSortUnique(void);
  void QSort(int l, int r);
  ICVector<T>& AddElt(const T&);
  ICVector<T>& AddElt(const T&, int);
  int AddEltSorted(const T&);
  ICVector<T>& DeleteElt(int);
  ICVector<T>& DeleteEltAt(int);
  int FindElt(const T&);
  friend class CVector<T>;
};

template <class T>
class CVector {
private:
  ICVector<T> * m_IContainer;
  void ICSCopy(int Size = 5);
public:
  /*
    constructors, constructor ops
    */
  CVector<T>(void);
  CVector<T>(int);
  CVector<T>(const CVector<T>&);
  ~CVector(void);
  void operator=(const CVector<T>&);
  /*
    constant retrievals
    */
  T& GetElt(const int nIndex) const;
  int FindElt(const T& Elt) const;
  int Contains(const T&) const;
  int Count(void) const;  
  T& operator[](const int nIndex) const;
  int operator==(const CVector<T>&) const;
  int operator>(const CVector<T>&) const;
  int operator<(const CVector<T>&) const;
  int operator>=(const CVector<T>&) const;
  int operator<=(const CVector<T>&) const;
  int operator!=(const CVector<T>&) const;
  /*
    size and element adjustement
    */
  CVector<T>& Grow(int);

  CVector<T>& AddElt(const T&, int);
  CVector<T>& AddElt(const T&);
  int AddEltSorted(const T&);
  int AddEltSortedUnique(const T&);  
  CVector<T>& operator+=(const T&);


  void operator-=(const T&);
  void operator-=(const int);  
  CVector<T>& DeleteElt(int nIndex);
  CVector<T>& DeleteEltAt(int nIndex);

  void Clear(void);  
  void Flush(CVector<T>&);  
    
  void QSort(void);
  void QSortUnique(void);
  
  friend ostream& operator<<(ostream&, const CVector<T>&);
  friend ofstream& operator<<(ofstream&, const CVector<T>&);
#ifdef ANSI
  friend cgiOutStream& operator<<(cgiOutStream&, const CVector<T>&);
#endif
};

#endif
