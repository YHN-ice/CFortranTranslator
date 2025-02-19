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

#pragma once

#include "../parser/tokenizer.h"
#include "../parser/parser.h"
#include "../parser/context.h"


#define FORTRAN_CONTINUATION_SPACE 5
struct SimplerContext {
	std::string code;
	int pos;
	std::vector <char> char_cache;
	std::vector <std::string> item_cache;
	/****************
	* newline_marker:
	* true if the previous token is CRLF
	* if `newline_marker` = `true`, current token is the first token of the current line
	****************/
	bool newline_marker = true; // initial is true not false
	/****************
	* label_border:
	* counts length to the end of label region
	* set to FORTRAN_CONTINUATION_SPACE + 1 at the beginning of every line
	* according to fortran 77 std, one line of source code is composed by
	* ```
	* lllllcxxxxxxxx
	* ```
	* in which `l` means label or comment marker [C/c]
	* `c` means continuation mark
	* `x` means normal codes
	* the translator is also compacted with new fortran standard, in which one line of source code can be composed by
	* ```
	* xxxxxxxx
	* ```
	****************/
	int label_border = FORTRAN_CONTINUATION_SPACE + 1;
	/****************
	* in_string_literal:
	* 0 if not wrapped in a string now
	* '\"' if wrapped by a '\"' string now
	* '\'' if wrapped by a '\'' string now
	****************/
	char in_string_literal = 0;
	bool in_format_stmt = false;
	void reset() {
		pos = 0;
	}
	SimplerContext(const std::string & _code, int p) : code(_code), pos(p) {

	}
};

SimplerContext & get_simpler_context();
void reset_simpler_context();

int simpler_yylex(void);
