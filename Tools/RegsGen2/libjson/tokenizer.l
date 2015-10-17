
/*
  Copyright Nils Alexander Roemcke 2005. 

  Use, modification, and distribution are subject to the Boost Software 
  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at 
  http://www.boost.org/LICENSE_1_0.txt)
*/


%{
#include "qstring.h"
%}

%option 8bit reentrant 
%option bison-bridge
%option bison-locations

%option   yylineno


%option noyywrap


%{
#include "parser.h"

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno; \
        yylloc->first_column = yycolumn; yylloc->last_column = yycolumn + yyleng - 1; \
        yycolumn += yyleng;

%}

%option yylineno

%x c_comment
%x cpp_comment

IDENTIFIER [_a-zA-Z][_0-9a-zA-Z]*

INTEGER    "-"?(0|([1-9][0-9]*))
REAL       "-"?(0|([1-9][0-9]*))"."[0-9]+((e|E)("+"|"-")?[0-9]+)?

STRING     "\""([^\n"\\]|("\\".))*"\""

BOPEN      "{"
BCLOSE     "}"
SBOPEN     "["
SBCLOSE    "]"
COLON      ":"
COMMA      ","

TRUE        "true"
FALSE       "false"
NULLVAL     "null"


%%


[ \n\r\t]+        /* eat up whitespace */


"//"                      BEGIN(cpp_comment);
<cpp_comment>[^\n]*
<cpp_comment>\n           BEGIN(INITIAL);
     

"/*"                      BEGIN(c_comment);
<c_comment>[^*\n]*
<c_comment>[^*\n]*\n      /* ++line_num; */
<c_comment>"*"+[^*/\n]*
<c_comment>"*"+[^*/\n]*\n /* ++line_num; */
<c_comment>"*"+"/"        BEGIN(INITIAL);



{TRUE}        yylval->v_bool = -1; return JBOOLEAN;
{FALSE}       yylval->v_bool = 0; return JBOOLEAN;
{NULLVAL}     return NULLVAL;


{IDENTIFIER}  yylval->v_string = strdup(yytext); return IDENTIFIER;

{INTEGER}     yylval->v_int = atoi(yytext); return INTEGER;
{REAL}        yylval->v_double = atof(yytext); return REAL;

{STRING}      yylval->v_string = _json_string_unquote(yytext); return STRING;


{BOPEN}       return BOPEN;
{BCLOSE}      return BCLOSE;
{SBOPEN}      return SBOPEN;
{SBCLOSE}     return SBCLOSE;
{COLON}       return COLON;
{COMMA}       return COMMA;

.             return yytext[0];

%%

/*
int main(int argc, char ** argv)
{
  yyscan_t yy;
  YYSTYPE lvalp;

  yylex_init(&yy);
  yylex (&lvalp, yy);

  yylex_destroy(yy);

  return 0;
}
*/ 
