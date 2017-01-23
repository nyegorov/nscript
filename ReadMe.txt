========================================================================
    Numeric parser
========================================================================

Test project to learn script parsing and COM interoperability

Performs single-pass script parsing, including:

 * integer, double, string and date manipulations
 * built-in functions: sin, cos, ln, etc.
 * user-defined symbols and functions
 * flow control statements (if, for)
 * classes, lambdas and COM objects

Examples can be found in UnitTest subfolder

Windows only (uses Win32 VARIANT type and COM infrastructure).

/////////////////////////////////////////////////////////////////////////////
Author:

Nick Yegorov, nick.yegorov@gmail.com

/////////////////////////////////////////////////////////////////////////////
Usage:

To use parser in your project, include files "nscript.h" and "nscript.cpp". 
Add "nhash.h" for associative arrays support and "ndispatch.h" for using 
COM objects.

/////////////////////////////////////////////////////////////////////////////
Other notes:

Project uses lightweight COM support library that also can be useful. See 
unit tests for more information.

/////////////////////////////////////////////////////////////////////////////
