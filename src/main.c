#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOB_IMPLEMENTATION
#include <nob.h>
#define FLAG_IMPLEMENTATION
#include <flag.h>
#define HM_IMPLEMENTATION
#include <hm.h>

#include "./macro_witchery.h"

// TODO: Use string_view for tokens and nodes properly
// struct change is in place already, just fix that

/* Tokenizer helper function */
int is_str_here(const char *src, const char *needle) {
  size_t len = strlen(needle) - 1;
  for (size_t i = 0; i < len; i++) {
    if (src[i] != needle[i]) {
      return 0;
    }
  }
  return 1;
}

typedef enum TokenType {
  // ----- special -----
  TOKEN_NULL,
  TOKEN_EOF,
  // ----- multi-character -----
  TOKEN_IDENTIFIER,
  TOKEN_ARROW,
  TOKEN_ARROW_FAT,
  TOKEN_NUMBER,
  TOKEN_STRING,
  TOKEN_TRUE,
  TOKEN_FALSE,
  TOKEN_COMMENT, // unused
  // ----- single-character -----
  TOKEN_CHAR,
  TOKEN_SYMBOL,
} TokenType;
#define TOKEN_VALUE                                                            \
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER, TOKEN_TRUE, TOKEN_FALSE

const char *token_type_to_string(TokenType type) {
  switch (type) {
  case TOKEN_NULL:
    return "null";
  case TOKEN_EOF:
    return "eof";
  case TOKEN_IDENTIFIER:
    return "identifier";
  case TOKEN_ARROW:
    return "arrow";
  case TOKEN_ARROW_FAT:
    return "fat_arrow";
  case TOKEN_NUMBER:
    return "number";
  case TOKEN_STRING:
    return "string";
  case TOKEN_TRUE:
    return "true";
  case TOKEN_FALSE:
    return "false";
  case TOKEN_COMMENT:
    return "comment";
  case TOKEN_CHAR:
    return "char";
  case TOKEN_SYMBOL:
    return "symbol";
  default:
    return "unknown";
  }
}

typedef struct {
  Nob_String_View lexeme;
  TokenType type;
  int line;
} Token;

typedef struct {
  Nob_String_View input;
  char *source_file_path;
  Token current_token;
  int pos;
  int last_newline_pos;
  int line_count;
} Lexer;

void lexer_next_token(Lexer *lexer) {
#define RETURN_TOKEN(TOK, VAL_FROM, VAL_TO)                                    \
  do {                                                                         \
    lexer->current_token =                                                     \
        (Token){.lexeme = {0}, .type = TOKEN_NULL, .line = 0};                 \
    /*nob_log(NOB_INFO, "Parsed token '%s' (%s)", (char *)(VAL),               \
            token_type_to_string((TOK)));*/                                    \
    lexer->pos = cur_char - lexer->input.data;                                 \
  } while (0)

  const char *cur_char = lexer->input.data + lexer->pos;
  while (*cur_char == ' ' || *cur_char == '\t' || *cur_char == '\n') {
    if (*cur_char == '\n') {
      lexer->last_newline_pos = cur_char - lexer->input.data;
      lexer->line_count++;
    }
    cur_char++;
  }

  if (*cur_char == '\0') {
    RETURN_TOKEN(TOKEN_EOF, NULL);
    return;
  }

  // Handle comments
  if (*cur_char == '/' && *(cur_char + 1) == '/') {
    cur_char += 2;
    while (*cur_char != '\0' && *cur_char != '\n') {
      cur_char++;
    }
    lexer->pos = cur_char - lexer->input.data;
    return lexer_next_token(lexer);
  }

  // Handle identifiers
  if (isalpha(*cur_char)) {
    Nob_String_Builder sb = {0};
    char *start = (char *)cur_char;

    while (isalnum(*cur_char) || *cur_char == '_')
      cur_char++;

    nob_da_append_many(&sb, start, (size_t)(cur_char - start));
    char *lexeme = (char *)nob_temp_sv_to_cstr(nob_sb_to_sv(sb));

    TokenType type = TOKEN_IDENTIFIER;
    if (strcmp(lexeme, "true") == 0)
      type = TOKEN_TRUE;
    if (strcmp(lexeme, "false") == 0)
      type = TOKEN_FALSE;

    RETURN_TOKEN(type, lexeme);
    return;
  }

  // Handle strings
  if (*cur_char == '"') {
    Nob_String_Builder sb = {0};
    char *start = (char *)++cur_char;

    while (*cur_char != '"')
      cur_char++;

    nob_da_append_many(&sb, start, (size_t)(cur_char - start));
    char *lexeme = (char *)nob_temp_sv_to_cstr(nob_sb_to_sv(sb));
    TokenType type = TOKEN_STRING;

    cur_char++;

    RETURN_TOKEN(type, lexeme);
    return;
  }

  // Handle numbers
  if (isdigit(*cur_char)) {
    Nob_String_Builder sb = {0};
    char *start = (char *)cur_char;

    bool previous_char_is_underscode = false;
    bool is_floating = false;
    for (;;) {
      if (*cur_char == '_') {
        if (previous_char_is_underscode) {
          nob_log(NOB_ERROR, "Number can't contain two or more consecutive "
                             "underscores.");
          break;
        }
        previous_char_is_underscode = true;
        cur_char++;
        continue;
      }
      previous_char_is_underscode = false;

      if (*cur_char == '.') {
        if (is_floating) {
          nob_log(NOB_ERROR, "Number can't contain two or more periods.");
          break;
        }
        is_floating = true;
        cur_char++;
        continue;
      }

      if (!isdigit(*cur_char))
        break;
      cur_char++;
    }

    nob_da_append_many(&sb, start, cur_char - start);

    char *lexeme = (char *)nob_temp_sv_to_cstr(nob_sb_to_sv(sb));
    TokenType type = TOKEN_NUMBER;

    RETURN_TOKEN(type, lexeme);
    return;
  }

  if (is_str_here(cur_char, "->")) {
    cur_char += 2;
    RETURN_TOKEN(TOKEN_ARROW, "->");
    return;
  }

  if (is_str_here(cur_char, "=>")) {
    cur_char += 2;
    RETURN_TOKEN(TOKEN_ARROW_FAT, "=>");
    return;
  }

  if (*cur_char == '\'') {
    cur_char++;
    char c = *(cur_char++);
    if (*cur_char != '\'') {
      nob_log(NOB_ERROR, "Invalid character declaration\n");
      lexer->pos = cur_char - lexer->input.data;
      return lexer_next_token(lexer);
    }
    cur_char++;
    char *lexeme = nob_temp_alloc(2);
    lexeme[0] = c;
    lexeme[1] = 0;
    RETURN_TOKEN(TOKEN_CHAR, lexeme);
    return;
  }

  char *lexeme = nob_temp_alloc(2);
  lexeme[0] = *(cur_char++);
  lexeme[1] = 0;
  RETURN_TOKEN(TOKEN_SYMBOL, lexeme);
  return;
#undef RETURN_TOKEN
}

/*
// 1. variables
one: int: 1
two: float: 0.2
three: string: "III"
yes: bool: true
no: bool: false

// 2. variables with type assumptions
four :: 4
five :: 0.5
six :: "VI"
is_it :: false
*/

/*
comments are handled by "lexer_next_noken"
for now I can only have assignments
*/

uint32_t choice_depth = 0;

typedef enum NodeType {
  NODE_NULL,
  NODE_PROGRAM,
  NODE_STATEMENT,
  NODE_FUNCTION_CALL,
  NODE_ASSIGNMENT,
  NODE_IMPLIED_ASSIGNMENT,
  NODE_EXPRESSION_ASSIGNMENT,
  NODE_VALUE_ASSIGNMENT,
  NODE_TYPE_ASSIGNMENT,
  NODE_EXPRESSION,
  NODE_IDENTIFIER,
  NODE_VALUE,
  NODE_TYPE,
} NodeType;

typedef struct AstNode {
  Nob_String_View source;
  void *node;
  NodeType type;
  int line;
  int col;
} AstNode;

typedef struct ManyElementNode {
  AstNode *items;
  size_t count;
  size_t capacity;
} Program;

typedef struct SingleElementNode {
  AstNode item;
} Statement, Assignment, FunctionCall, Expression, Type;

typedef struct ImpliedAssignment {
  AstNode left;
  AstNode right;
} ImpliedAssignment, ExpressionAssignment, ValueAssignment, TypeAssignment;

typedef struct Identifier {
  const char *str;
} Identifier;
AstNode parse_literal(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing literal");
  NOB_UNUSED(lexer);
  NOB_TODO("Implement parse_literal.");
}

AstNode parse_type_function_call(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing type_function_call");
  NOB_UNUSED(lexer);
  NOB_TODO("Implement parse_type_function_call.");
}

AstNode parse_function_type(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing function_type");
  NOB_UNUSED(lexer);
  NOB_TODO("Implement parse_function_type.");
}

AstNode parse_code_block(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing code_block");
  NOB_UNUSED(lexer);
  NOB_TODO("Implement parse_code_block.");
}

AstNode parse_function_definition(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing function_definition");
  NOB_UNUSED(lexer);
  NOB_TODO("Implement parse_function_definition.");
}

AstNode parse_identifier(Lexer *lexer);
AstNode parse_type(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing type");
  AstNode expression = new_ast_node(Expression, lexer);

  AstNode identifier = {0};
  parse_(lexer, identifier, identifier, function_type, type_function_call,
         literal);
  ok_or_return(identifier.type, expression);
  ((Type *)expression.node)->item = identifier;

  AstNode node = compose_ast_node(lexer, expression, NODE_EXPRESSION);
  nob_log(NOB_INFO, "Parsed type: `%.*s`", (int)node.source.count,
          node.source.items);
  return node;
}

AstNode parse_value(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing value");
  NOB_UNUSED(lexer);
  NOB_TODO("Implement parse_value.");
}

AstNode parse_identifier(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing identifier");
  AstNode identifier = new_ast_node(Identifier, lexer);

  ((Identifier *)identifier.node)->str = lexer->current_token.lexeme;
  consume(lexer, TOKEN_IDENTIFIER);

  AstNode node = compose_ast_node(lexer, identifier, NODE_IDENTIFIER);
  nob_log(NOB_INFO, "Parsed identifier: `%.*s`", (int)node.source.count,
          node.source.items);
  return node;
}

AstNode parse_expression(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing expression");
  AstNode expression = new_ast_node(Expression, lexer);

  AstNode identifier = {0};
  parse_(lexer, identifier, type, value, function_definition, code_block);
  ok_or_return(identifier.type, expression);
  ((Expression *)expression.node)->item = identifier;

  AstNode node = compose_ast_node(lexer, expression, NODE_EXPRESSION);
  nob_log(NOB_INFO, "Parsed expression: `%.*s`", (int)node.source.count,
          node.source.items);
  return node;
}

AstNode parse_implied_assignment(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing implied_assignment");
  AstNode implied_assignment = new_ast_node(ImpliedAssignment, lexer);

  AstNode identifier = parse_identifier(lexer);
  ok_or_return(identifier.type, implied_assignment);
  ((ImpliedAssignment *)implied_assignment.node)->left = identifier;

  int pos = lexer->pos;
  consume_str(lexer, "::", ":=");
  ok_or_return(pos != lexer->pos, implied_assignment);

  AstNode expression = parse_expression(lexer);
  ok_or_return(expression.type, implied_assignment);
  ((ImpliedAssignment *)implied_assignment.node)->right = expression;

  AstNode node =
      compose_ast_node(lexer, implied_assignment, NODE_IMPLIED_ASSIGNMENT);
  nob_log(NOB_INFO, "Parsed implied assignment: `%.*s`", (int)node.source.count,
          node.source.items);
  return node;
}

AstNode parse_type_assignment(Lexer *lexer);
AstNode parse_expression_assignment(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing expression_assignment");
  AstNode expression_assignment = new_ast_node(ExpressionAssignment, lexer);

  AstNode identifier = {0};
  parse_(lexer, identifier, identifier, type_assignment);
  ok_or_return(identifier.type, expression_assignment);
  ((ImpliedAssignment *)expression_assignment.node)->left = identifier;

  int pos = lexer->pos;
  consume_str(lexer, ":", "=");
  ok_or_return(pos != lexer->pos, expression_assignment);

  AstNode expression = parse_expression(lexer);
  ok_or_return(expression.type, expression_assignment);
  ((ImpliedAssignment *)expression_assignment.node)->right = expression;

  AstNode node = compose_ast_node(lexer, expression_assignment,
                                  NODE_EXPRESSION_ASSIGNMENT);
  nob_log(NOB_INFO, "Parsed expression assignment: `%.*s`",
          (int)node.source.count, node.source.items);
  return node;
}

AstNode parse_value_assignment(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing value_assignment");
  AstNode value_assignment = new_ast_node(ValueAssignment, lexer);

  AstNode identifier = parse_identifier(lexer);
  ok_or_return(identifier.type, value_assignment);
  ((ImpliedAssignment *)value_assignment.node)->left = identifier;

  int pos = lexer->pos;
  consume_str(lexer, "=");
  ok_or_return(pos != lexer->pos, value_assignment);

  AstNode value = parse_value(lexer);
  ok_or_return(value.type, value_assignment);
  ((ImpliedAssignment *)value_assignment.node)->right = value;

  AstNode node =
      compose_ast_node(lexer, value_assignment, NODE_VALUE_ASSIGNMENT);
  nob_log(NOB_INFO, "Parsed value assignment: `%.*s`", (int)node.source.count,
          node.source.items);
  return node;
}

AstNode parse_type_assignment(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing type_assignment");
  AstNode type_assignment = new_ast_node(TypeAssignment, lexer);

  AstNode identifier = parse_identifier(lexer);
  ok_or_return(identifier.type, type_assignment);
  ((ImpliedAssignment *)type_assignment.node)->left = identifier;

  int pos = lexer->pos;
  consume_str(lexer, "=");
  ok_or_return(pos != lexer->pos, type_assignment);

  AstNode type = parse_type(lexer);
  ok_or_return(type.type, type_assignment);
  ((ImpliedAssignment *)type_assignment.node)->right = type;

  AstNode node = compose_ast_node(lexer, type_assignment, NODE_TYPE_ASSIGNMENT);
  nob_log(NOB_INFO, "Parsed type assignment: `%.*s`", (int)node.source.count,
          node.source.items);
  return node;
}

AstNode parse_assignment(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing assignment");
  AstNode assignment = new_ast_node(Assignment, lexer);
  parse_(lexer, assignment, implied_assignment, expression_assignment,
         value_assignment, type_assignment);
  ok_or_return(assignment.type, assignment);
  AstNode node = compose_ast_node(lexer, assignment, NODE_ASSIGNMENT);
  nob_log(NOB_INFO, "Parsed assignment: `%.*s`", (int)node.source.count,
          node.source.items);
  return node;
}

AstNode parse_function_call(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing function_call");
  NOB_UNUSED(lexer);
  NOB_TODO("Implement parse_function_call.");
}

AstNode parse_statement(Lexer *lexer) {
  nob_log(NOB_INFO, "Parsing statement");
  AstNode statement = new_ast_node(Statement, lexer);
  parse_(lexer, statement, assignment, function_call);
  ok_or_return(statement.type, statement);
  AstNode node = compose_ast_node(lexer, statement, NODE_STATEMENT);
  nob_log(NOB_INFO, "Parsed statement: `%.*s`", (int)node.source.count,
          node.source.items);
  return node;
}

AstNode parse_program(Lexer *lexer, AstNode program) {
  nob_log(NOB_INFO, "Parsing program");
  nob_da_append((Program *)program.node, parse_statement(lexer));

  if (lexer->current_token.type == TOKEN_EOF) {
    AstNode node = compose_ast_node(lexer, program, NODE_PROGRAM);
    nob_log(NOB_INFO, "Parsed program: `%.*s`", (int)node.source.count,
            node.source.items);
    return node;
  } else
    return parse_program(lexer, program);
}

AstNode parse_source_file(Lexer lexer) {
  nob_log(NOB_INFO, "Parsing source_file");
  lexer_next_token(&lexer);

  AstNode node = parse_program(&lexer, new_ast_node(Program, &lexer));
  nob_log(NOB_INFO, "Parsed source file: `%.*s`", (int)node.source.count,
          node.source.items);
  return node;
}

void usage(FILE *stream) {
  fprintf(stream, "Usage: ./main [OPTIONS] <INPUT FILES...>\n");
  fprintf(stream, "OPTIONS:\n");
  flag_print_options(stream);
}

int main(int argc, char **argv) {
  bool *help =
      flag_bool("help", false, "Print this help to stdout and exit with 0");
  // char **output_path = flag_str("o", NULL, "Line to output to the file");

  if (!flag_parse(argc, argv)) {
    usage(stderr);
    flag_print_error(stderr);
    exit(1);
  }

  if (*help) {
    usage(stdout);
    exit(0);
  }

  int rest_argc = flag_rest_argc();
  char **rest_argv = flag_rest_argv();

  if (rest_argc <= 0) {
    usage(stderr);
    fprintf(stderr, "[ERROR] No input files are provided\n");
    exit(1);
  }

  Nob_String_Builder source_file = {0};
  argc -= 1;
  ok_or_return(nob_read_entire_file(*rest_argv, &source_file), 1);
  nob_sb_append_null(&source_file);
  Lexer lexer = {
      .input = nob_sb_to_sv(source_file),
      .source_file_path = *rest_argv,
      .pos = 0,
      .current_token = {0},
  };
  AstNode program = parse_source_file(lexer);
  nob_log(NOB_INFO, "Parsed root.");
  nob_da_foreach(AstNode, i, (Program *)program.node) {
    nob_log(NOB_INFO, "statement: %.*s", (int)i->source.count, i->source.items);
  }
  nob_sb_free(source_file);

  // nob_sb_append_null(&source_file);
  // printf("%s", source_file.items);

  return 69;
  // --- raw API ---
  HM hm = {0};
  {
    HM_init(&hm, sizeof(int), 0);

    int val = 2;
    HM_set(&hm, "test", &val);

    int *res = (int *)HM_get(&hm, "test");
    if (res != NULL) {
      printf("res: %d\n", *res);
    } else {
      printf("res: not found\n");
    }

    HM_remove(&hm, "test");
    assert(HM_get(&hm, "test") == NULL);

    HM_deinit(&hm);
  }
  return 0;
}
