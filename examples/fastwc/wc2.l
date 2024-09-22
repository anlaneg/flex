/* Somewhat faster "wc" tool: match more text with each rule */

ws    [ \t]
nonws [^ \t\n]
word  {ws}*{nonws}+

%option main noyywrap
%%
	int cc = 0, wc = 0, lc = 0;

/*这种分析快的原因是啥？匹配的token较wc1.l变多了*/
{word}{ws}*	cc += yyleng; ++wc;
{word}{ws}*\n	cc += yyleng; ++wc; ++lc;

{ws}+		cc += yyleng;

/*遇到换行符(多行），增加字符数*/
\n+		cc += yyleng; lc += yyleng;

<<EOF>>		{
		printf( "%8d %8d %8d\n", lc, wc, cc );
		yyterminate();
		}
