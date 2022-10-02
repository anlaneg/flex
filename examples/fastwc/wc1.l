/* First cut at a flex-based "wc" tool. */

ws    [ \t]
nonws [^ \t\n]

%option main noyywrap
%%
	int cc = 0/*字符数*/, wc = 0/*word数目*/, lc = 0/*行数*/;

/*遇到非空字符，增加字符数，单词数。yyleng记录匹配数目*/
{nonws}+	cc += yyleng; ++wc;

/*遇到空字符，增加字符数，yyleng记录匹配数目*/
{ws}+		cc += yyleng;

/*遇到换行符，增加字符数*/
\n		++lc; ++cc;

/*遇到文件结尾，输出统计信息，并退出*/
<<EOF>>		{
		printf( "%8d %8d %8d\n", lc, wc, cc );
		yyterminate();
		}
