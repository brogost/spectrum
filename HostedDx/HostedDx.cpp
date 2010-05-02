// HostedDx.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

extern "C"
{
	__declspec(dllexport) int funky_test()
	{
		return 42;
	}

};