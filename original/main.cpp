#include "PDFFile.h"

void main(int argc, char ** argv){
	if (argc > 1) {
		PDFFile PDF(argv[1]);
	}
}

#include <cvector/cvector.cpp>
template class CVector<int>;


