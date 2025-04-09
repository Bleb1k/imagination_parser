#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Dynamic array macros
#define DA_INIT_CAP 8
#define da_append(da, item)                                                    \
  do {                                                                         \
    if ((da)->len >= (da)->cap) {                                              \
      (da)->cap = (da)->cap ? (da)->cap * 2 : DA_INIT_CAP;                     \
      (da)->items = realloc((da)->items, (da)->cap * sizeof(*(da)->items));    \
    }                                                                          \
    (da)->items[(da)->len++] = (item);                                         \
  } while (0)

typedef enum {
  TOKEN_NAME,
  TOKEN_TYPE,
  TOKEN_COLON,
  TOKEN_LPAREN,
  TOKEN_RPAREN,
  TOKEN_ARROW,
  TOKEN_LBRACE,
  TOKEN_RBRACE,
  TOKEN_COMMA,
  TOKEN_VALUE,
  TOKEN_COMMENT,
  TOKEN_EOF
} TokenType;

typedef struct {
  TokenType type;
  char lexeme[100];
} Token;

typedef struct {
  char *name;
  char *type;
} Parameter;

typedef struct {
  Parameter *items;
  size_t len;
  size_t cap;
} ParameterArray;

typedef struct {
  char *name;
  char *type;
  char *value;
} VarDecl;

typedef struct {
  VarDecl *items;
  size_t len;
  size_t cap;
} VarDeclArray;

typedef struct {
  char *name;
  char *type;
  char *returnType;
  ParameterArray params;
  VarDeclArray vars;
  char *comment;
} FunctionDecl;

typedef struct Parser {
  Token current_token;
  char *input;
} Parser;

const char *token_type_to_string(TokenType type) {
  switch (type) {
  case TOKEN_NAME:
    return "NAME";
  case TOKEN_TYPE:
    return "TYPE";
  case TOKEN_COLON:
    return "COLON";
  case TOKEN_LPAREN:
    return "LPAREN";
  case TOKEN_RPAREN:
    return "RPAREN";
  case TOKEN_ARROW:
    return "ARROW";
  case TOKEN_LBRACE:
    return "LBRACE";
  case TOKEN_RBRACE:
    return "RBRACE";
  case TOKEN_COMMA:
    return "COMMA";
  case TOKEN_VALUE:
    return "VALUE";
  case TOKEN_COMMENT:
    return "COMMENT";
  case TOKEN_EOF:
    return "EOF";
  default:
    return "UNKNOWN";
  }
}

void print_token_tree(int depth, Token token) {
  for (int i = 0; i < depth; i++) {
    printf("  ");
  }
  printf("%s", token_type_to_string(token.type));
  if (strlen(token.lexeme) > 0) {
    printf(" (%s)", token.lexeme);
  }
  printf("\n");
}

void next_token(Parser *parser) {
  while (isspace(*parser->input))
    parser->input++;
  if (!*parser->input) {
    parser->current_token.type = TOKEN_EOF;
    return;
  }
  if (isalpha(*parser->input)) {
    int i = 0;
    while (isalnum(*parser->input) || *parser->input == '_')
      parser->current_token.lexeme[i++] = *parser->input++;
    parser->current_token.lexeme[i] = '\0';
    parser->current_token.type = (parser->current_token.lexeme[0] >= 'A' &&
                                  parser->current_token.lexeme[0] <= 'Z')
                                     ? TOKEN_TYPE
                                     : TOKEN_NAME;
    return;
  }
  switch (*parser->input) {
  case ':':
    parser->current_token.type = TOKEN_COLON;
    parser->current_token.lexeme[0] = ':';
    parser->current_token.lexeme[1] = '\0';
    parser->input++;
    break;
  case '(':
    parser->current_token.type = TOKEN_LPAREN;
    parser->current_token.lexeme[0] = '(';
    parser->current_token.lexeme[1] = '\0';
    parser->input++;
    break;
  case ')':
    parser->current_token.type = TOKEN_RPAREN;
    parser->current_token.lexeme[0] = ')';
    parser->current_token.lexeme[1] = '\0';
    parser->input++;
    break;
  case '-':
    if (parser->input[1] == '>') {
      parser->current_token.type = TOKEN_ARROW;
      parser->current_token.lexeme[0] = '-';
      parser->current_token.lexeme[1] = '>';
      parser->current_token.lexeme[2] = '\0';
      parser->input += 2;
    }
    break;
  case '{':
    parser->current_token.type = TOKEN_LBRACE;
    parser->current_token.lexeme[0] = '{';
    parser->current_token.lexeme[1] = '\0';
    parser->input++;
    break;
  case '}':
    parser->current_token.type = TOKEN_RBRACE;
    parser->current_token.lexeme[0] = '}';
    parser->current_token.lexeme[1] = '\0';
    parser->input++;
    break;
  case ',':
    parser->current_token.type = TOKEN_COMMA;
    parser->current_token.lexeme[0] = ',';
    parser->current_token.lexeme[1] = '\0';
    parser->input++;
    break;
  case '<':
    parser->input++;
    int i = 0;
    while (*parser->input != '>')
      parser->current_token.lexeme[i++] = *parser->input++;
    parser->current_token.lexeme[i] = '\0';
    parser->current_token.type = TOKEN_VALUE;
    parser->input++;
    break;
  case '/':
    if (parser->input[1] == '/') {
      parser->current_token.type = TOKEN_COMMENT;
      int i = 0;
      parser->input += 2;
      while (*parser->input && *parser->input != '\n')
        parser->current_token.lexeme[i++] = *parser->input++;
      parser->current_token.lexeme[i] = '\0';
    }
    break;
  default:
    printf("Unexpected character: %c\n", *parser->input);
    exit(1);
  }
}

Token consume(Parser *parser, TokenType expected) {
  Token current = parser->current_token;
  if (current.type == expected) {
    print_token_tree(0, current);
    next_token(parser);
    return current;
  } else {
    printf("Error: expected %d, got %d\n", expected, current.type);
    exit(1);
  }
}

VarDecl parse_var_decl(Parser *parser) {
  printf("INFO: Parsing variable declaration\n");
  VarDecl var;
  Token name = consume(parser, TOKEN_NAME);
  consume(parser, TOKEN_COLON);
  Token type = consume(parser, TOKEN_TYPE);
  consume(parser, TOKEN_COLON);
  Token value = consume(parser, TOKEN_VALUE);

  var.name = strdup(name.lexeme);
  var.type = strdup(type.lexeme);
  var.value = strdup(value.lexeme);
  return var;
}

VarDeclArray parse_var_decls(Parser *parser) {
  printf("INFO: Parsing variable declarations\n");
  VarDeclArray vars = {0};
  while (parser->current_token.type == TOKEN_NAME) {
    VarDecl var = parse_var_decl(parser);
    da_append(&vars, var);
  }
  return vars;
}

Parameter parse_param(Parser *parser) {
  printf("INFO: Parsing parameter\n");
  Parameter param;
  Token name = consume(parser, TOKEN_NAME);
  consume(parser, TOKEN_COLON);
  Token type = consume(parser, TOKEN_TYPE);

  param.name = strdup(name.lexeme);
  param.type = strdup(type.lexeme);
  return param;
}

ParameterArray parse_params(Parser *parser) {
  printf("INFO: Parsing parameters\n");
  ParameterArray params = {0};
  if (parser->current_token.type == TOKEN_NAME) {
    Parameter param = parse_param(parser);
    da_append(&params, param);
    while (parser->current_token.type == TOKEN_COMMA) {
      consume(parser, TOKEN_COMMA);
      param = parse_param(parser);
      da_append(&params, param);
    }
  }
  return params;
}

FunctionDecl *parse_function_decl(Parser *parser) {
  printf("INFO: Parsing function declaration\n");
  FunctionDecl *func = malloc(sizeof(FunctionDecl));

  Token name = consume(parser, TOKEN_NAME);
  consume(parser, TOKEN_COLON);
  Token type = consume(parser, TOKEN_TYPE);
  consume(parser, TOKEN_COLON);
  consume(parser, TOKEN_LPAREN);
  func->params = parse_params(parser);
  consume(parser, TOKEN_RPAREN);
  consume(parser, TOKEN_ARROW);
  Token returnType = consume(parser, TOKEN_TYPE);
  consume(parser, TOKEN_LBRACE);
  func->vars = parse_var_decls(parser);
  consume(parser, TOKEN_RBRACE);

  func->name = strdup(name.lexeme);
  func->type = strdup(type.lexeme);
  func->returnType = strdup(returnType.lexeme);
  func->comment = NULL;

  if (parser->current_token.type == TOKEN_COMMENT) {
    Token comment = consume(parser, TOKEN_COMMENT);
    func->comment = strdup(comment.lexeme);
  }

  return func;
}

int main() {
  Parser parser;
  parser.input =
      "funcName: FuncType : (param1: Int, param2: String) -> Bool { var1: "
      "Int: <42> } // comment";
  next_token(&parser);
  FunctionDecl *ast = parse_function_decl(&parser);
  printf("Recursive descent parsing successful.\n");

  // Free AST here
  free(ast->name);
  free(ast->type);
  free(ast->returnType);
  if (ast->comment)
    free(ast->comment);

  for (size_t i = 0; i < ast->params.len; i++) {
    free(ast->params.items[i].name);
    free(ast->params.items[i].type);
  }
  free(ast->params.items);

  for (size_t i = 0; i < ast->vars.len; i++) {
    free(ast->vars.items[i].name);
    free(ast->vars.items[i].type);
    free(ast->vars.items[i].value);
  }
  free(ast->vars.items);

  free(ast);
  return 0;
}
