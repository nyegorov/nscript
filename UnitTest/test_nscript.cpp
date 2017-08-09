#include "pch.h"
#include "CppUnitTest.h"

#include "../NScriptHost/NScript/NScript.h"
#include "../NScriptHost/NScript/NHash.h"
#include "../NScriptHost/NScript/NDispatch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

namespace UnitTest
{		
	TEST_CLASS(NScriptTest)
	{
		HRESULT eval_hr(const char *expr) {
			nscript::NScript ns;
			variant_t v;
			ns.AddObject(L"map", (nscript::IHash*)(new nscript::Hash()));
			ns.AddObject(TEXT("createobject"), new nscript::CreateObject());
			return ns.Exec(bstr_t(expr), v);
		}
		string eval(const char *expr) {
			nscript::NScript ns;
			variant_t	v;
			string		res;
			ns.AddObject(L"map", (nscript::IHash*)(new nscript::Hash()));
			ns.AddObject(TEXT("createobject"), new nscript::CreateObject());
			HRESULT hr = ns.Exec(bstr_t(expr), v);
			if (FAILED(hr)) {
				IErrorInfoPtr pei;
				GetErrorInfo(0, &pei);
				_com_error e(hr, pei, true);
				return (LPCSTR)e.Description();
			}
			try {
				if (V_VT(&v) & VT_ARRAY) {
					nscript::SafeArray sa(v);
					res = '[';
					for (int i = 0; i<sa.Count(); i++) {
						if (i)	res += "; ";
						res += bstr_t(sa[i]);
					}
					res += ']';
				}
				else
					res = (LPCSTR)bstr_t(v);
			}
			catch (...) { res = "Type mismatch."; }
			return res;
		}

	public:

		TEST_METHOD(Parsing)
		{
			Assert::AreEqual("2,1", eval("2.10000000000000000000000000000000000001").c_str());
			Assert::AreEqual("2", eval("2").c_str(), "integers", LINE_INFO());
			Assert::AreEqual("-1234567890123456789", eval("-1234567890123456789").c_str(), "long int", LINE_INFO());
			Assert::AreEqual("31", eval("0x1F").c_str(), "hex", LINE_INFO());
			Assert::AreEqual("3,14", eval("3.14").c_str(), "float", LINE_INFO());
			Assert::AreEqual("-1,314", eval("-3.14e-1-1e+0").c_str(), "exp", LINE_INFO());
			Assert::AreEqual("a\"b\"'c'", eval("'a\"b\"'+\"'c'\"").c_str(), "string", LINE_INFO());
			Assert::AreEqual("3", eval("a=3;a").c_str(), "variable", LINE_INFO());
			Assert::AreEqual("26.10.1974", eval("#26.10.74#").c_str(), "date", LINE_INFO());
			Assert::AreEqual("[1; 2; 3]", eval("[1,2,3]").c_str(), "array", LINE_INFO());
		}
		TEST_METHOD(Statements)
		{
			Assert::AreEqual("ok", eval("\
				intr := sub(f,a,b,dx) {for(my s=0, my x=a;x<b;x+=dx) s+=f(x)*dx; s}; \
				if(intr(sub(x) x^2, 0, 2, 0.01)-2^3/3 < 0.01) 'ok' else 'fail'").c_str());
			Assert::AreEqual("1", eval("x=2; test = sub {x=1;}; test(); x").c_str());
			Assert::AreEqual("2", eval("x=2; test = sub {my x; x=1;}; test(); x").c_str());
		}
		TEST_METHOD(Operators)
		{
			Assert::AreEqual("-1", eval("3^(2*(5-3))==81").c_str());
			Assert::AreEqual("-5", eval("x=1; y=x++; z=++x; y-z+-x").c_str());
			Assert::AreEqual("3", eval("x=1; y=x--; z=--x; y-z+-x").c_str());
			Assert::AreEqual("1,5", eval("5/2-5\\2+5%2").c_str());
			Assert::AreEqual("A3", eval("upper(hex(0xAA & ~0x0F | 0x3))").c_str());
			Assert::AreEqual("ok", eval("(1>2 || 1>=2 || 1<=2 || 1<2) && !(3==4) && (3!=4) ? 'ok' : 'fail'").c_str());
			Assert::AreEqual("fail", eval("(2<=1 || 1<1 || 1>1 || 1<1) && !(3==3) && (3!=3) ? 'ok' : 'fail'").c_str());
			Assert::AreEqual("-0,5", eval("x=1; y=2; x+=y; y-=x; x*=y; x\\=y; x-=1; y/=2").c_str());
		}
		TEST_METHOD(Functions)
		{
			// conversions
			Assert::AreEqual("", eval("empty").c_str());
			Assert::AreEqual("-1", eval("true").c_str());
			Assert::AreEqual("0", eval("false").c_str());
			Assert::AreEqual("6", eval("int(pi())+int('2.9')").c_str());
			Assert::AreEqual("0,14", eval("dbl('3.14')-dbl(3)").c_str());
			Assert::AreEqual("12", eval("str(1)+str(2)").c_str());
			// date
			Assert::AreEqual("26.10.1974", eval("date('26.10.74')").c_str());
			Assert::AreEqual("-1", eval("year(now())>2012").c_str());
			Assert::AreEqual("26", eval("day(#26.10.74#)").c_str());
			Assert::AreEqual("10", eval("month(#26.10.74#)").c_str());
			Assert::AreEqual("1974", eval("year(#26.10.74#)").c_str());
			Assert::AreEqual("13", eval("hour(#13:15#)").c_str());
			Assert::AreEqual("15", eval("minute(#13:15:45#)").c_str());
			Assert::AreEqual("45", eval("second(#13:15:45#)").c_str());
			Assert::AreEqual("5", eval("dayofweek(#23.08.2013#)").c_str());
			Assert::AreEqual("235", eval("dayofyear(#23.08.2013#)").c_str());
			// math
			Assert::AreEqual("0", eval("pi()-atan2(1,1)*4").c_str());
			Assert::AreEqual("0", eval("a=pi()/3;sin(a)/cos(a)-tan(a)").c_str());
			Assert::AreEqual("314", eval("int(atan(tan(pi()/4))*100*4)").c_str());
			Assert::AreEqual("0", eval("exp(log(pi()))-pi()").c_str());
			Assert::AreEqual("20", eval("abs(sqrt(25)-5^2)").c_str());
			Assert::AreEqual("2", eval("sgn(pi())-sgn(-pi())+sgn(0)").c_str());
			Assert::AreEqual("0,14", eval("fract(0.14)").c_str());
			Assert::AreEqual("-1", eval("rnd() <= 1 && rnd() >= 0").c_str());

			// string
			Assert::AreEqual("ABC DEF", eval("upper('abc'+chr(32)+'def')").c_str());
			Assert::AreEqual("ace", eval("s='ABCDE';lower(left(s,1)+mid(s,2,1)+right(s,1))").c_str());
			Assert::AreEqual("67", eval("asc('A')+asc('')+instr('abcdef', 'c')").c_str());
			Assert::AreEqual("5aaaaa", eval("s=string(5,'s');str(len(s))+replace(s,'s','a')").c_str());
			Assert::AreEqual("26-10-74", eval("format(#26.10.1974#, 'DD-MM-YY')").c_str());
			Assert::AreEqual("3,14", eval("format(pi(), '#.##')").c_str());
			Assert::AreEqual("-01,20", eval("format(-1.2, '00.00')").c_str());
			// other
			Assert::AreEqual("-5", eval("min(1, pi(), -5)+min()").c_str());
			Assert::AreEqual("0,75", eval("max(1/2, 3/4, 2/3)+max()").c_str());
			Assert::AreEqual("7F3F0F", eval("hex(rgb(15,63,127))").c_str());

		}
		TEST_METHOD(Arrays)
		{
			Assert::AreEqual("2", eval("x=1;x[0]=2;x[0]").c_str());
			Assert::AreEqual("2", eval("[1,2,3][1]").c_str());
			Assert::AreEqual("3", eval("size([4,5,6])").c_str());
			Assert::AreEqual("10", eval("a=[[1,2,[3,4]],[4,5,6]];a[0][1]=a[0][2][1]+a[1][2]").c_str());
			Assert::AreEqual("bbb", eval("a=[];a=add(a,'aaa');a=add(a,'bbb');a=remove(a,0);a[0]").c_str());
			Assert::AreEqual("3", eval("m=new map; m['abc']=3; m['abc']").c_str());
			Assert::AreEqual("", eval("m=map; mm=new m; mm[0]").c_str());
		}
		TEST_METHOD(Functional)
		{
			Assert::AreEqual("15", eval("sum = fold(fn(x,y) x+y); sum([4,5,6])").c_str());
			Assert::AreEqual("36", eval("fold(fn(x,y) x*y)(fmap(fn @^2)([1,2,3]))").c_str());
			Assert::AreEqual("35", eval(R"(
				odds   = fn @%2==1; 
				square = fn @^2; 
				plus   = fn(x,y) x+y; 
				[1,2,3,4,5] | filter(odds) | fmap(square) | fold(plus)
			)").c_str());
			Assert::AreEqual("brothers,in,metal", eval("['brothers', 'in', 'metal'] | fold(fn(x,y) x+','+y)").c_str());
			Assert::AreEqual("DRAGONS=KEEPERS", eval(R"(
				join  = fn(c)    fold(fn(x, y) x+c+y);
				split = fn(s, c) (n = instr(s, c)) < 0 ? s : [left(s, n):split(mid(s, n + 1, len(s) - n), c)];
				split('holy dragons keepers of time', ' ') | filter(fn len(@)>4) | fmap(upper) | join('=')
			)").c_str());

			Assert::AreEqual("[1; 1; 2; 3; 3; 4; 5]", eval(R"(
				greater = fn(x) filter(fn(y) y > x);
				lesse   = fn(x) filter(fn(y) y <= x);
				qsort   = fn @ == [] ? [] : [tail(@) | lesse(head(@)) | qsort : head(@) : tail(@) | greater(head(@)) | qsort];
				[2,1,5,4,3,1,3] | qsort
			)").c_str());

		}
		TEST_METHOD(Classes)
		{
			Assert::AreEqual("1", eval("f=sub {1?new object{x=1}:0};f().x").c_str());
			Assert::AreEqual("10", eval("\
				point = object(x,y) {\
					length = sub {sqrt(x^2+y^2)};\
				};\
				dist = sub(p1, p2) {sqrt((p1.x-p2.x)^2 + (p1.y-p2.y)^2)};\
				p1=new point(3,4); p2 = new point(3, -1);\
				p1.length() + dist(p1,p2);\
			").c_str());
		}
		TEST_METHOD(Dispatch)
		{
			Assert::AreEqual("matrix has you, neo!", eval("\
				o = CreateObject(\"MSXML2.DOMDocument\");\
				o.loadXML('<Root><Item Name=\"choosen one\">neo</Item></Root>');\
				neo = o.selectSingleNode('/*/Item[@Name=\"choosen one\"]').Text;\
				o.selectSingleNode('/*/Item[@Name=\"choosen one\"]').Text = 'matrix';\
				mat = o.selectNodes('/*/Item[@Name=\"choosen one\"]').item[0].Text;\
				mat + ' has you, ' + neo + '!'\
			").c_str());
		}
		TEST_METHOD(Errors)
		{
			Assert::AreEqual("Missing ')' character", eval("(1,2").c_str());
			Assert::AreEqual(E_NS_MISSINGCHARACTER, eval_hr("(1,2"));
			Assert::AreEqual(E_NS_MISSINGCHARACTER, eval_hr("{c=1"));
			Assert::AreEqual(E_NS_MISSINGCHARACTER, eval_hr("[1,2"));
			//Assert::AreEqual(E_NS_SYNTAXERROR,	eval_hr("5+"));
			Assert::AreEqual(DISP_E_UNKNOWNNAME, eval_hr("x+2"));
			//Assert::AreEqual(E_NS_TOOMANYITERATIONS, eval_hr("for(;1;) {1}"));
			Assert::AreEqual(E_NS_SYNTAXERROR, eval_hr("object(x) {"));
			Assert::AreEqual(E_NS_SYNTAXERROR, eval_hr("sub(x,$);"));
			Assert::AreEqual(E_NOTIMPL, eval_hr("(new object {})(0)"));
			Assert::AreEqual(E_NOTIMPL, eval_hr("(new object {})=1"));
			Assert::AreEqual(E_NOTIMPL, eval_hr("(new object {})[0]"));
			Assert::AreEqual(E_NOTIMPL, eval_hr("new (new object)"));
			Assert::AreEqual(DISP_E_BADPARAMCOUNT, eval_hr("new (object(x,y) {})()"));
			Assert::AreEqual(DISP_E_BADPARAMCOUNT, eval_hr("sin(1,2)"));
			Assert::AreEqual(DISP_E_BADINDEX, eval_hr("x=1;;;;x[5]"));
			Assert::AreEqual(DISP_E_BADINDEX, eval_hr("1[5]"));
			Assert::AreEqual(DISP_E_TYPEMISMATCH, eval_hr("(new map)[0]()"));
			Assert::AreEqual(DISP_E_TYPEMISMATCH, eval_hr("(new map)[0].x"));
			Assert::AreEqual(DISP_E_TYPEMISMATCH, eval_hr("(new map)[0][0]"));
			Assert::AreEqual(E_OUTOFMEMORY, eval_hr("a=[];a[0x7fffffff]=1"));
			Assert::AreEqual(E_NS_SYNTAXERROR, eval_hr("1e2.3"));
			Assert::AreEqual(E_NS_SYNTAXERROR, eval_hr("1e2e3"));
			Assert::AreEqual(DISP_E_OVERFLOW, eval_hr("0x1ffffffffffffffff"));
			Assert::AreEqual(DISP_E_OVERFLOW, eval_hr("999999999999999999999999999"));
		}

	};
}