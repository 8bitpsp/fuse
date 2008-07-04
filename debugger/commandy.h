/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     LOGICAL_OR = 258,
     LOGICAL_AND = 259,
     COMPARISON = 260,
     EQUALITY = 261,
     NEGATE = 262,
     TIMES_DIVIDE = 263,
     BASE = 264,
     BREAK = 265,
     TBREAK = 266,
     CLEAR = 267,
     CONDITION = 268,
     CONTINUE = 269,
     DEBUGGER_DELETE = 270,
     DISASSEMBLE = 271,
     FINISH = 272,
     IF = 273,
     DEBUGGER_IGNORE = 274,
     NEXT = 275,
     DEBUGGER_OUT = 276,
     PORT = 277,
     READ = 278,
     SET = 279,
     STEP = 280,
     TIME = 281,
     WRITE = 282,
     PAGE = 283,
     DEBUGGER_REGISTER = 284,
     NUMBER = 285,
     DEBUGGER_ERROR = 286
   };
#endif
/* Tokens.  */
#define LOGICAL_OR 258
#define LOGICAL_AND 259
#define COMPARISON 260
#define EQUALITY 261
#define NEGATE 262
#define TIMES_DIVIDE 263
#define BASE 264
#define BREAK 265
#define TBREAK 266
#define CLEAR 267
#define CONDITION 268
#define CONTINUE 269
#define DEBUGGER_DELETE 270
#define DISASSEMBLE 271
#define FINISH 272
#define IF 273
#define DEBUGGER_IGNORE 274
#define NEXT 275
#define DEBUGGER_OUT 276
#define PORT 277
#define READ 278
#define SET 279
#define STEP 280
#define TIME 281
#define WRITE 282
#define PAGE 283
#define DEBUGGER_REGISTER 284
#define NUMBER 285
#define DEBUGGER_ERROR 286




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 45 "commandy.y"
{

  int token;
  int reg;

  libspectrum_dword integer;
  debugger_breakpoint_type bptype;
  debugger_breakpoint_life bplife;
  struct { int value1; libspectrum_word value2; } pair;

  debugger_expression* exp;

}
/* Line 1489 of yacc.c.  */
#line 125 "commandy.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

