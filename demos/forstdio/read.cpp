/**********************************************************************/
/* File:                                                              */
/* Author:                                                            */
/* This codes is generated by CFortranTranslator                      */
/* CFortranTranslator is published under GPL license                  */
/* refer to https://github.com/CalvinNeo/CFortranTranslator/ for more */
/**********************************************************************/
#include "../../for90std/for90std.h" 
#define USE_FORARRAY 
int main()
{
	string buf = "                    ";
	string chr = "";
	int i = 0;
	int j = 0;
	string str = "                    ";
	str = SS("111 222 apple");
	forread(str, IOFormat{"%d\n", 0, 2}, &i);
	forwritefree(stdout, i);
	forread(str, IOFormat{"%d\n", 0, 2}, &j);
	forwritefree(stdout, j);
	forread(str, IOFormat{"%d%d%1s%s\n", 0, 9}, &i, &j, &chr, &buf);
	forwritefree(stdout, i, j, chr, buf);
	
	
	
	return 0;
}
