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

// get number of arguments with __NARG__
#define __NARG__(...) __NARG_I_(__VA_ARGS__, __RSEQ_N())
#define __NARG_I_(...) __ARG_N(__VA_ARGS__)
#define __ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14,   \
                _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26,    \
                _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38,    \
                _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,    \
                _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62,    \
                _63, N, ...)                                                   \
  N
#define __RSEQ_N()                                                             \
  63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45,  \
      44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27,  \
      26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9,   \
      8, 7, 6, 5, 4, 3, 2, 1, 0

#define _VFUNC_(name, n) name##n
#define _VFUNC(name, n) _VFUNC_(name, n)
#define VFUNC(func, ...) _VFUNC(func, __NARG__(__VA_ARGS__))(__VA_ARGS__)

#define one_of(val, ...) VFUNC(one_of_, val, __VA_ARGS__)
#define one_of_1(val) val
#define one_of_2(val, cmp) (val == cmp)
#define one_of_3(val, cmp, ...) (val == cmp) || one_of_2(val, __VA_ARGS__)
#define one_of_4(val, cmp, ...) (val == cmp) || one_of_3(val, __VA_ARGS__)
#define one_of_5(val, cmp, ...) (val == cmp) || one_of_4(val, __VA_ARGS__)
#define one_of_6(val, cmp, ...) (val == cmp) || one_of_5(val, __VA_ARGS__)
#define one_of_7(val, cmp, ...) (val == cmp) || one_of_6(val, __VA_ARGS__)
#define one_of_8(val, cmp, ...) (val == cmp) || one_of_7(val, __VA_ARGS__)
#define one_of_9(val, cmp, ...) (val == cmp) || one_of_8(val, __VA_ARGS__)

#define ok_or_return(bool_val, ret_val)                                        \
  do {                                                                         \
    if (!(bool_val)) {                                                         \
      printf("[ERROR] %s:%d: %s: Code is not okay\n", __FILE__, __LINE__,      \
             __FUNCTION__);                                                    \
      return (ret_val);                                                        \
    }                                                                          \
  } while (0)

typedef enum TokenType {
  // ----- special -----
  TOKEN_NULL, // hopefully unused
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
  TOKEN_COLON,
  TOKEN_SEMICOLON,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_COMMA,
  TOKEN_PERIOD,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_CHAR,
  TOKEN_ADD,
  TOKEN_SUB,
  TOKEN_DIV,
  TOKEN_MUL,
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
  case TOKEN_COLON:
    return "colon";
  case TOKEN_SEMICOLON:
    return "semicolon";
  case TOKEN_LPAREN:
    return "lparen";
  case TOKEN_RPAREN:
    return "rparen";
  case TOKEN_COMMA:
    return "comma";
  case TOKEN_PERIOD:
    return "period";
  case TOKEN_LBRACE:
    return "lbrace";
  case TOKEN_RBRACE:
    return "rbrace";
  case TOKEN_CHAR:
    return "char";
  case TOKEN_ADD:
    return "add";
  case TOKEN_SUB:
    return "subtract";
  case TOKEN_DIV:
    return "divide";
  case TOKEN_MUL:
    return "multiply";
  default:
    return "unknown";
  }
}

typedef struct {
  TokenType type;
  char *lexeme;
} Token;

typedef struct {
  Nob_String_View input;
  char *source_file_path;
  int pos;
  int last_newline_pos;
  int line_count;
  Token current_token;
} Lexer;

void lexer_next_token(Lexer *lexer) {
#define RETURN_TOKEN(TOK, VAL)                                                 \
  do {                                                                         \
    lexer->current_token = (Token){(TOK), (VAL)};                              \
    /*nob_log(NOB_INFO, "Parsed token '%s' (%s)", (char *)(VAL),*/             \
    /*token_type_to_string((TOK)));*/                                          \
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

  // Handle single-character tokens
  switch (*cur_char) {
  case ':':
    cur_char++;
    RETURN_TOKEN(TOKEN_COLON, ":");
    return;
  case '(':
    cur_char++;
    RETURN_TOKEN(TOKEN_LPAREN, "(");
    return;
  case ')':
    cur_char++;
    RETURN_TOKEN(TOKEN_RPAREN, ")");
    return;
  case ',':
    cur_char++;
    RETURN_TOKEN(TOKEN_COMMA, ",");
    return;
  case '.':
    cur_char++;
    RETURN_TOKEN(TOKEN_PERIOD, ".");
    return;
  case '{':
    cur_char++;
    RETURN_TOKEN(TOKEN_LBRACE, "{");
    return;
  case '}':
    cur_char++;
    RETURN_TOKEN(TOKEN_RBRACE, "}");
    return;
  case ';':
    cur_char++;
    RETURN_TOKEN(TOKEN_SEMICOLON, ";");
    return;
  case '\'': {
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
    lexeme[1] = '\0';
    RETURN_TOKEN(TOKEN_CHAR, lexeme);
    return;
  }
  case '+':
    cur_char++;
    RETURN_TOKEN(TOKEN_ADD, "+");
    return;
  case '-':
    if (*(cur_char + 1) == '>') {
      lexer->pos += 2;
      lexer->current_token = (Token){TOKEN_ARROW, "->"};
      return;
    }
    cur_char++;
    RETURN_TOKEN(TOKEN_SUB, "-");
    return;
  case '/':
    cur_char++;
    RETURN_TOKEN(TOKEN_DIV, "/");
    return;
  case '*':
    cur_char++;
    RETURN_TOKEN(TOKEN_DIV, "*");
    return;
  default:
    nob_log(NOB_ERROR, "Unrecognized token '%c'", *cur_char);
    lexer->pos = cur_char - lexer->input.data + 1;
    return lexer_next_token(lexer);
  }
#undef RETURN_TOKEN
}

#define PARSING_ERROR(...)                                                     \
  do {                                                                         \
    TokenType types[] = {__VA_ARGS__};                                         \
    size_t len = sizeof(types) / sizeof(TokenType);                            \
    fprintf(stderr,                                                            \
            "\n[ERROR] %s:%d:%d  (%s:%d)\nFound: '%s'\nExpected one of:\n",    \
            (lexer)->source_file_path, lexer->line_count,                      \
            (lexer)->pos - (lexer)->last_newline_pos, __FILE__, __LINE__,      \
            token_type_to_string((lexer)->current_token.type));                \
    for (size_t i = 0; i < len; i++) {                                         \
      fprintf(stderr, "  - '%s'\n", token_type_to_string(types[i]));           \
    }                                                                          \
    exit(123);                                                                 \
  } while (0)

#define consume_selection(lexer, ...)                                          \
  ({                                                                           \
    TokenType types[] = {__VA_ARGS__};                                         \
    size_t len = sizeof(types) / sizeof(TokenType);                            \
    Token found = {0};                                                         \
    for (size_t i = 0; i < len; i++) {                                         \
      if ((lexer)->current_token.type == types[i]) {                           \
        found = (lexer)->current_token;                                        \
        lexer_next_token(lexer);                                               \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    if (found.lexeme == NULL && found.type == TOKEN_NULL) {                    \
      PARSING_ERROR(__VA_ARGS__);                                              \
    }                                                                          \
  })

#define consume(lexer, ...) consume_selection(lexer, __VA_ARGS__)

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

typedef enum NodeType {
  NODE_ROOT,
  NODE_DECLARATION,
  NODE_BINOP_SUM_SUB,
  NODE_BINOP_MUL_DIV,
  NODE_LIT_IDENT,
  NODE_LIT_INT,
  NODE_LIT_STRING,
  NODE_LIT_BOOL,
  NODE_FUNC,
  NODE_FUNC_TYPE
} NodeType;

typedef struct AstNode {
  NodeType type;
  Nob_String_View source;
  void *node;
} AstNode;

typedef char *Literal;

typedef struct Function {
  AstNode type;
  AstNode body;
} Function;

typedef struct Binop {
  Token *items;
  size_t count;
  size_t capacity;
} Binop;
#define new_Binop(val) ((AstNode){NODE_BINOP, {0}, &(val)})

typedef struct Declaration {
  Token target;
  AstNode type;
  AstNode value;
} Declaration;
#define new_Declaration(val) ((AstNode){NODE_DECLARATION, {0}, &(val)})

typedef struct Root {
  AstNode *items;
  size_t count;
  size_t capacity;
} Root;
#define new_Root(val) ((AstNode){NODE_ROOT, {0}, &(val)})

AstNode parse_value(Lexer *lexer, Binop expr) {
  NOB_TODO("Implement parse_value");
  if (lexer->current_token.type == TOKEN_LPAREN) {
    // return parse_function(lexer, expr);
  }

  // return new_Binop(expr); TODO: fix
}

AstNode parse_type(Lexer *lexer) {
  NOB_UNUSED(lexer);
  NOB_TODO("Implement parse_type");
}

AstNode parse_declaration(Lexer *lexer) {
  Declaration decl = {0};
  decl.target = lexer->current_token;
  consume(lexer, TOKEN_IDENTIFIER);
  consume(lexer, TOKEN_COLON);
  if (lexer->current_token.type != TOKEN_COLON) {
    decl.type = parse_type(lexer);
    consume(lexer, TOKEN_IDENTIFIER);
  }
  consume(lexer, TOKEN_COLON);
  nob_log(NOB_INFO, "Parsed assignment '%s :%s: %s'", decl.target.lexeme,
          "<TODO: AstNode to string>" /*decl.type.lexeme*/,
          "<TODO: AstNode to string>" /*decl.expression.lexeme*/);
  return new_Declaration(decl);
}

AstNode parse_root(Lexer *lexer, Root root) {
  nob_da_append(&root, parse_declaration(lexer));

  if (lexer->current_token.type == TOKEN_EOF)
    return new_Root(root);
  else
    return parse_root(lexer, root);
}

AstNode parse_source_file(Lexer lexer) {
  lexer_next_token(&lexer);

  return parse_root(&lexer, (Root){0});
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
  AstNode root = parse_source_file(lexer);
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
