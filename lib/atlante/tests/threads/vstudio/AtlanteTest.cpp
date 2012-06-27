// Atlante.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
using namespace std;

#include "../../../atlante/include/cvg_types.h"

void a();

int _tmain(int argc, _TCHAR* argv[])
{
	a();
	cvg_long a = CVG_LITERAL_LONG(0x1234567812345678);
	printf("0x%"CVG_PRINTF_PREFIX_LONG"x\n", a);

	getc(stdin);
	return 0;
}

