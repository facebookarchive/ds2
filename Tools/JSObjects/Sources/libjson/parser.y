
/*
  Copyright Nils Alexander Roemcke 2005. 

  Use, modification, and distribution are subject to the Boost Software 
  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at 
  http://www.boost.org/LICENSE_1_0.txt)
*/


%{


#include <stdio.h>
#include <string.h>

#include "json_internal.h"

  
//#define YYPARSE_PARAM scanner
#define YYLEX_PARAM   scanner

struct parse_state_t;

%}



%locations
%pure-parser
%error-verbose
%parse-param {void *scanner} 
%parse-param {void *ps}
%lex-param {void *scanner}

%union value {

  int    v_bool;
  int    v_int;
  double v_double;
  char   *v_string;
 
  void   *object;
}

%{

#include "tokenizer.h"

void
yyerror (YYLTYPE *yylloc, yyscan_t *scanner, struct parse_state_t *ps, char const *s)
{
  if (ps->err_cb)
    (ps->err_cb)(ps->err_data, yylloc->first_line, yylloc->first_column, s);
  else
    fprintf (stderr, "%s\n", s);
}


%}



%token NUMBER


%token <v_string> IDENTIFIER

%token <v_int> INTEGER
%token <v_double> REAL

%token <v_string> STRING

%token BOPEN
%token BCLOSE
%token SBOPEN
%token SBCLOSE
%token COLON
%token COMMA

%token <v_bool> JBOOLEAN
%token NULLVAL



%start value
%pure-parser
%locations


%%



object:  BOPEN           { _json_parse_obj_start (ps); }
         members BCLOSE  { _json_parse_obj_end (ps); }
;



member:  identifier COLON value
;


identifier:  STRING        { _json_parse_member(ps, $1 ); }
           | IDENTIFIER    { _json_parse_member(ps, $1 ); }
;


members:  /* empty */
        | member
        | members COMMA member
	| error
;


array:   SBOPEN            { _json_parse_array_start (ps); }
         elements SBCLOSE  { _json_parse_array_end (ps); }
;


elements:   /* empty */
          | value
          | elements COMMA value
	  | error
;


value:   STRING   { _json_parse_string(ps, $1 ); }
       | INTEGER  { _json_parse_int(ps, $1 ); }
       | REAL     { _json_parse_double(ps, $1 ); }
       | object  
       | array    
       | JBOOLEAN   { _json_parse_bool(ps, $1 ); }
       | NULLVAL  { _json_parse_null(ps); }
 
;
     



%%


/*
int
main (int argc, char ** argv)
{
  if (argc > 1)
    json_parse(argv[1], NULL, NULL, NULL, NULL);

  return 0;
}
*/

int 
json_fparse(FILE *f, 
	    struct json_cb_t *vb, 
	    void *root, 
	    json_err_cb *err_cb,
	    void *err_data)
{

  struct parse_state_t ps;

  int result;

  yyscan_t yy;

  memset(&ps, 0, sizeof(struct parse_state_t));
  
  ps.err_cb = err_cb;
  ps.err_data = err_data;

  ps.context = _json_context_push_root(NULL, vb, root);
  if (!ps.context)
    return -1;

  if (yylex_init(&yy))
    {
      _json_context_pop(ps.context);
      return -1;
    }


  yyset_in(f, yy);
  
  result = yyparse (yy, &ps);

  yylex_destroy(yy);
  _json_context_popall(ps.context);

  return result;
   
}
 
