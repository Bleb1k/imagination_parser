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

#define sized_enum(type, name)                                                 \
  typedef type name;                                                           \
  enum

// void *__malloc_log(void *ptr, size_t size, char *f, int l) {
//   fprintf(stderr, "%p: %s:%d: > %zu\n", ptr, f, l, size);
//   return ptr;
// }
// #define malloc(size_) __malloc_log(malloc((size_)), (size_), __FILE__,
// __LINE__)

// TODO: Use string_view for tokens and nodes properly
// struct change is in place already, just fix that

/* Tokenizer helper function */
int is_str_here(const char *src, const char *needle) {
  size_t len = strlen(needle);
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
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER, TOKEN_TRUE, TOKEN_FALSE,       \
      TOKEN_CHAR

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
  String_View lexeme;
  TokenType type;
  int line;
} Token;

typedef struct Lexer {
  String_View input;
  char *source_file_path;
  Token current_token;
  int pos;
  int last_newline_pos;
  int line_count;
} Lexer;

#define PARSER_ERROR_IMPLEMENTATION
#include "./macro_witchery.h"

void lexer_next_token(Lexer *lexer) {
#define RETURN_TOKEN(TOK, VAL_FROM, VAL_TO)                                    \
  do {                                                                         \
    lexer->current_token =                                                     \
        (Token){.lexeme = {.data = VAL_FROM, .count = VAL_TO - VAL_FROM},      \
                .type = TOK,                                                   \
                .line = lexer->line_count};                                    \
    lexer->pos = VAL_TO - lexer->input.data;                                   \
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
    RETURN_TOKEN(TOKEN_EOF, cur_char, cur_char);
    return;
  }

  // Handle comments
  if (*cur_char == '/') {
    const char *start = cur_char;

    if (*(cur_char + 1) == '/') {
      cur_char += 2;
      while (*cur_char != '\0' && *cur_char != '\n')
        cur_char++;
      RETURN_TOKEN(TOKEN_COMMENT, start, cur_char);
      return;
      // lexer->pos = cur_char - lexer->input.data;
      // return lexer_next_token(lexer);
    } else if (*(cur_char + 1) == '*') {
      cur_char += 3;
      int depth = 1;
      for (; depth; cur_char++) {
        if (*cur_char == '/') {
          if (*(cur_char - 1) == '*')
            depth -= 1;
          else if (*(cur_char + 1) == '*')
            depth += 1;
        }
      }
      RETURN_TOKEN(TOKEN_COMMENT, start, cur_char);
      return;
    }
  }

  // Handle identifiers
  if (*cur_char == '_' || isalpha(*cur_char)) {
    // String_Builder sb = {0};
    const char *start = cur_char;

    while (isalnum(*cur_char) || *cur_char == '_')
      cur_char++;

    // da_append_many(&sb, start, (size_t)(cur_char - start));
    // char *lexeme = (char *)temp_sv_to_cstr(sb_to_sv(sb));

    TokenType type = TOKEN_IDENTIFIER;
    if (is_str_here(cur_char, "true"))
      type = TOKEN_TRUE;
    if (is_str_here(cur_char, "false"))
      type = TOKEN_FALSE;

    RETURN_TOKEN(type, start, cur_char);
    return;
  }

  // Handle strings
  if (*cur_char == '"') {
    // String_Builder sb = {0};
    const char *start = (char *)++cur_char;

    while (*cur_char != '"')
      cur_char++;

    // da_append_many(&sb, start, (size_t)(cur_char - start));
    // char *lexeme = (char *)temp_sv_to_cstr(sb_to_sv(sb));
    TokenType type = TOKEN_STRING;

    RETURN_TOKEN(type, start, cur_char++);
    return;
  }

  // Handle numbers
  if ((*cur_char == '.' && isdigit(cur_char[1])) || isdigit(*cur_char)) {
    // String_Builder sb = {0};
    char *start = (char *)cur_char;

    bool previous_char_is_underscode = false;
    bool is_floating = false;
    for (;;) {
      if (*cur_char == '_') {
        if (previous_char_is_underscode) {
          nob_log(ERROR, "Number can't contain two or more consecutive "
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
          nob_log(ERROR, "Number can't contain two or more periods.");
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

    if (!is_floating) {
      if (*cur_char == 'u' || *cur_char == 'U')
        cur_char++;
      if (*cur_char == 'l' || *cur_char == 'L')
        cur_char++;
      if (*cur_char == 'l' || *cur_char == 'L')
        cur_char++;
    } else {
      // exponent
      if (*cur_char == 'e' || *cur_char == 'E') {
        cur_char++;
        if (*cur_char == '+' || *cur_char == '-')
          cur_char++;
        while (isdigit(*cur_char))
          cur_char++;
      } else if (*cur_char == 'f' || *cur_char == 'F' || *cur_char == 'l' ||
                 *cur_char == 'L') {
        cur_char++;
      }
    }
    if (*cur_char == 'i')
      cur_char++;

    // da_append_many(&sb, start, cur_char - start);

    // char *lexeme = (char *)temp_sv_to_cstr(sb_to_sv(sb));
    TokenType type = TOKEN_NUMBER;

    RETURN_TOKEN(type, start, cur_char);
    return;
  }

  if (*cur_char == '\'') {
    cur_char++;
    const char *start = cur_char++;
    if (*cur_char != '\'') {
      nob_log(ERROR, "Invalid character declaration\n");
      lexer->pos = cur_char - lexer->input.data;
      return lexer_next_token(lexer);
    }
    cur_char++;
    // char *lexeme = temp_alloc(2);
    // lexeme[0] = c;
    // lexeme[1] = 0;
    RETURN_TOKEN(TOKEN_CHAR, start, (start + 1));
    return;
  }

  if (is_str_here(cur_char, "->")) {
    RETURN_TOKEN(TOKEN_ARROW, cur_char, cur_char + 2);
    return;
  }

  if (is_str_here(cur_char, "=>")) {
    // cur_char += 2;
    RETURN_TOKEN(TOKEN_ARROW_FAT, cur_char, cur_char + 2);
    return;
  }

  RETURN_TOKEN(TOKEN_SYMBOL, cur_char, cur_char + 1);
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

#define NODE_TYPES                                                             \
  X(PROGRAM)                                                                   \
  X(COMMENT)                                                                   \
  X(STATEMENT)                                                                 \
  X(ASSIGNMENT)                                                                \
  X(FULL_ASSIGNMENT)                                                           \
  X(IMPLIED_ASSIGNMENT)                                                        \
  X(VALUE_ASSIGNMENT)                                                          \
  X(TYPE_OR_CONST_ASSIGNMENT)                                                  \
  X(TYPE_ASSIGNMENT)                                                           \
  X(CONST_ASSIGNMENT)                                                          \
  X(EXPRESSION)                                                                \
  X(BINARY_EXPRESSION)                                                         \
  X(TYPE)                                                                      \
  X(MULTIWORD_TYPE)                                                                      \
  X(VALUE)                                                                     \
  X(IDENTIFIER)

typedef enum NodeType {
#define X(v) NODE_##v,
  NODE_TYPES
#undef X
} NodeType;

const char *node_type_to_string(NodeType type) {
  switch (type) {
#define X(v)                                                                   \
  case NODE_##v:                                                               \
    return #v;
    NODE_TYPES
#undef X
  }
}

typedef struct AstNode {
  String_View source;
  void *node;
  NodeType type;
  int line;
  int col;
} AstNode;

AstNode *__compose_ast_node(AstNode node, int source_count) {
  AstNode *result = malloc(sizeof(AstNode));
  node.source.count = source_count;
  *result = node;
  return result;
}
#define compose_ast_node(lexer, _node)                                         \
  __compose_ast_node((_node), (int)((lexer)->current_token.lexeme.data -       \
                                    (_node).source.data))

typedef struct ValueNode {
  enum {
    VALUE_IDENTIFIER,
    VALUE_STRING,
    VALUE_NUMBER,
    VALUE_CHAR,
    VALUE_TRUE,
    VALUE_FALSE,
  } kind;
  union {
    String_View identifier, string, number, character, truth, falth;
  } as;
} Value;

typedef struct ProgramNode {
  AstNode *items;
  size_t count, capacity;
} Program;

typedef struct Statement {
  AstNode *item;
} Statement;

typedef struct Assignment {
  AstNode *item;
} Assignment;

typedef struct Expression {
  AstNode *item;
} Expression;

typedef struct Type {
  AstNode *item;
} Type;

#define BIN_EXPR_STRS "+", "-", "*", "/", "%"
sized_enum(char, BinaryExpressionKind){
    BIN_EXPR_ADD, BIN_EXPR_SUB, BIN_EXPR_MUL, BIN_EXPR_DIV, BIN_EXPR_MOD,
};

typedef struct BinaryExpression {
  AstNode *left;
  AstNode *right;
  BinaryExpressionKind kind;
} BinaryExpression;

typedef struct ImpliedAssignment {
  AstNode *left;
  AstNode *right;
} ImpliedAssignment;

typedef struct ValueAssignment {
  AstNode *left;
  AstNode *right;
} ValueAssignment;

typedef struct TypeOrConstAssignment {
  AstNode *left;
  AstNode *right;
} TypeOrConstAssignment;

typedef struct FullAssignmentNode {
  AstNode *target;
  AstNode *type;
  AstNode *value;
} FullAssignment;

typedef struct CommentNode {
  String_View str;
} Comment;

typedef struct IdentifierNode {
  String_View str;
} Identifier;

typedef struct MultiwordTypeNode {
  String_View str;
} MultiwordType;

const char *binary_expression_kind_to_string(BinaryExpressionKind kind) {
  switch (kind) {
  case BIN_EXPR_ADD:
    return "add";
  case BIN_EXPR_SUB:
    return "sub";
  case BIN_EXPR_MUL:
    return "mul";
  case BIN_EXPR_DIV:
    return "div";
  case BIN_EXPR_MOD:
    return "mod";
  default:
    UNREACHABLE(temp_sprintf("unknown BinaryExpressionKind: <%d>", kind));
  }
}

void padded_puts(const char *str, int pad) {
  while (*str != '\0') {
    putchar(*str);
    if (*str++ == '\n') {
      for (int i = 0; i < pad; i++) {
        putchar(' ');
      }
    }
  }
}

void print_node(AstNode ast) {
  static int space_size = 0;
  static bool on_line = false;
  bool was_on_line = on_line;
  int space_size_was = space_size;
#define padded(...)                                                            \
  printf("\n%*s", space_size, "");                                             \
  padded_puts(temp_sprintf(__VA_ARGS__), space_size)
#define pad(...)                                                               \
  if (!on_line)                                                                \
  padded(__VA_ARGS__)
  switch (ast.type) {
  case NODE_PROGRAM: {
    Program *program = ast.node;
    pad("Program {");
    space_size += 2;
    on_line = false;
    da_foreach(AstNode, statement, program) print_node(*statement);
    space_size -= 2;
    pad("}\n");
    break;
  }
  case NODE_COMMENT: {
    Comment *comment = ast.node;
    pad("Comment");
    space_size += 2;
    padded("%.*s", (int)comment->str.count, comment->str.data);
    space_size -= 2;
    padded("");
    break;
  }
  case NODE_STATEMENT: {
    Statement *statement = ast.node;
    pad("Statement {");
    on_line = true;
    print_node(*statement->item);
    pad("}");

    break;
  }
  case NODE_ASSIGNMENT: {
    Assignment *assignment = ast.node;
    pad("Assignment {");
    on_line = true;
    print_node(*assignment->item);
    pad("}");
    break;
  }
  case NODE_FULL_ASSIGNMENT: {
    FullAssignment *assignment = ast.node;
    pad("FullAssignment {");
    space_size += 2;
    on_line = true;
    padded("target: ");
    print_node(*assignment->target);
    padded("type: ");
    print_node(*assignment->type);
    padded("value: ");
    print_node(*assignment->value);
    space_size -= 2;
    padded("}");
    break;
  }
  case NODE_IDENTIFIER: {
    Identifier *assignment = ast.node;
    pad("Identifier %.*s", (int)assignment->str.count, assignment->str.data);
    break;
  }
  case NODE_TYPE: {
    Type *type = ast.node;
    pad("Type {");
    on_line = true;
    print_node(*type->item);
    pad("}");
    break;
  }
  case NODE_VALUE: {
    Value *value = ast.node;
    pad("Value ");
    switch (value->kind) {
    case VALUE_IDENTIFIER:
      printf("<identifier> %.*s", (int)value->as.identifier.count,
             value->as.identifier.data);
      break;
    case VALUE_STRING:
      printf("<string> %.*s", (int)value->as.string.count,
             value->as.string.data);
      break;
    case VALUE_NUMBER:
      printf("<number> %.*s", (int)value->as.number.count,
             value->as.number.data);
      break;
    case VALUE_CHAR:
      printf("<char> %.*s", (int)value->as.character.count,
             value->as.character.data);
      break;
    case VALUE_TRUE:
      printf("<true>");
      break;
    case VALUE_FALSE:
      printf("<false>");
      break;
    }
    break;
  }
  case NODE_IMPLIED_ASSIGNMENT: {
    ImpliedAssignment *assignment = ast.node;
    pad("ImpliedAssignment {");
    space_size += 2;
    on_line = true;
    padded("target: ");
    print_node(*assignment->left);
    padded("value: ");
    print_node(*assignment->right);
    space_size -= 2;
    padded("}");
    break;
  }
  case NODE_TYPE_OR_CONST_ASSIGNMENT: {
    TypeOrConstAssignment *assignment = ast.node;
    pad("TypeOrConstAssignment {");
    space_size += 2;
    on_line = true;
    padded("target: ");
    print_node(*assignment->left);
    padded("type: ");
    print_node(*assignment->right);
    space_size -= 2;
    padded("}");
    break;
  }
  case NODE_TYPE_ASSIGNMENT: {
    TypeOrConstAssignment *assignment = ast.node;
    pad("TypeAssignment {");
    space_size += 2;
    on_line = true;
    padded("target: ");
    print_node(*assignment->left);
    padded("type: ");
    print_node(*assignment->right);
    space_size -= 2;
    padded("}");
    break;
  }
  case NODE_CONST_ASSIGNMENT: {
    TypeOrConstAssignment *assignment = ast.node;
    pad("ConstAssignment {");
    space_size += 2;
    on_line = true;
    padded("target: ");
    print_node(*assignment->left);
    padded("value: ");
    print_node(*assignment->right);
    space_size -= 2;
    padded("}");
    break;
  }
  case NODE_VALUE_ASSIGNMENT: {
    ValueAssignment *assignment = ast.node;
    pad("ValueAssignment {");
    space_size += 2;
    on_line = true;
    padded("target: ");
    print_node(*assignment->left);
    padded("value: ");
    print_node(*assignment->right);
    space_size -= 2;
    padded("}");
    break;
  }
  case NODE_EXPRESSION: {
    Expression *expression = ast.node;
    pad("Expression {");
    on_line = true;
    print_node(*expression->item);
    pad("}");
    break;
  }
  case NODE_BINARY_EXPRESSION: {
    BinaryExpression *expression = ast.node;
    pad("BinaryExpression {");
    space_size += 2;
    on_line = true;
    padded("kind: %s", binary_expression_kind_to_string(expression->kind));
    padded("left: ");
    print_node(*expression->left);
    padded("right: ");
    print_node(*expression->right);
    space_size -= 2;
    padded("}");
    break;
  }
  case NODE_MULTIWORD_TYPE: {
    MultiwordType *type = ast.node;
    pad("MultiwordType {%.*s}", (int)type->str.count, type->str.data);
    break;
  }
  default:
    UNREACHABLE(
        temp_sprintf("Unhandled node <%s>", node_type_to_string(ast.type)));
  }

  on_line = was_on_line;
  space_size = space_size_was;
#undef pad
#undef padded
}

#define free_node(ast, ...) __free_node((ast), __VA_ARGS__ + 0)

void __free_node(AstNode *ast, bool early_return) {
  if (!ast)
    return;
  switch (ast->type) {
  case NODE_PROGRAM: {
    Program *program = ast->node;
    da_foreach(AstNode, statement, program) free_node(statement, true);
    da_free(*program);
    free(program);
    break;
  }
  case NODE_STATEMENT: {
    Statement *val = ast->node;
    free_node(val->item);
    free(val);
    break;
  }
  case NODE_ASSIGNMENT: {
    Assignment *val = ast->node;
    free_node(val->item);
    free(val);
    break;
  }
  case NODE_FULL_ASSIGNMENT: {
    FullAssignment *val = ast->node;
    free_node(val->target);
    free_node(val->type);
    free_node(val->value);
    free(val);
    break;
  }
  case NODE_IMPLIED_ASSIGNMENT: {
    ImpliedAssignment *val = ast->node;
    free_node(val->left);
    free_node(val->right);
    free(val);
    break;
  }
  case NODE_TYPE_OR_CONST_ASSIGNMENT: {
    TypeOrConstAssignment *val = ast->node;
    free_node(val->left);
    free_node(val->right);
    free(val);
    break;
  }
  case NODE_CONST_ASSIGNMENT: {
    TypeOrConstAssignment *val = ast->node;
    free_node(val->left);
    free_node(val->right);
    free(val);
    break;
  }
  case NODE_TYPE_ASSIGNMENT: {
    TypeOrConstAssignment *val = ast->node;
    free_node(val->left);
    free_node(val->right);
    free(val);
    break;
  }
  case NODE_VALUE_ASSIGNMENT: {
    ValueAssignment *val = ast->node;
    free_node(val->left);
    free_node(val->right);
    free(val);
    break;
  }
  case NODE_TYPE: {
    Type *val = (Type *)ast->node;
    free_node(val->item);
    free(val);
    break;
  }
  case NODE_COMMENT:
  case NODE_VALUE: {
    free(ast->node);
    break;
  }
  case NODE_IDENTIFIER: {
    Identifier *val = ast->node;
    free(val);
    break;
  }
  case NODE_EXPRESSION: {
    Expression *val = ast->node;
    free_node(val->item);
    free(val);
    break;
  }
  case NODE_BINARY_EXPRESSION: {
    BinaryExpression *val = ast->node;
    free_node(val->left);
    free_node(val->right);
    free(val);
    break;
  }
  case NODE_MULTIWORD_TYPE: {
    MultiwordType *val = ast->node;
    free(val);
    break;
  }
  default:
    UNREACHABLE(temp_sprintf("unknown node type: <%s>",
                             node_type_to_string(ast->type)));
  }
  if (!early_return)
    free(ast);
}

#define node_type_to_enum_val(type)                                            \
  _Generic(type,                                                               \
      Program: NODE_PROGRAM,                                                   \
      Comment: NODE_COMMENT,                                                   \
      Statement: NODE_STATEMENT,                                               \
      Assignment: NODE_ASSIGNMENT,                                             \
      FullAssignment: NODE_FULL_ASSIGNMENT,                                    \
      ImpliedAssignment: NODE_IMPLIED_ASSIGNMENT,                              \
      TypeOrConstAssignment: NODE_TYPE_OR_CONST_ASSIGNMENT,                    \
      ValueAssignment: NODE_VALUE_ASSIGNMENT,                                  \
      Expression: NODE_EXPRESSION,                                             \
      BinaryExpression: NODE_BINARY_EXPRESSION,                                \
      Type: NODE_TYPE,                                                         \
      Value: NODE_VALUE,                                                       \
      Identifier: NODE_IDENTIFIER,                                             \
      MultiwordType: NODE_MULTIWORD_TYPE,                                      \
      default: __builtin_trap())

bool __parse(Lexer *lexer, AstNode **result, AstNode *(**parsers)(Lexer *),
             size_t parsers_count, const char *current_items,
             const char *caller_name, const char *file_name, int file_line) {
  AstNode *ast_node = NULL;
  nob_log(INFO, "\\ %s: Searching nodes: %s", caller_name, current_items);
  choice_depth += 1;
  for (size_t i = 0; i < parsers_count; ++i) {
    Lexer lexer_copy = *lexer;
    ast_node = parsers[i](&lexer_copy);
    if (ast_node != NULL) {
      nob_log(INFO, "/ %s: Found node <%s> '%.*s'", caller_name,
              node_type_to_string(ast_node->type), (int)ast_node->source.count,
              ast_node->source.data);
      *lexer = lexer_copy;
      *result = ast_node;
      break;
    }
  }
  choice_depth -= 1;
  if (ast_node == NULL) {
    nob_log(WARNING, "/ %s:%d: %s: Expected node from: %s", file_name,
            file_line, caller_name, current_items);
    if (!choice_depth)
      __builtin_trap();
    return false;
  }
  return true;
}

int __consume_str(Lexer *lexer, TokenType type, const char **strs,
                  size_t strs_count, const char *current_items,
                  const char *caller_name, const char *file_name,
                  int file_line) {
  nob_log(INFO, "\\ %s: Searching %s from: %s", caller_name,
          token_type_to_string(type), current_items);
  if (lexer->current_token.type != type) {
    nob_log(WARNING, "/ %s:%d: %s: Expected %s, got %s", file_name, file_line,
            caller_name, token_type_to_string(type),
            token_type_to_string(lexer->current_token.type));
    return -1;
  }
  choice_depth += 1;
  for (size_t i = 0; i < strs_count; ++i) {
    const char *str = strs[i];
    size_t len = strlen(str);
    if (type == TOKEN_SYMBOL) {
      Lexer copy = *lexer;
      while (len > 0) {
        if (copy.current_token.type != type ||
            !sv_starts_with((String_View){len, str}, copy.current_token.lexeme))
          break;
        // printf("ate symbol '%c'\n", *copy.current_token.lexeme.data);
        lexer_next_token(&copy);
        str += 1;
        len -= 1;
      }
      if (!len) {
        str = strs[i];
        len = strlen(str);
        nob_log(INFO, "/ %s: Found %s: \"%s\"", caller_name,
                token_type_to_string(type), str);
        *lexer = copy;
        choice_depth -= 1;
        return i;
      }
    } else {
      if (lexer->current_token.lexeme.count != len ||
          !sv_starts_with(lexer->current_token.lexeme, (String_View){len, str}))
        continue;
      nob_log(INFO, "/ %s: Found %s: \"%s\"", caller_name,
              token_type_to_string(type), str);
      lexer_next_token(lexer);
      choice_depth -= 1;
      return i;
    }
  }
  nob_log(WARNING, "/ %s:%d: %s: Found no %s from: %s", file_name, file_line,
          caller_name, token_type_to_string(type), current_items);
  if (!--choice_depth)
    __builtin_trap(); // TODO: exit properly
  return -1;
}

AstNode *parse_type(Lexer *lexer);
AstNode *parse_identifier(Lexer *lexer);
AstNode *parse_type_function_argument(Lexer *lexer) {
  UNUSED(lexer);
  TODO("Implement parse_type_function_argument.");
}

AstNode *parse_list(Lexer *lexer, const char *delim,
                    AstNode *(*parse_list_item)(Lexer *lexer)) {
  UNUSED(lexer);
  UNUSED(delim);
  UNUSED(parse_list_item);
  TODO("Implement parse_list");
}

AstNode *parse_type_function_call(Lexer *lexer) {
  UNUSED(lexer);
  TODO("Implement parse_type_function_call.");
}

AstNode *parse_literal(Lexer *lexer) {
  UNUSED(lexer);
  TODO("Implement parse_literal.");
}

AstNode *parse_function_type(Lexer *lexer) {
  UNUSED(lexer);
  TODO("Implement parse_function_type.");
}

AstNode *parse_code_block(Lexer *lexer) {
  UNUSED(lexer);
  TODO("Implement parse_code_block.");
}

AstNode *parse_multiword_type(Lexer *lexer) {
  AstNode type = new_ast_node(MultiwordType, lexer);
  String_View str = cast(MultiwordType *, type.node)->str;
  Lexer copy = *lexer;

  consume_str(lexer, SYMBOL, "(");
  str.data = lexer->current_token.lexeme.data;
  consume_tok(lexer, IDENTIFIER);
  for (;;) {
    str.count = lexer->current_token.lexeme.data +
                lexer->current_token.lexeme.count - str.data;
    consume_tok(lexer, IDENTIFIER);
    if (consume_str_opt(lexer, SYMBOL, ")") == 0)
      break;
  }
  cast(MultiwordType *, type.node)->str = str;

  return compose_ast_node(lexer, type);
throw:;
  *lexer = copy;
  free_node(&type, true);
  return NULL;
}

AstNode *parse_type(Lexer *lexer) {
  AstNode *ast_buf, type = new_ast_node(Type, lexer);

  parse_or_throw(lexer, ast_buf, multiword_type, identifier);
  cast(Type *, type.node)->item = ast_buf;

  return compose_ast_node(lexer, type);
throw:;
  free_node(&type, true);
  return NULL;
}

AstNode *parse_value(Lexer *lexer) {
  AstNode value_ast = new_ast_node(Value, lexer);
  Value *value = value_ast.node;

  switch (lexer->current_token.type) {
  case TOKEN_IDENTIFIER:
    value->kind = VALUE_IDENTIFIER;
    value->as.identifier = lexer->current_token.lexeme;
    break;
  case TOKEN_STRING:
    value->kind = VALUE_STRING;
    value->as.string = lexer->current_token.lexeme;
    break;
  case TOKEN_NUMBER:
    value->kind = VALUE_NUMBER;
    value->as.number = lexer->current_token.lexeme;
    break;
  case TOKEN_CHAR:
    value->kind = VALUE_CHAR;
    value->as.number = lexer->current_token.lexeme;
    break;
  case TOKEN_TRUE:
    value->kind = VALUE_TRUE;
    value->as.truth = lexer->current_token.lexeme;
    break;
  case TOKEN_FALSE:
    value->kind = VALUE_FALSE;
    value->as.falth = lexer->current_token.lexeme;
    break;
  default:;
  }
  consume_tok(lexer, IDENTIFIER, STRING, NUMBER, CHAR, TRUE, FALSE);

  return compose_ast_node(lexer, value_ast);
throw:;
  free_node(&value_ast, true);
  return NULL;
}

AstNode *parse_identifier(Lexer *lexer) {
  AstNode identifier = new_ast_node(Identifier, lexer);

  cast(Identifier *, identifier.node)->str = lexer->current_token.lexeme;
  consume_tok(lexer, IDENTIFIER);

  return compose_ast_node(lexer, identifier);
throw:;
  free_node(&identifier, true);
  return NULL;
}

AstNode *parse_binary_expression(Lexer *lexer);

AstNode *parse_binary_expression_tail(Lexer *lexer, AstNode *left) {
  AstNode *ast_buf, bin_expression_ast;
  Lexer lexer_copy = *lexer;

  for (;;) {
    bin_expression_ast = new_ast_node(BinaryExpression, &lexer_copy);
    BinaryExpression *bin_expression = bin_expression_ast.node;

    bin_expression->kind = consume_str(&lexer_copy, SYMBOL, BIN_EXPR_STRS);
    parse_or_throw(&lexer_copy, ast_buf, binary_expression);

    // TODO: manage precedence

    bin_expression_ast.source.data = left->source.data;
    bin_expression->left = left;
    bin_expression->right = ast_buf;
    left = compose_ast_node(&lexer_copy, bin_expression_ast);
    *lexer = lexer_copy;
  }

throw:;
  free_node(&bin_expression_ast, true);
  return left;
}

AstNode *parse_binary_expression(Lexer *lexer) {
  AstNode *ast_buf;
  parse_or_throw(lexer, ast_buf, value);
  return parse_binary_expression_tail(lexer, ast_buf);
throw:;
  return NULL;
}

AstNode *parse_expression(Lexer *lexer) {
  AstNode *ast_buf, expression = new_ast_node(Expression, lexer);

  parse_or_throw(lexer, ast_buf, binary_expression);
  cast(Expression *, expression.node)->item = ast_buf;

  return compose_ast_node(lexer, expression);
throw:;
  free_node(&expression, true);
  return NULL;
}

/*
  Parses full assignments.
  BNF: TODO
  EXAPMLES: ```
    name : type : val
    name : type = val
  ```*/
AstNode *parse_full_assignment(Lexer *lexer) {
  AstNode *ast_buf, full_assignment_ast = new_ast_node(FullAssignment, lexer);
  FullAssignment *full_assignment = full_assignment_ast.node;

  parse_or_throw(lexer, ast_buf, identifier);
  full_assignment->target = ast_buf;

  consume_str(lexer, SYMBOL, ":");

  parse_or_throw(lexer, ast_buf, type);
  full_assignment->type = ast_buf;

  consume_str(lexer, SYMBOL, ":", "=");

  parse_or_throw(lexer, ast_buf, expression);
  full_assignment->value = ast_buf;

  return compose_ast_node(lexer, full_assignment_ast);
throw:;
  free_node(&full_assignment_ast, true);
  return NULL;
}

AstNode *parse_implied_assignment(Lexer *lexer) {
  AstNode *ast_buf,
      implied_assignment_ast = new_ast_node(ImpliedAssignment, lexer);
  ImpliedAssignment *implied_assignment = implied_assignment_ast.node;

  parse_or_throw(lexer, ast_buf, identifier);
  implied_assignment->left = ast_buf;

  consume_str(lexer, SYMBOL, "::", ":=");

  parse_or_throw(lexer, ast_buf, expression);
  implied_assignment->right = ast_buf;

  return compose_ast_node(lexer, implied_assignment_ast);
throw:;
  free_node(&implied_assignment_ast, true);
  return NULL;
}

AstNode *parse_type_or_const_assignment(Lexer *lexer) {
  AstNode *ast_buf, assignment = new_ast_node(TypeOrConstAssignment, lexer);

  parse_or_throw(lexer, ast_buf, identifier);
  cast(ImpliedAssignment *, assignment.node)->left = ast_buf;

  consume_str(lexer, SYMBOL, ":");

  parse_or_throw(lexer, ast_buf, identifier, type, expression);
  cast(ImpliedAssignment *, assignment.node)->right = ast_buf;
  switch (ast_buf->type) {
  case NODE_TYPE:
    assignment.type = NODE_TYPE_ASSIGNMENT;
    break;
  case NODE_VALUE:
    assignment.type = NODE_CONST_ASSIGNMENT;
    break;
  default:
    assignment.type = NODE_TYPE_OR_CONST_ASSIGNMENT;
  }

  return compose_ast_node(lexer, assignment);
throw:;
  free_node(&assignment, true);
  return NULL;
}

AstNode *parse_value_assignment(Lexer *lexer) {
  AstNode *ast_buf, value_assignment = new_ast_node(ValueAssignment, lexer);

  parse_or_throw(lexer, ast_buf, identifier);
  cast(ImpliedAssignment *, value_assignment.node)->left = ast_buf;

  consume_str(lexer, SYMBOL, "=");

  parse_or_throw(lexer, ast_buf, expression);
  cast(ImpliedAssignment *, value_assignment.node)->right = ast_buf;

  return compose_ast_node(lexer, value_assignment);
throw:;
  free_node(&value_assignment, true);
  return NULL;
}

AstNode *parse_assignment(Lexer *lexer) {
  AstNode *ast_buf, assignment = new_ast_node(Assignment, lexer);

  parse_or_throw(lexer, ast_buf, full_assignment, implied_assignment,
                 type_or_const_assignment, value_assignment);
  cast(Assignment *, assignment.node)->item = ast_buf;

  return compose_ast_node(lexer, assignment);
throw:;
  free_node(&assignment, true);
  return NULL;
}

AstNode *parse_function_call(Lexer *lexer) {
  UNUSED(lexer);
  TODO("Implement parse_function_call.");
}

/*
  Parses c like comments.
  BNF: NO
  EXAMPLES: ```
    // single line comment
    /\*
      multiline comment
    *\/
    /\*
      multiline /\*
        nested
      *\/ comment
    *\/
  ```*/
AstNode *parse_comment(Lexer *lexer) {
  AstNode comment = new_ast_node(Comment, lexer);
  cast(Comment *, comment.node)->str = lexer->current_token.lexeme;
  consume_tok(lexer, COMMENT);
  return compose_ast_node(lexer, comment);
throw:;
  free_node(&comment, true);
  return NULL;
}

/*
  Parses a single statement.
  BNF: <comment> | <assignment> | <function_call>
  EXAMPLES:
  `// comments are statements that do nothing`
  `std :: #use "std"`
  `foo :: (() 3)()`
  `bar: bool: true`*/
AstNode *parse_statement(Lexer *lexer) {
  AstNode *ast_buf, statement = new_ast_node(Statement, lexer);

  parse_or_throw(lexer, ast_buf, comment, assignment);
  cast(Statement *, statement.node)->item = ast_buf;

  consume_str_opt(lexer, SYMBOL, ";");

  return compose_ast_node(lexer, statement);
throw:;
  free_node(&statement, true);
  return NULL;
}

/*
  Parses the whole program.
  BNF: <statement>*
  EXAPMLE: ```
    std :: #use "std"

    a: char: 'c'
    b :: () 123
    main :: () {
      return 0
    }
  ```*/
AstNode *parse_program(Lexer *lexer) {
  AstNode *ast_buf, program = new_ast_node(Program, lexer);

  while (lexer->current_token.type != TOKEN_EOF) {
    parse_or_throw(lexer, ast_buf, statement);
    da_append((Program *)program.node, *ast_buf);
    free(ast_buf);
  };

  return compose_ast_node(lexer, program);
throw:;
  free_node(&program, true);
  return NULL;
}

AstNode *parse_source_file(Lexer lexer) {
  lexer_next_token(&lexer);

  return parse_program(&lexer);
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

  String_Builder source_file = {0};
  argc -= 1;
  if (!read_entire_file(*rest_argv, &source_file))
    return 1;
  sb_append_null(&source_file);
  Lexer lexer = {
      .input = sb_to_sv(source_file),
      .source_file_path = *rest_argv,
      .pos = 0,
      .current_token = {0},
  };
  AstNode *program = parse_source_file(lexer);
  nob_log(INFO, "Parsed file.");
  print_node(*program);
  free_node(program);
  sb_free(source_file);

  // sb_append_null(&source_file);
  // printf("%s", source_file.items);

  return 69;
  // --- raw API ---
  HM hm = {0};
  {
    HM_init(&hm, sizeof(int), 0);

    int val = 2;
    HM_set(&hm, "test", &val);

    int *res = HM_get(&hm, "test");
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
