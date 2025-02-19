/*
*   Calvin Neo
*   Copyright (C) 2016  Calvin Neo <calvinneo@calvinneo.com>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License along
*   with this program; if not, write to the Free Software Foundation, Inc.,
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
%{
#include <cstdio>
#include <iostream>
#include <functional>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <memory>
#include "../parser/tokenizer.h"
#include "../parser/parser.h"
#include "../parser/context.h"
#include "../target/gen_config.h"
#include "for90.tab.h"
#include <tuple>
#define YY_DECL int pure_yylex(void)


static char * iter_buff = nullptr;
static YY_BUFFER_STATE yy_buffer = nullptr;

void update_flex(int len) {
	// do not update CurrentTerm
	bool newline = std::string(yytext) == "\n";
	if (newline) {
		get_flex_state().parse_line++;
		get_flex_state().parse_pos += len; get_flex_state().line_pos = 0; get_flex_state().parse_len = len;
	}
	else {
		get_flex_state().parse_pos += len; get_flex_state().line_pos += get_flex_state().parse_len; get_flex_state().parse_len = len;
	}
}
void update_flex() {
	// do not update CurrentTerm
	update_flex(yyleng);
}
void update_yylval(Term & current_term) {
	ParseNode newnode{ FlexState(get_flex_state()) , nullptr, nullptr };
	newnode.fs.CurrentTerm = current_term;
	yylval = RETURN_NT(newnode);
}

int yylex(void) {
	int p;
	Term term;
	if (!get_flex_context().terminal_cache.empty())
	{
		// use terminal in the cache
		std::tie(p, term) = get_flex_context().terminal_cache.back();
		update_yylval(term);
		get_flex_context().terminal_cache.pop_back();
	}
	else {
		p = pure_yylex();
		term = get_flex_state().CurrentTerm;
	}
	if (term.what == "format")
	{

		if (text[0] == '(' && !in_str)
		{
			left_brackets++;
		}
		else if (text[0] == ')' && !in_str) {
			left_brackets--;
			if (left_brackets == 0)
			{
				std::string result = code;
				update_flex((int)len);
				return quit_condition(YY_FORMAT_STMT, Term{ TokenMeta::NT_FORMAT, result });
			}
		}
		else {
			if (text[0] == '\"' || text[0] == '\'')
			{
				in_str = !in_str;
			}
			else if (text[0] == '\n')
			{
				get_flex_state().parse_line++;
			}
		}
		code += text;
	}
	else if (term.token == TokenMeta::Label)
	{
		// handle end label of do
	}
	return p;
}

static bool abandon = false;

int else_parse(const char * origin_yytext);
int comment_parse(const char * origin_yytext);
int label_parse(const char * origin_yytext);
int literal_parse(const char * origin_yytext, TokenMeta_T token, int yytoken);
int nop_parse();
int crlf_parse(const char * origin_yytext);
int word_parse(const char * origin_yytext);
int operator_parse(const char * origin_yytext, TokenMeta_T token, int yytoken);

#define DORET(YYTOKEN) if(YYTOKEN == YY_IGNORE_THIS){/* do not return */}else{ return YYTOKEN; }

%}

%option yymore
%x external_scanner

delim   [ \t]
ws      {delim}+
newline [\r\n]
/***
R412 signed - real - literal - constant is[sign] real - literal - constant
R413 real - literal - constant is significand[exponent - letter exponent][_ kind - param]
	or digit - string exponent - letter exponent[_ kind - param]
R414 significand is digit - string .[digit - string]
	or .digit - string
R415 exponent - letter is E
	or D
R416 exponent is signed - digit - string
Constraint : If both kind - param and exponent - letter are present, exponent - letter must be E.
	Constraint : The value of kind - param must specify an approximation method that exists on the processor.
	A real literal constant without a kind type parameter is a default real constant if it is without an exponent part or
	has exponent letter E, and is a double precision real constant if it has exponent letter D.A real literal constant
	written with a kind type parameter is a real constant with the specified kind type parameter.
**/
digit [0-9]
letter [a-zA-Z]
digits {digit}+
kind_param (digits|{letter}+)
exponent [eEdD][-+]?{digits}?\.{digits}
floating {digits}?\.{digits}{exponent}?
narrow_floating {digits}\.
integral {digits}(_{kind_param})?
starter({letter}|_)
wordname {starter}({starter}|{digit})*
wordoperator \.{wordname}\.
word ({wordname}|{wordoperator})
string1 \"(\\.|[^"])*\"
string2 \'(\\.|[^'])*\'
string ({string1}|{string2})

%%

<external_scanner>{newline} {
	// this rule is essential or `\n` will br left out intentionally
	int p = get_current_scanner()->do_scanner(yytext);
	DORET(YY_IGNORE_THIS)
}
<external_scanner>. {
	int p = get_current_scanner()->do_scanner(yytext);
	DORET(YY_IGNORE_THIS)
}

^[Cc]{delim}*[^\r\n]* {
	// comment start with [Cc]
	int p = comment_parse(yytext);
	update_flex();
	DORET(p);
}
![^\r\n]* {
	// comment start with `!`
	int p = comment_parse(yytext);
	update_flex();
	DORET(p);
}

{newline}([ ][ ][ ][ ][ ])({digit}|[*&]) {
	// continuation line
	// {newline}{ws}\*
	int p = nop_parse();
	update_flex();
	DORET(p);
}
{newline} {
	int p = crlf_parse(yytext);
	update_flex();
	DORET(p);
}
{ws} {
	int p = nop_parse();
	update_flex();
	DORET(p);
}
<INITIAL>^{delim}*[0-9]+ {
	int p = label_parse(yytext);
	update_flex();
	DORET(p);
}
<label_end_scanner>^{delim}*[0-9]+ {
	int label_index;
	sscanf(yytext, "%d", &label_index);
	auto iter = get_context().end_labels.find(std::to_string(label_index));
	if (iter != get_context().end_labels.end()) {
		FlexLabelEndContext * fec;
		bool first = true;
		while (!iter->second.empty()) {
			fec = dynamic_cast<FlexLabelEndContext*> (get_current_scanner());
			iter->second.pop_back();
			quit_condition(YY_ENDDO, Term{ TokenMeta::RBrace, "enddo" });
			if (first) {
				first = false;
			}
			else {
				get_flex_context().terminal_cache.push_back(std::make_tuple(YY_ENDDO, Term{ TokenMeta::RBrace, std::string("enddo") }));
			}
			get_flex_context().terminal_cache.push_back(std::make_tuple(YY_CRLF, Term{ TokenMeta::CRLF, std::string("") }));
		}
		update_flex();
		return YY_ENDDO;
	}
	else {
		int p = label_parse(yytext);
		update_flex();
		DORET(p);
	}
}

{integral} { 
	int p = literal_parse(yytext, TokenMeta::META_INTEGER, YY_INTEGER);
	update_flex();
	DORET(p)
}
{floating} {
	int p = literal_parse(yytext, TokenMeta::META_REAL, YY_FLOAT);
	update_flex();
	DORET(p)
}
{narrow_floating} {
	int p = literal_parse(yytext, TokenMeta::META_REAL, YY_FLOAT);
	update_flex();
	DORET(p)
}
{floating}_[-+]?{floating} {
	int p = literal_parse(yytext, TokenMeta::META_COMPLEX, YY_COMPLEX);
	update_flex();
	DORET(p)
}
{string} {
	/* The delimiting quotes are escaped because they are Flex meta-characters. */
	int p = literal_parse(yytext, TokenMeta::META_STRING, YY_STRING);
	update_flex();
	DORET(p)
}

"(/" { int p = operator_parse(yytext, TokenMeta::META_ANY, YY_ARRAYBUILDER_START);  update_flex(); DORET(p) }
"/)" { int p = operator_parse(yytext, TokenMeta::META_ANY, YY_ARRAYBUILDER_END); update_flex(); DORET(p)  }
"::" { int p = operator_parse(yytext, TokenMeta::DoubleColon, YY_DOUBLECOLON); update_flex(); DORET(p) }
"**" { int p = operator_parse(yytext, TokenMeta::Power, YY_POWER);  update_flex(); DORET(p) }
">" { int p = operator_parse(yytext, TokenMeta::GT, YY_GT);  update_flex(); DORET(p)  }
">=" { int p = operator_parse(yytext, TokenMeta::GE, YY_GE);  update_flex(); DORET(p)  }
"<" { int p = operator_parse(yytext, TokenMeta::LT, YY_LT);  update_flex(); DORET(p)  }
"<=" { int p = operator_parse(yytext, TokenMeta::LE, YY_LE);  update_flex(); DORET(p) }
"==" { int p = operator_parse(yytext, TokenMeta::EQ, YY_EQ);  update_flex(); DORET(p) }
"/=" { int p = operator_parse(yytext, TokenMeta::NEQ, YY_NEQ);  update_flex(); DORET(p)  }
"//" { int p = operator_parse(yytext, TokenMeta::Add, '+');  update_flex(); DORET(p) /* string concat */}

{word} {
	/*do not call update_yylval in this macro because some tokens need to be looked ahead, such as `else if` */
	int p = word_parse(yytext);
	// std::cout << "-----------" << yytext << std::endl;
	update_flex(); // IMPORTANT, yyleng NOT always equal to `lowercase_name.size()` because of `pre_map`
	DORET(p)
}

. {
	/* all other things, including some operators not listed */
	int p = else_parse(yytext);
	update_flex();
	DORET(p)
}

%%

int else_parse(const char * origin_yytext) {
	update_yylval(Term{ TokenMeta::META_ILLEGAL, std::string(origin_yytext) });
	return origin_yytext[0];
}
int comment_parse(const char * origin_yytext) {
	/* do not update yylval */  
	get_flex_context().comments.push_back(std::string(yytext));
	return YY_IGNORE_THIS; /* do not return */
}
int label_parse(const char * origin_yytext) {
	// remove leading spaces
	int label_index;
	sscanf(origin_yytext, "%d", &label_index);
	update_yylval(Term{ TokenMeta::Label, std::to_string(label_index) });
	return YY_LABEL;
}
int literal_parse(const char * origin_yytext, TokenMeta_T token, int yytoken) {
	update_yylval(Term{ token, std::string(origin_yytext) });
	return yytoken;
}
int nop_parse() {
	/* do not update yylval */  
	return YY_IGNORE_THIS; /* do not return */
}

int crlf_parse(const char * origin_yytext) {
	update_yylval(Term{ TokenMeta::CRLF, std::string(origin_yytext) });
	return YY_CRLF;
}

int operator_parse(const char * origin_yytext, TokenMeta_T token, int yytoken) {
	update_yylval(Term{ token, std::string(origin_yytext) });  
	return yytoken;
}

int word_parse(const char * origin_yytext) {
	using namespace std;
	/* MUST NOT call update_flex at beginning because CutString */
	const FlexState & fs = get_flex_state();
	if (abandon) {
		// 跳过这个字符
		abandon = false;
		return YY_IGNORE_THIS;
	}
	string lowercase_name = string(origin_yytext);
	transform(lowercase_name.begin(), lowercase_name.end(), lowercase_name.begin(), tolower); 
FIND_CONCAT:
	if (lowercase_name == "format")
	{
		enter_condition(new FlexExternalContext("format", external_scanner));		
		return YY_IGNORE_THIS;
	}
	else if (lowercase_name == "do") {
		std::string next = CutString(get_context().global_code.begin() + fs.parse_pos + yyleng , get_context().global_code.end(), false);
		if (next[0] >= '0' && next[0] <= '9') {
			// label-do-stmt is [ do-construct-name : ] DO label [ loop-control ]
			get_context().end_labels[next].push_back("do");
			enter_condition(new FlexLabelEndContext("do", label_end_scanner));
			update_yylval(Term{ TokenMeta::Do, "do" });
			return YY_DO;
		}
	}
NORMAL_CONDITION:
	TokenMeta_T tok = TokenMeta::META_WORD;
	int yytok = YY_WORD;
	auto keyword_iter = find_if(keywords.begin(), keywords.end(), [&](const auto & x) {return x.what == lowercase_name; });
	if (keyword_iter != keywords.end()) {
		// if fortran keyword
		auto forward1_iter = find_if(forward1.begin(), forward1.end(), [&](const auto & x) {return x.first == lowercase_name; });
		if (forward1_iter != forward1.end()) {
			// not a valid yylval
			std::string next = CutString(get_context().global_code.begin() + fs.parse_pos + yyleng , get_context().global_code.end(), false);
			transform(next.begin(), next.end(), next.begin(), tolower);
			auto next_iter = find_if(forward1_iter->second.begin(), forward1_iter->second.end(), [&](const auto & x) {return x == next; });
			if (next_iter != forward1_iter->second.end()) {
				// concat two string
				lowercase_name += next;
				abandon = true; // ignore next
				goto FIND_CONCAT;
			}
		}
		tok = keyword_iter->token;
		yytok = keyword_iter->yytoken;
	}
	if (pre_map.find(lowercase_name) != pre_map.end()) {
		// replace according to tokenizer.cpp
		lowercase_name = pre_map.at(lowercase_name);
	}
	update_yylval(Term{ tok, std::string(lowercase_name) });
	return yytok;
}

void set_buff(const std::string & code) {
	using namespace std;
	int slen = (int)code.size();
	get_flex_state().parse_line = 0;
	get_flex_state().parse_pos = 0;
	get_flex_state().parse_len = 0;
	get_flex_state().line_pos = 0; 
	if (iter_buff != nullptr) {
		free(iter_buff); iter_buff = nullptr;
	}
	iter_buff = (char *)malloc(slen + 2);
	// must have 2 '\0's at the end of the string
	memset(iter_buff, 0, slen + 2);
	strcpy(iter_buff, code.c_str());
	yy_buffer = yy_scan_buffer(iter_buff, slen + 2);
}
void release_buff() {
	yy_delete_buffer(yy_buffer);
	free(iter_buff); iter_buff = nullptr;
}

int yywrap()
{
	return(1);
}
