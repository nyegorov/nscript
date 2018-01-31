// Testbench.cpp : Defines the entry point for the console application.
//

#include "pch.h"
#include "../NScriptHost/NScript3/NScript3.h"

using namespace std;

int main()
{
	nscript3::NScript ns;
	error_code ec;
	auto v = ns.eval("1 ? 'ok' : 'false' ", ec);
	//auto v = ns.eval("-3", ec);
	cout << to_string(v) << endl;
	return 0;
}

