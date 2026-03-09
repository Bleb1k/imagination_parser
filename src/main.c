#include <ctype.h>
#include <fcntl.h>
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

// #define PARSER_DEBUG
#include "./macro_witchery.h"

// void *__malloc_log(void *ptr, size_t size, char *f, int l) {
//   fprintf(stderr, "%p: %s:%d: > %zu\n", ptr, f, l, size);
//   return ptr;
// }
// #define malloc(size_) __malloc_log(malloc((size_)), (size_), __FILE__,
// __LINE__)

NOBDEF void log_handler_stdout(Nob_Log_Level level, const char *fmt,
                               va_list args) {
  switch (level) {
  case NOB_INFO:
    fprintf(stdout, "[INFO] ");
    break;
  case NOB_WARNING:
    fprintf(stdout, "[WARNING] ");
    break;
  case NOB_ERROR:
    fprintf(stdout, "[ERROR] ");
    break;
  case NOB_NO_LOGS:
    return;
  default:
    NOB_UNREACHABLE("Nob_Log_Level");
  }
  vfprintf(stdout, fmt, args);
  fprintf(stdout, "\n");
}

/* Tokenizer helper function */
bool is_str_here(const char *src, const char *needle) {
  size_t len = strlen(needle);
  for (size_t i = 0; i < len; i++) {
    if (src[i] != needle[i]) {
      return false;
    }
  }
  return true;
}

typedef enum TokenType {
  // ----- special -----
  TOKEN_NULL = 1,
  TOKEN_EOF,
  // ----- multi-character -----
  TOKEN_IDENTIFIER,
  TOKEN_INT,
  TOKEN_INTU,
  TOKEN_INTL,
  TOKEN_INTUL,
  TOKEN_INTLL,
  TOKEN_INTULL,
  TOKEN_FLOAT32,
  TOKEN_FLOAT64,
  TOKEN_FLOAT128,
  TOKEN_STRING,
  TOKEN_COMMENT,
  TOKEN_ARROW,
  // ----- single-character -----
  TOKEN_CHAR,
  TOKEN_SYMBOL,
} TokenType;

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
  case TOKEN_INT:
    return "int";
  case TOKEN_INTU:
    return "intu";
  case TOKEN_INTL:
    return "intl";
  case TOKEN_INTUL:
    return "intul";
  case TOKEN_INTLL:
    return "intll";
  case TOKEN_INTULL:
    return "intull";
  case TOKEN_FLOAT32:
    return "float32";
  case TOKEN_FLOAT64:
    return "float64";
  case TOKEN_FLOAT128:
    return "float128";
  case TOKEN_STRING:
    return "string";
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
} Token;

typedef struct Lexer {
  String_View input;
  char *source_file_path;
  Token current_token;
  size_t pos;
} Lexer;

void lexer_next_token(Lexer *lexer) {
#define RETURN_TOKEN(TOK, VAL_FROM, VAL_TO)                                    \
  do {                                                                         \
    lexer->current_token =                                                     \
        (Token){.lexeme = {.data = VAL_FROM, .count = VAL_TO - VAL_FROM},      \
                .type = TOK};                                                  \
    lexer->pos = VAL_TO - lexer->input.data;                                   \
  } while (0)

  printf("==> %s " SV_Fmt "\n", token_type_to_string(lexer->current_token.type),
         SV_Arg(lexer->current_token.lexeme));

  const char *cur_char = lexer->input.data + lexer->pos;

  if ((size_t)lexer->pos >= lexer->input.count - 1) {
    RETURN_TOKEN(TOKEN_EOF, cur_char, cur_char);
    return;
  }

  while (*cur_char == ' ' || *cur_char == '\t' || *cur_char == '\n') {
    cur_char++;
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
      cur_char += 2;
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

    RETURN_TOKEN(TOKEN_IDENTIFIER, start, cur_char);
    return;
  }

  // Handle strings
  if (*cur_char == '"') {
    // String_Builder sb = {0};
    const char *start = (char *)++cur_char;

    while (*cur_char != '"' && *cur_char != '\n')
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

    TokenType type = TOKEN_INT;

    if (!is_floating) {
      if (*cur_char == 'u' || *cur_char == 'U') {
        cur_char++;
        type = TOKEN_INTU;
      }
      if (*cur_char == 'l' || *cur_char == 'L') {
        cur_char++;
        type = type == TOKEN_INT ? TOKEN_INTL : TOKEN_INTUL;
      }
      if (*cur_char == 'l' || *cur_char == 'L') {
        cur_char++;
        type = type == TOKEN_INTL ? TOKEN_INTLL : TOKEN_INTULL;
      }
    } else {
      type = TOKEN_FLOAT64;
      // exponent
      if (*cur_char == 'e' || *cur_char == 'E') {
        cur_char++;
        if (*cur_char == '+' || *cur_char == '-')
          cur_char++;
        while (isdigit(*cur_char))
          cur_char++;
      } else if (*cur_char == 'f' || *cur_char == 'F') {
        cur_char++;
        type = TOKEN_FLOAT32;
      } else if (*cur_char == 'l' || *cur_char == 'L') {
        cur_char++;
        type = TOKEN_FLOAT128;
      }
    }
    if (*cur_char == 'i')
      cur_char++;

    // da_append_many(&sb, start, cur_char - start);

    // char *lexeme = (char *)temp_sv_to_cstr(sb_to_sv(sb));

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
    lexer->pos = cur_char - lexer->input.data;
    return;
  }

  if (is_str_here(cur_char, "->")) {
    RETURN_TOKEN(TOKEN_ARROW, cur_char, cur_char + 2);
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
  X(INVALID)                                                                   \
  X(GE)                                                                        \
  X(GT)                                                                        \
  X(ID)                                                                        \
  X(IF)                                                                        \
  X(LE)                                                                        \
  X(LT)                                                                        \
  X(FOR)                                                                       \
  X(MOD)                                                                       \
  X(PTR)                                                                       \
  X(TYPE)                                                                      \
  X(CALL)                                                                      \
  X(CAST)                                                                      \
  X(ELSE)                                                                      \
  X(ENUM)                                                                      \
  X(EXPR)                                                                      \
  X(LOOP)                                                                      \
  X(PLUS)                                                                      \
  X(TAIL)                                                                      \
  X(BLOCK)                                                                     \
  X(DEREF)                                                                     \
  X(EQUAL)                                                                     \
  X(FIELD)                                                                     \
  X(MINUS)                                                                     \
  X(OTHER)                                                                     \
  X(RANGE)                                                                     \
  X(SLICE)                                                                     \
  X(TIMES)                                                                     \
  X(TUPLE)                                                                     \
  X(ASSIGN)                                                                    \
  X(BIT_OR)                                                                    \
  X(DIVIDE)                                                                    \
  X(PARENS)                                                                    \
  X(STRING)                                                                    \
  X(STRUCT)                                                                    \
  X(SWITCH)                                                                    \
  X(BIT_AND)                                                                   \
  X(BIT_NOT)                                                                   \
  X(LITERAL)                                                                   \
  X(UNKNOWN)                                                                   \
  X(ENUM_VAL)                                                                  \
  X(FUNCTION)                                                                  \
  X(INDEXING)                                                                  \
  X(NEGATIVE)                                                                  \
  X(VARIABLE)                                                                  \
  X(ASSIGN_EQ)                                                                 \
  X(FLOW_STMT)                                                                 \
  X(NOT_EQUAL)                                                                 \
  X(LOGICAL_OR)                                                                \
  X(VAL_ASSIGN)                                                                \
  X(FULL_ASSIGN)                                                               \
  X(LOGICAL_AND)                                                               \
  X(LOGICAL_NOT)                                                               \
  X(TYPE_ASSIGN)                                                               \
  X(STRUCT_LITERAL)                                                            \
  X(IMPLIED_ASSIGN)                                                            \
  X(FULL_CONST_ASSIGN)                                                         \
  X(IMPLIED_CONST_ASSIGN)

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
  default:
    UNREACHABLE(temp_sprintf("Unknown NodeType %d", type));
  }
}

typedef struct ProgramNode Program;
typedef struct StmtNode Stmt;
typedef struct SwitchCaseNode SwitchCase;
typedef struct CaseItemNode CaseItem;
typedef struct IfStmtNode IfStmt;
typedef struct ForBodyNode ForBody;
typedef struct AssignNode Assign;
typedef struct AssignEqNode AssignEq;
typedef struct RvalNode Rval;
typedef struct FunctionNode Function;
typedef struct ParamItemNode ParamItem;
typedef struct EnumItemNode EnumItem;
typedef struct TypeNode Type;
typedef struct TypeNoIdentNode TypeNoIdent;
typedef struct ExprNode Expr;
typedef struct LvalNode Lval;

#define AST_NODES                                                              \
  X(Program)                                                                   \
  X(Stmt)                                                                      \
  X(SwitchCase)                                                                \
  X(CaseItem)                                                                  \
  X(IfStmt)                                                                    \
  X(ForBody)                                                                   \
  X(Assign)                                                                    \
  X(AssignEq)                                                                  \
  X(Rval)                                                                      \
  X(Function)                                                                  \
  X(ParamItem)                                                                 \
  X(EnumItem)                                                                  \
  X(Type)                                                                      \
  X(TypeNoIdent)                                                               \
  X(Expr)                                                                      \
  X(Lval)

typedef union AstNode AstNode;

// AstNode *__compose_ast_node(AstNode node, int source_count) {
//   AstNode *result = malloc(sizeof(AstNode));
//   node.source.count = source_count;
//   node.source = sv_trim_right(node.source);
//   *result = node;
//   return result;
// }
#define compose_node(_node, lexer)                                             \
  ((_node).source.count =                                                      \
       (lexer)->current_token.lexeme.data - (_node).source.data,               \
   (_node))
// (_node){.source = {.count}}
// __compose_ast_node((_node), (int)((lexer)->current_token.lexeme.data - \
//                                   (_node).source.data))

typedef struct AstCallbackContext {
  HM *scope;
  const char *file_name;
  const char *source;
} AstCallbackContext;

typedef struct AstCallback {
  AstCallbackContext context;
  void (*fn)(AstCallbackContext context, AstNode *item);
} AstCallback;

typedef struct CallbackDA {
  AstCallback *items;
  size_t count, capacity;
} AstCallbackDA;

typedef enum AstNodeId {
#define X(v) v##Id,
  AST_NODES NODE_TYPES
#undef X
} AstNodeId;

AstCallbackDA callbacks[] = {
#define X(v) [v##Id] = {0},
    AST_NODES
#undef X
};

#define add_callback(typ, fn, ctx)                                             \
  da_append(&callbacks[(typ##Id)], ((AstCallback){(ctx), (fn)}))

#define call_ast_callbacks(typ, ast)                                           \
  do {                                                                         \
    iterate(&callbacks[(typ##Id)]) it->fn(it->context, (ast));                 \
  } while (0)

#define arr_field(Type)                                                        \
  struct {                                                                     \
    Type *items;                                                               \
    size_t count, capacity;                                                    \
  }

struct LvalNode {
  String_View source;
  NodeType type;
  union {
    Token id;
    struct {
      arr_field(Token);
    } field;
  } as;
};
struct ExprNode {
  String_View source;
  NodeType type;
  bool parens;
  union {
    Token enum_val, literal;
    Lval variable;
    arr_field(Token) string;
    Expr *prefix;
    struct {
      Expr *expr;
      union {
        arr_field(Rval) call;
        Type *cast;
        Rval *indexing;
        struct {
          Rval *from, *to;
          bool inclusive;
        } range;
        uint8_t deref_count;
      };
    } postfix;
    struct {
      Expr *left, *right;
    } times, divide, mod, bit_and, plus, minus, bit_or, bit_not, equal,
        not_equal, lt, gt, le, ge, logical_or, logical_and, binop;
  } as;
};
struct TypeNoIdentNode {
  String_View source;
  NodeType type;
  union {
    arr_field(Token) unknown;
    struct {
      Lval *type;
      arr_field(EnumItem) items;
    } enum_;
    arr_field(ParamItem) struct_;
  } as;
};
struct TypeNode {
  String_View source;
  NodeType type;
  union {
    Token ident;
    TypeNoIdent other;
    arr_field(Type) tuple;
    Type *slice, *ptr;
    struct {
      arr_field(Type) params;
      Type *result;
    } function;
  } as;
};
struct EnumItemNode {
  String_View source;
  Token ident;
  Expr *expr;
};
struct ParamItemNode {
  String_View source;
  arr_field(Token) idents;
  Type type;
};
struct FunctionNode {
  String_View source;
  arr_field(ParamItem) params;
  Type *result;
  Program *program;
};
struct RvalNode {
  String_View source;
  NodeType type;
  union {
    Expr expr;
    Function function;
    struct {
      Lval *type;
      arr_field(EnumItem) items;
    } struct_literal;
    TypeNoIdent type;
    struct {
      Expr expr;
      arr_field(SwitchCase) case_;
      arr_field(Rval) rval;
    } switch_;
    struct {
      Rval *val;
      Expr expr;
      Rval *else_val;
    } if_;
  } as;
};
struct AssignEqNode {
  String_View source;
  Lval lval;
  Rval rval;
  Token kind;
};
struct AssignNode {
  String_View source;
  NodeType type;
  Lval lval;
  Type typ;
  Rval rval;
  Token assign_eq_kind;
};
struct ForBodyNode {
  String_View source;
  Assign *start;
  Expr expr;
  NodeType end_type;
  union {
    AssignEq *assign_eq;
    Expr *expr;
  } end;
};
struct IfStmtNode {
  String_View source;
  NodeType type;
  union {
    struct {
      Expr expr;
      Program *program;
    } if_;
    arr_field(IfStmt) else_;
    struct {
      IfStmt *start;
      Program *end;
    } tail;
  } as;
};
struct CaseItemNode {
  String_View source;
  Rval rval;
  Expr *if_expr;
};
struct SwitchCaseNode {
  String_View source;
  bool default_;
  arr_field(CaseItem) items;
};
struct StmtNode {
  String_View source;
  NodeType type;
  union {
    Assign assign;
    struct {
      Lval lval;
      arr_field(Rval) params;
    } call;
    IfStmt if_;
    struct {
      Expr expr;
      arr_field(SwitchCase) case_;
      arr_field(Program) body;
    } switch_;
    struct {
      Token which;
      Rval *rval;
    } flow_stmt;
    Program *block, *loop;
    struct {
      ForBody head;
      Stmt *stmt;
    } for_;
  } as;
};
typedef struct SwitchStmtBranches SwitchStmtBranches;
struct ProgramNode {
  String_View source;
  arr_field(Stmt) stmts;
};

int expr_precedence(Expr *expr) {
  if (expr->parens)
    return 0;
  switch (expr->type) {
  case NODE_ENUM_VAL:
  case NODE_VARIABLE:
  case NODE_STRING:
  case NODE_LITERAL:
    return 0;
  case NODE_NEGATIVE:
  case NODE_LOGICAL_NOT:
    return 1;
  case NODE_CALL:
  case NODE_CAST:
  case NODE_DEREF:
  case NODE_INDEXING:
  case NODE_RANGE:
    return 2;
  case NODE_TIMES:
  case NODE_DIVIDE:
  case NODE_MOD:
  case NODE_BIT_AND:
    return 3;
  case NODE_PLUS:
  case NODE_MINUS:
  case NODE_BIT_OR:
  case NODE_BIT_NOT:
    return 4;
  case NODE_EQUAL:
  case NODE_NOT_EQUAL:
  case NODE_LT:
  case NODE_GT:
  case NODE_LE:
  case NODE_GE:
    return 5;
  case NODE_LOGICAL_AND:
    return 6;
  case NODE_LOGICAL_OR:
    return 7;
  default:
    return -1;
  }
}

#define padded_puts(str, ...) __padded_puts((str), __VA_ARGS__ + 0)
static void __padded_puts(const char *str, int pad_change) {
  static int pad = 0;
  pad += pad_change;
  if (pad < 0)
    __asm("int3");
  while (*str != '\0') {
    printf("%c", *str);
    if (*str++ == '\n') {
      printf(
          "%.*s", pad,
          "|   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   "
          "|   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   "
          "|   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   "
          "|   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   ");
    }
  }
}

void print_program(Program node);
void print_stmt(Stmt node);
void print_switch_case(SwitchCase node);
void print_case_item(CaseItem node);
void print_if_(IfStmt node);
void print_for_body(ForBody node);
void print_assign(Assign node);
void print_assign_eq(AssignEq node);
void print_rval(Rval node);
void print_function(Function node);
void print_param_item(ParamItem node);
void print_enum_item(EnumItem node);
void print_type(Type node);
void print_type_no_ident(TypeNoIdent node);
void print_expr(Expr node);
void print_lval(Lval node);

void print_program(Program node) {
  padded_puts("Program {", 2);
  da_foreach(Stmt, it, &node.stmts) {
    padded_puts("\n");
    print_stmt(*it);
  }
  padded_puts("}", -2);
}
void print_stmt(Stmt node) {
  // padded_puts("Stmt {");
  switch (node.type) {
  case NODE_ASSIGN:
    print_assign(node.as.assign);
    break;
  case NODE_CALL:
    padded_puts("Call {\n", 2);
    print_lval(node.as.call.lval);
    da_foreach(Rval, it, &node.as.call.params) {
      if ((int)(it - node.as.call.params.items) % 4 == 0)
        padded_puts("\n");
      print_rval(*it);
    }
    padded_puts("}", -2);
    break;
  case NODE_FLOW_STMT:
    padded_puts("Flow(");
    padded_puts(temp_sprintf(SV_Fmt, SV_Arg(node.as.flow_stmt.which.lexeme)));
    padded_puts(") {");
    if (node.as.flow_stmt.rval)
      print_rval(*node.as.flow_stmt.rval);
    padded_puts("}");
    break;
  case NODE_IF:
    print_if_(node.as.if_);
    break;
  case NODE_FOR:
    padded_puts("For {\n", 2);
    print_for_body(node.as.for_.head);
    padded_puts("\n");
    print_stmt(*node.as.for_.stmt);
    padded_puts("}", -2);
    break;
  case NODE_BLOCK:
    print_program(*node.as.block);
    break;
  case NODE_SWITCH:
    padded_puts("Switch {\n", 2);
    print_expr(node.as.switch_.expr);
    iterate(&node.as.switch_.case_) {
      Program body =
          node.as.switch_.body.items[it - node.as.switch_.case_.items];
      padded_puts("\n");
      print_switch_case(*it);
      padded_puts("\n");
      print_program(body);
    }
    padded_puts("}", -2);
    break;
  default:
    padded_puts(temp_sprintf("Stmt {%s}", node_type_to_string(node.type)));
  }
}
void print_switch_case(SwitchCase node) {
  if (node.default_)
    return (void)printf("Default {}");
  padded_puts("Case {", 2);
  iterate(&node.items) {
    if (node.items.count > 1)
      padded_puts("\n");
    print_case_item(*it);
  }
  padded_puts("}", -2);
}
void print_case_item(CaseItem node) {
  print_rval(node.rval);
  if (node.if_expr) {
    padded_puts("\n");
    padded_puts("if: ", 2);
    print_expr(*node.if_expr);
    padded_puts("", -2);
  }
}
void print_if_(IfStmt node) {
  switch (node.type) {
  case NODE_IF:
    padded_puts("If {\n", 2);
    print_expr(node.as.if_.expr);
    padded_puts("\n");
    print_program(*node.as.if_.program);
    padded_puts("}", -2);
    break;
  case NODE_ELSE: {
    bool first = true;
    iterate(&node.as.else_) {
      if (!first)
        padded_puts("\nElse ");
      else
        first = false;
      print_if_(*it);
    }
    break;
  }
  case NODE_TAIL:
    print_if_(*node.as.tail.start);
    padded_puts("\nElse ");
    print_program(*node.as.tail.end);
    break;
  default:
    UNREACHABLE(temp_sprintf("%s", node_type_to_string(node.type)));
  }
}
void print_for_body(ForBody node) {
  padded_puts("ForBody {", 2);
  if (node.start || node.end_type != NODE_INVALID)
    padded_puts("\n");
  if (node.start) {
    print_assign(*node.start);
    padded_puts("\n");
  }
  print_expr(node.expr);
  if (node.end_type != NODE_INVALID) {
    padded_puts("\n");
    switch (node.end_type) {
    case NODE_EXPR:
      print_expr(*node.end.expr);
      break;
    case NODE_ASSIGN_EQ:
      print_assign_eq(*node.end.assign_eq);
      break;
    default:
      UNREACHABLE(temp_sprintf("%s", node_type_to_string(node.end_type)));
    }
    padded_puts("\n", -2);
  } else
    padded_puts("", -2);
  padded_puts("}");
}
void print_assign(Assign node) {
  printf("Assign(%s) {", node_type_to_string(node.type));
  padded_puts("\ntarget: ", 2);
  print_lval(node.lval);
  switch (node.type) {
  case NODE_FULL_ASSIGN:
  case NODE_FULL_CONST_ASSIGN:
    padded_puts("\ntype:   ");
    print_type(node.typ);
    goto val;
  case NODE_ASSIGN_EQ:
    padded_puts("\nkind:   ");
    printf(SV_Fmt, SV_Arg(node.assign_eq_kind.lexeme));
    // falls through
  val:;
  case NODE_IMPLIED_CONST_ASSIGN:
  case NODE_IMPLIED_ASSIGN:
  case NODE_VAL_ASSIGN:
    padded_puts("\nvalue:  ");
    print_rval(node.rval);
    break;
  default:
    padded_puts(temp_sprintf("%s", node_type_to_string(node.type)));
  }
  padded_puts("}", -2);
}
void print_assign_eq(AssignEq node) {
  padded_puts("Assign(ASSIGN_EQ) {", 2);
  padded_puts("\ntarget: ");
  print_lval(node.lval);
  padded_puts("\nkind:   ");
  printf(SV_Fmt, SV_Arg(node.kind.lexeme));
  padded_puts("\nvalue:  ");
  print_rval(node.rval);
  padded_puts("\n}", -2);
}
void print_rval(Rval node) {
  padded_puts("Rval {");
  switch (node.type) {
  case NODE_EXPR:
    print_expr(node.as.expr);
    break;
  case NODE_TYPE:
    print_type_no_ident(node.as.type);
    break;
  case NODE_FUNCTION:
    print_function(node.as.function);
    break;
  case NODE_STRUCT_LITERAL:
    printf("Literal");
    if (node.as.struct_literal.type)
      printf("(" SV_Fmt ")", SV_Arg(node.as.struct_literal.type->source));
    padded_puts(" {", 2);
    iterate(&node.as.struct_literal.items) {
      padded_puts("\n");
      print_enum_item(*it);
    }
    padded_puts("\n}", -2);
    break;
  case NODE_SWITCH:
    padded_puts("Switch {\n", 2);
    print_expr(node.as.switch_.expr);
    iterate(&node.as.switch_.case_) {
      int i = it - node.as.switch_.case_.items;
      padded_puts("\n");
      print_switch_case(*it);
      padded_puts(" -> ");
      print_rval(node.as.switch_.rval.items[i]);
    }
    padded_puts("}", -2);
    break;
  default:
    padded_puts(temp_sprintf("%s", node_type_to_string(node.type)));
  }
  padded_puts("}");
}
void print_function(Function node) {
  padded_puts("Function {\n", 2);
  iterate(&node.params) {
    print_param_item(*it);
    padded_puts("\n");
  }
  if (node.result) {
    print_type(*node.result);
    padded_puts("\n");
  }
  print_program(*node.program);
  padded_puts("\n}", -2);
}
void print_param_item(ParamItem node) {
  bool first = true;
  iterate(&node.idents) {
    if (!first)
      padded_puts("\n");
    else
      first = false;
    String_View sv = sv_trim(it->lexeme);
    printf("Param {" SV_Fmt ", ", SV_Arg(sv));
    print_type(node.type);
    printf("}");
  }
}
void print_enum_item(EnumItem node) {
  padded_puts("EnumItem {");
  printf(SV_Fmt, SV_Arg(node.ident.lexeme));
  if (node.expr) {
    padded_puts(", ");
    print_expr(*node.expr);
  }
  padded_puts("}");
}
void print_type(Type node) {
  padded_puts("Type {");
  switch (node.type) {
  case NODE_ID:
  case NODE_SLICE:
  case NODE_PTR:
    String_View sv = sv_trim(node.source);
    printf(SV_Fmt, SV_Arg(sv));
    break;
  case NODE_FUNCTION:
    padded_puts("Function {\nparams:", 2);
    padded_puts("", 2);
    iterate(&node.as.function.params) {
      padded_puts("\n");
      print_type(*it);
    }
    padded_puts("\n", -2);
    padded_puts("ret:\n", 2);
    if (node.as.function.result)
      print_type(*node.as.function.result);
    padded_puts("\n}", -4);
    break;
  default:
    printf("%s", node_type_to_string(node.type));
  }
  padded_puts("}");
}
void print_type_no_ident(TypeNoIdent node) {
  padded_puts("TypeNI {");
  switch (node.type) {
  case NODE_ENUM:
    padded_puts("Enum {", 2);
    if (node.as.enum_.type) {
      padded_puts("\ntype: ");
      print_lval(*node.as.enum_.type);
    }
    iterate(&node.as.enum_.items) {
      padded_puts("\n");
      print_enum_item(*it);
    }
    padded_puts("\n}", -2);
    break;
  case NODE_STRUCT:
    padded_puts("Struct {", 2);
    iterate(&node.as.struct_) {
      padded_puts("\n");
      print_param_item(*it);
    }
    padded_puts("\n}", -2);
    break;
  default:
    padded_puts(node_type_to_string(node.type));
  }
  padded_puts("}");
}
void print_expr(Expr node) {
  // padded_puts("Expr ");
  switch (node.type) {
  case NODE_STRING:
    padded_puts("String {", 2);
    if (node.as.string.count > 1) {
      iterate(&node.as.string)
          padded_puts(temp_sprintf("\n\"" SV_Fmt "\"", SV_Arg(it->lexeme)));
      padded_puts("\n}", -2);
    } else {
      padded_puts(temp_sprintf("\"" SV_Fmt "\"",
                               SV_Arg(node.as.string.items[0].lexeme)));
      padded_puts("}", -2);
    }
    break;
  case NODE_VARIABLE:
    print_lval(node.as.variable);
    break;
  case NODE_CALL:
    padded_puts("Call {\n", 2);
    print_expr(*node.as.postfix.expr);
    iterate(&node.as.postfix.call) {
      padded_puts("\n");
      print_rval(*it);
    }
    padded_puts("}", -2);
    break;
  case NODE_LITERAL:
    printf("Literal(%s) " SV_Fmt, token_type_to_string(node.as.literal.type),
           SV_Arg(node.as.literal.lexeme));
    break;
  case NODE_ENUM_VAL:
    printf("." SV_Fmt, SV_Arg(node.as.enum_val.lexeme));
    break;
  case NODE_LOGICAL_NOT:
    printf("LogicalNot ");
    goto prefix;
  case NODE_NEGATIVE:
    printf("Neg ");
    goto prefix;
  case NODE_PTR:
    printf("Ptr ");
  prefix:;
    print_expr(*node.as.prefix);
    break;
  case NODE_DEREF:
    printf("Deref(%d) ", node.as.postfix.deref_count);
    print_expr(*node.as.postfix.expr);
    break;
  case NODE_INDEXING:
    padded_puts("Index {\n", 2);
    print_expr(*node.as.postfix.expr);
    padded_puts("\n");
    print_rval(*node.as.postfix.indexing);
    padded_puts("}", -2);
    break;
  case NODE_CAST:
    padded_puts("Cast {\n", 2);
    print_expr(*node.as.postfix.expr);
    padded_puts("\n");
    print_type(*node.as.postfix.cast);
    padded_puts("}", -2);
    break;
  case NODE_RANGE:
    padded_puts(temp_sprintf("Range%s {",
                             node.as.postfix.range.inclusive ? "Inc" : ""));
    if (node.as.postfix.range.from) {
      printf("from: ");
      print_rval(*node.as.postfix.range.from);
    }
    if (node.as.postfix.range.to) {
      printf("to: ");
      print_rval(*node.as.postfix.range.to);
    }
    padded_puts("}");
    break;
  default:
    if (expr_precedence(&node) > 2) {
      // binop
      padded_puts(temp_sprintf("Binop(%s) {\n", node_type_to_string(node.type)),
                  2);
      print_expr(*node.as.binop.left);
      padded_puts("\n");
      print_expr(*node.as.binop.right);
      padded_puts("}", -2);
      break;
    }
    printf("Expr {%s}", node_type_to_string(node.type));
  }
}
void print_lval(Lval node) {
  padded_puts("Lval {");
  switch (node.type) {
  case NODE_ID:
    printf(SV_Fmt, SV_Arg(node.as.id.lexeme));
    break;
  case NODE_FIELD:
    bool first = true;
    iterate(&node.as.field) {
      if (first)
        first = false;
      else
        printf(".");
      printf(SV_Fmt, SV_Arg(it->lexeme));
    }
    break;
  default:
    padded_puts(temp_sprintf("%s", node_type_to_string(node.type)));
  }
  padded_puts("}");
}

int dbg_lvl = 0;
#define print_node(node)                                                       \
  (padded_puts(temp_sprintf("%d\n", dbg_lvl -= 1), -2),                        \
   _Generic((node),                                                            \
       Program: print_program,                                                 \
       Stmt: print_stmt,                                                       \
       SwitchCase: print_switch_case,                                          \
       CaseItem: print_case_item,                                              \
       IfStmt: print_if_,                                                      \
       ForBody: print_for_body,                                                \
       Assign: print_assign,                                                   \
       AssignEq: print_assign_eq,                                              \
       Rval: print_rval,                                                       \
       Function: print_function,                                               \
       ParamItem: print_param_item,                                            \
       EnumItem: print_enum_item,                                              \
       Type: print_type,                                                       \
       TypeNoIdent: print_type_no_ident,                                       \
       Expr: print_expr,                                                       \
       Lval: print_lval)(node),                                                \
   putchar('\n'))

void free_program(Program program);
void free_stmt(Stmt stmt);
void free_switch_case(SwitchCase switch_case);
void free_case_item(CaseItem case_item);
void free_if_(IfStmt if_);
void free_for_body(ForBody for_body);
void free_assign(Assign assign);
void free_assign_eq(AssignEq assign_eq);
void free_rval(Rval rval);
void free_function(Function function);
void free_param_item(ParamItem param_item);
void free_enum_item(EnumItem enum_item);
void free_type(Type type);
void free_type_no_ident(TypeNoIdent type_no_ident);
void free_expr(Expr expr);
void free_lval(Lval lval);

void free_program(Program program) {
  da_foreach(Stmt, it, &program.stmts) free_stmt(*it);
  da_free(program.stmts);
}
void free_stmt(Stmt stmt) {
  switch (stmt.type) {
  case NODE_ASSIGN:
    return free_assign(stmt.as.assign);
  case NODE_CALL:
    free_lval(stmt.as.call.lval);
    da_foreach(Rval, it, &stmt.as.call.params) free_rval(*it);
    da_free(stmt.as.call.params);
    return;
  case NODE_IF:
    return free_if_(stmt.as.if_);
  case NODE_SWITCH:
    free_expr(stmt.as.switch_.expr);
    da_foreach(SwitchCase, it, &stmt.as.switch_.case_) free_switch_case(*it);
    da_foreach(Program, it, &stmt.as.switch_.body) free_program(*it);
    da_free(stmt.as.switch_.case_);
    da_free(stmt.as.switch_.body);
    return;
  case NODE_FLOW_STMT:
    if (stmt.as.flow_stmt.rval != NULL) {
      free_rval(*stmt.as.flow_stmt.rval);
      free(stmt.as.flow_stmt.rval);
    }
    return;
  case NODE_BLOCK:
    free_program(*stmt.as.block);
    free(stmt.as.block);
    return;
  case NODE_LOOP:
    free_program(*stmt.as.loop);
    free(stmt.as.loop);
    return;
  case NODE_FOR:
    free_for_body(stmt.as.for_.head);
    free_stmt(*stmt.as.for_.stmt);
    free(stmt.as.for_.stmt);
    return;
  case NODE_INVALID:
    return;
  default:
    UNREACHABLE(temp_sprintf("%s", node_type_to_string(stmt.type)));
  }
}
void free_switch_case(SwitchCase switch_case) {
  if (!switch_case.default_) {
    da_foreach(CaseItem, it, &switch_case.items) free_case_item(*it);
    da_free(switch_case.items);
  }
}
void free_case_item(CaseItem case_item) {
  free_rval(case_item.rval);
  if (case_item.if_expr != NULL) {
    free_expr(*case_item.if_expr);
    free(case_item.if_expr);
  }
}
void free_if_(IfStmt if_) {
  switch (if_.type) {
  case NODE_IF:
    free_expr(if_.as.if_.expr);
    free_program(*if_.as.if_.program);
    free(if_.as.if_.program);
    return;
  case NODE_ELSE:
    da_foreach(IfStmt, it, &if_.as.else_) free_if_(*it);
    da_free(if_.as.else_);
    return;
  case NODE_TAIL:
    if (if_.as.tail.start) {
      free_if_(*if_.as.tail.start);
      free(if_.as.tail.start);
    }
    if (if_.as.tail.end) {
      free_program(*if_.as.tail.end);
      free(if_.as.tail.end);
    }
    return;
  default:
    UNREACHABLE(temp_sprintf("%s", node_type_to_string(if_.type)));
  }
}
void free_for_body(ForBody for_body) {
  if (for_body.start != NULL) {
    free_assign(*for_body.start);
    free(for_body.start);
  }
  free_expr(for_body.expr);
  switch (for_body.end_type) {
  case NODE_ASSIGN_EQ:
    free_assign_eq(*for_body.end.assign_eq);
    free(for_body.end.assign_eq);
    return;
  case NODE_EXPR:
    free_expr(*for_body.end.expr);
    free(for_body.end.expr);
    return;
  default:
  }
}
void free_assign(Assign assign) {
  free_lval(assign.lval);
  switch (assign.type) {
  case NODE_TYPE_ASSIGN:
    free_type(assign.typ);
    return;
  case NODE_FULL_ASSIGN:
  case NODE_FULL_CONST_ASSIGN:
    free_type(assign.typ);
  case NODE_IMPLIED_ASSIGN:
  case NODE_IMPLIED_CONST_ASSIGN:
  case NODE_VAL_ASSIGN:
  case NODE_ASSIGN_EQ:
    free_rval(assign.rval);
  case NODE_INVALID:
    return;
  default:
    UNREACHABLE(temp_sprintf("%s", node_type_to_string(assign.type)));
  }
}
void free_assign_eq(AssignEq assign_eq) {
  free_lval(assign_eq.lval);
  free_rval(assign_eq.rval);
}
void free_rval(Rval rval) {
  switch (rval.type) {
  case NODE_EXPR:
    return free_expr(rval.as.expr);
  case NODE_FUNCTION:
    return free_function(rval.as.function);
  case NODE_STRUCT_LITERAL:
    if (rval.as.struct_literal.type != NULL) {
      free_lval(*rval.as.struct_literal.type);
      free(rval.as.struct_literal.type);
    }
    da_foreach(EnumItem, it, &rval.as.struct_literal.items) free_enum_item(*it);
    da_free(rval.as.struct_literal.items);
    return;
  case NODE_TYPE:
    return free_type_no_ident(rval.as.type);
  case NODE_SWITCH:
    free_expr(rval.as.switch_.expr);
    da_foreach(SwitchCase, it, &rval.as.switch_.case_) free_switch_case(*it);
    da_foreach(Rval, it, &rval.as.switch_.rval) free_rval(*it);
    da_free(rval.as.switch_.case_);
    da_free(rval.as.switch_.rval);
    return;
  case NODE_IF:
    free_rval(*rval.as.if_.val);
    free(rval.as.if_.val);
    free_expr(rval.as.if_.expr);
    free_rval(*rval.as.if_.else_val);
    free(rval.as.if_.else_val);
    return;
  case NODE_INVALID:
    return;
  default:
    UNREACHABLE(temp_sprintf("%s", node_type_to_string(rval.type)));
  }
}
void free_function(Function function) {
  da_foreach(ParamItem, it, &function.params) free_param_item(*it);
  da_free(function.params);
  if (function.result != NULL) {
    free_type(*function.result);
    free(function.result);
  }
  if (function.program)
    free_program(*function.program);
  free(function.program);
}
void free_param_item(ParamItem param_item) {
  da_free(param_item.idents);
  free_type(param_item.type);
}
void free_enum_item(EnumItem enum_item) {
  if (enum_item.expr != NULL) {
    free_expr(*enum_item.expr);
    free(enum_item.expr);
  }
}
void free_type(Type type) {
  switch (type.type) {
  case NODE_INVALID:
  case NODE_ID:
    return;
  case NODE_OTHER:
    return free_type_no_ident(type.as.other);
  case NODE_TUPLE:
    da_foreach(Type, it, &type.as.tuple) free_type(*it);
    da_free(type.as.tuple);
    return;
  case NODE_SLICE:
    free_type(*type.as.slice);
    free(type.as.slice);
    return;
  case NODE_PTR:
    free_type(*type.as.ptr);
    free(type.as.ptr);
    return;
  case NODE_FUNCTION:
    da_foreach(Type, it, &type.as.function.params) free_type(*it);
    free_type(*type.as.function.result);
    da_free(type.as.function.params);
    free(type.as.function.result);
    return;
  default:
    UNREACHABLE(temp_sprintf("%s", node_type_to_string(type.type)));
  }
}
void free_type_no_ident(TypeNoIdent type_no_ident) {
  switch (type_no_ident.type) {
  case NODE_UNKNOWN:
    return;
  case NODE_ENUM:
    if (type_no_ident.as.enum_.type != NULL) {
      free_lval(*type_no_ident.as.enum_.type);
      free(type_no_ident.as.enum_.type);
    }
    da_foreach(EnumItem, it, &type_no_ident.as.enum_.items) free_enum_item(*it);
    da_free(type_no_ident.as.enum_.items);
    return;
  case NODE_STRUCT:
    da_foreach(ParamItem, it, &type_no_ident.as.struct_) free_param_item(*it);
    da_free(type_no_ident.as.struct_);
    return;
  case NODE_INVALID:
    return;
  default:
    UNREACHABLE(temp_sprintf("%s", node_type_to_string(type_no_ident.type)));
  }
}
void free_expr(Expr expr) {
  switch (expr.type) {
  case NODE_ENUM_VAL:
  case NODE_LITERAL:
    return;
  case NODE_STRING:
    return da_free(expr.as.string);
  case NODE_VARIABLE:
    return free_lval(expr.as.variable);
  case NODE_CAST:
    if (expr.as.postfix.cast) {
      free_type(*expr.as.postfix.cast);
      free(expr.as.postfix.cast);
    }
    goto postfix_free;
  case NODE_INDEXING:
    free_rval(*expr.as.postfix.indexing);
    free(expr.as.postfix.indexing);
    goto postfix_free;
  case NODE_CALL:
    da_foreach(Rval, it, &expr.as.postfix.call) free_rval(*it);
    da_free(expr.as.postfix.call);
    goto postfix_free;
  case NODE_RANGE:
    if (expr.as.postfix.range.from) {
      free_rval(*expr.as.postfix.range.from);
      free(expr.as.postfix.range.from);
    }
    if (expr.as.postfix.range.to) {
      free_rval(*expr.as.postfix.range.to);
      free(expr.as.postfix.range.to);
    }
    // falls through
  case NODE_NEGATIVE:
  case NODE_LOGICAL_NOT:
  case NODE_DEREF:
  postfix_free:;
    if (expr.as.postfix.expr) {
      free_expr(*expr.as.postfix.expr);
      free(expr.as.postfix.expr);
    }
    return;
  case NODE_TIMES:
  case NODE_DIVIDE:
  case NODE_MOD:
  case NODE_BIT_AND:
  case NODE_PLUS:
  case NODE_MINUS:
  case NODE_BIT_OR:
  case NODE_BIT_NOT:
  case NODE_EQUAL:
  case NODE_NOT_EQUAL:
  case NODE_LT:
  case NODE_GT:
  case NODE_LE:
  case NODE_GE:
  case NODE_LOGICAL_OR:
  case NODE_LOGICAL_AND:
    free_expr(*expr.as.binop.left);
    free_expr(*expr.as.binop.right);
    free(expr.as.binop.left);
    free(expr.as.binop.right);
    return;
  case NODE_INVALID:
    return;
  default:
    UNREACHABLE(temp_sprintf("%s", node_type_to_string(expr.type)));
  }
}
void free_lval(Lval lval) {
  if (lval.type == NODE_FIELD)
    da_free(lval.as.field);
}

#define free_node(node)                                                        \
  _Generic((node),                                                             \
      Program: free_program,                                                   \
      Stmt: free_stmt,                                                         \
      SwitchCase: free_switch_case,                                            \
      CaseItem: free_case_item,                                                \
      IfStmt: free_if_,                                                        \
      ForBody: free_for_body,                                                  \
      Assign: free_assign,                                                     \
      AssignEq: free_assign_eq,                                                \
      Rval: free_rval,                                                         \
      Function: free_function,                                                 \
      ParamItem: free_param_item,                                              \
      EnumItem: free_enum_item,                                                \
      Type: free_type,                                                         \
      TypeNoIdent: free_type_no_ident,                                         \
      Expr: free_expr,                                                         \
      Lval: free_lval)(node)

// #define node_type_to_enum_val(type) \
//   _Generic(type, \
//                                                                                \
//       default: __builtin_trap())

union AstNode {
  Program program;
  Stmt stmt;
  SwitchCase switch_case;
  CaseItem case_item;
  IfStmt if_;
  ForBody for_body;
  Assign assign;
  AssignEq assign_eq;
  Rval rval;
  Function function;
  ParamItem param_item;
  EnumItem enum_item;
  Type type;
  TypeNoIdent type_no_ident;
  Expr expr;
  Lval lval;
};

#ifdef PARSER_DEBUG
int __parse(Lexer *lexer, AstNode *result, bool (**parsers)(Lexer *, void *),
            size_t parsers_count, const char **current_items,
            const char *caller_name, const char *file_name, int file_line)
#else
int __parse(Lexer *lexer, AstNode *result, bool (**parsers)(Lexer *, void *),
            size_t parsers_count)
#endif // PARSER_DEBUG
{
  AstNode ast_node = {0};
#ifdef PARSER_DEBUG
  char *buf = "";
  for (size_t i = 0; i < parsers_count; i++)
    buf = temp_sprintf("%s %s", buf, current_items[i]);
  nob_log(INFO, "\\ %s: Searching nodes:%s", caller_name, buf);
#endif // PARSER_DEBUG
  choice_depth += 1;
  size_t i = 0;
  for (; i < parsers_count; ++i) {
    Lexer save = *lexer;
    if (parsers[i](&save, &ast_node)) {
#ifdef PARSER_DEBUG
      nob_log(INFO, "/ %s: Found node %s '%.*s'", caller_name, current_items[i],
              (int)ast_node.program.source.count, ast_node.program.source.data);
#endif // PARSER_DEBUG
      *lexer = save;
      *result = ast_node;
      choice_depth -= 1;
      return i;
    }
  }
  choice_depth -= 1;
#ifdef PARSER_DEBUG
  nob_log(WARNING, "/ %s:%d: %s: Expected node from: %s", file_name, file_line,
          caller_name, buf);
#endif // PARSER_DEBUG
  if (!choice_depth)
    __builtin_trap();
  return -1;
}

#ifdef PARSER_DEBUG
int __consume_str(Lexer *lexer, TokenType type, const char **strs,
                  size_t strs_count, const char *current_items,
                  const char *caller_name, const char *file_name, int file_line)
#else
int __consume_str(Lexer *lexer, TokenType type, const char **strs,
                  size_t strs_count)
#endif // PARSER_DEBUG
{
#ifdef PARSER_DEBUG
  nob_log(INFO, "\\ %s: Searching %s from: %s", caller_name,
          token_type_to_string(type), current_items);
#endif // PARSER_DEBUG
  while (lexer->current_token.type != type) {
    if (lexer->current_token.type == TOKEN_COMMENT) {
      lexer_next_token(lexer);
      continue;
    }
#ifdef PARSER_DEBUG
    nob_log(WARNING, "/ %s:%d: %s: Expected %s, got %s", file_name, file_line,
            caller_name, token_type_to_string(type),
            token_type_to_string(lexer->current_token.type));
#endif // PARSER_DEBUG
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
        str += copy.current_token.lexeme.count;
        len -= copy.current_token.lexeme.count;
        lexer_next_token(&copy);
      }
      if (!len) {
        str = strs[i];
        len = strlen(str);
#ifdef PARSER_DEBUG
        nob_log(INFO, "/ %s: Found %s: \"%s\"", caller_name,
                token_type_to_string(type), str);
#endif // PARSER_DEBUG
        *lexer = copy;
        choice_depth -= 1;
        return i;
      }
    } else {
      if (lexer->current_token.lexeme.count != len ||
          !sv_starts_with(lexer->current_token.lexeme, (String_View){len, str}))
        continue;
#ifdef PARSER_DEBUG
      nob_log(INFO, "/ %s: Found %s: \"%s\"", caller_name,
              token_type_to_string(type), str);
#endif // PARSER_DEBUG
      lexer_next_token(lexer);
      choice_depth -= 1;
      return i;
    }
  }
#ifdef PARSER_DEBUG
  nob_log(WARNING, "/ %s:%d: %s: Found no %s from: %s", file_name, file_line,
          caller_name, token_type_to_string(type), current_items);
#endif // PARSER_DEBUG
  if (!--choice_depth)
    __builtin_trap(); // TODO: exit properly
  return -1;
}

// AstNode *parse_multiword_type(Lexer *lexer) {
//   AstNode type = new_node(MultiwordType, lexer);
//   String_View str = cast(MultiwordType *, type.node)->str;
//   Lexer copy = *lexer;

//   consume_str(lexer, SYMBOL, "(");
//   str.data = lexer->current_token.lexeme.data;
//   consume_tok(lexer, IDENTIFIER);
//   for (;;) {
//     str.count = lexer->current_token.lexeme.data +
//                 lexer->current_token.lexeme.count - str.data;
//     consume_tok(lexer, IDENTIFIER);
//     if (consume_str_opt(lexer, SYMBOL, ")") == 0)
//       break;
//   }
//   cast(MultiwordType *, type.node)->str = str;

//   return compose_ast_node(lexer, type);
// throw:;
//         *lexer = copy;
//         free_node(&type, true);
//         return NULL;
// }

// AstNode *parse_type(Lexer *lexer) {
//         AstNode *ast_buf, type_ast = new_node(Type, lexer);
//         Type *type = type_ast.node;

//         int res = consume_str_opt(lexer, IDENTIFIER, "unique");
//         if (res == 0)
//           type->unique = true;

//         parse_or_throw(lexer, ast_buf, multiword_type, identifier);
//         type->item = ast_buf;

//         return compose_ast_node(lexer, type_ast);
//       throw:;
//         free_node(&type_ast, true);
//         return NULL;
// }

// AstNode *parse_value(Lexer *lexer) {
//         AstNode value_ast = new_node(Value, lexer);
//         Value *value = value_ast.node;

//         switch (lexer->current_token.type) {
//         case TOKEN_IDENTIFIER:
//           value->kind = VALUE_IDENTIFIER;
//           value->as.identifier = lexer->current_token.lexeme;
//           break;
//         case TOKEN_STRING:
//           value->kind = VALUE_STRING;
//           value->as.string = lexer->current_token.lexeme;
//           break;
//         case TOKEN_INT:
//           value->kind = VALUE_INT;
//           value->as.number = lexer->current_token.lexeme;
//           break;
//         case TOKEN_INTU:
//           value->kind = VALUE_INTU;
//           value->as.number = lexer->current_token.lexeme;
//           break;
//         case TOKEN_INTL:
//           value->kind = VALUE_INTL;
//           value->as.number = lexer->current_token.lexeme;
//           break;
//         case TOKEN_INTUL:
//           value->kind = VALUE_INTUL;
//           value->as.number = lexer->current_token.lexeme;
//           break;
//         case TOKEN_INTLL:
//           value->kind = VALUE_INTLL;
//           value->as.number = lexer->current_token.lexeme;
//           break;
//         case TOKEN_INTULL:
//           value->kind = VALUE_INTULL;
//           value->as.number = lexer->current_token.lexeme;
//           break;
//         case TOKEN_FLOAT32:
//           value->kind = VALUE_FLOAT32;
//           value->as.number = lexer->current_token.lexeme;
//           break;
//         case TOKEN_FLOAT64:
//           value->kind = VALUE_FLOAT64;
//           value->as.number = lexer->current_token.lexeme;
//           break;
//         case TOKEN_FLOAT128:
//           value->kind = VALUE_FLOAT128;
//           value->as.number = lexer->current_token.lexeme;
//           break;
//         case TOKEN_CHAR:
//           value->kind = VALUE_CHAR;
//           value->as.number = lexer->current_token.lexeme;
//           break;
//         case TOKEN_TRUE:
//           value->kind = VALUE_TRUE;
//           value->as.truth = lexer->current_token.lexeme;
//           break;
//         case TOKEN_FALSE:
//           value->kind = VALUE_FALSE;
//           value->as.falth = lexer->current_token.lexeme;
//           break;
//         default:;
//         }
//         consume_tok(lexer, IDENTIFIER, STRING, INT, INTU, INTL, INTUL, INTLL,
//                     INTULL, FLOAT32, FLOAT64, FLOAT128, TRUE, FALSE, CHAR);

//         return compose_ast_node(lexer, value_ast);
//       throw:;
//         free_node(&value_ast, true);
//         return NULL;
// }

// AstNode *parse_identifier(Lexer *lexer) {
//         AstNode identifier = new_node(Identifier, lexer);

//         cast(Identifier *, identifier.node)->str =
//         lexer->current_token.lexeme; consume_tok(lexer, IDENTIFIER);

//         return compose_ast_node(lexer, identifier);
//       throw:;
//         free_node(&identifier, true);
//         return NULL;
// }

// AstNode *parse_binary_expression(Lexer *lexer);

// AstNode *parse_binary_expression_tail(Lexer *lexer, AstNode *left) {
//         AstNode *ast_buf, bin_expression_ast;
//         Lexer lexer_copy = *lexer;

//         for (;;) {
//           bin_expression_ast = new_node(BinaryExpression, &lexer_copy);
//           BinaryExpression *bin_expression = bin_expression_ast.node;

//           bin_expression->kind =
//               consume_str(&lexer_copy, SYMBOL, BIN_EXPR_STRS);
//           parse_or_throw(&lexer_copy, ast_buf, binary_expression);

//           // TODO: manage precedence

//           bin_expression_ast.source.data = left->source.data;
//           bin_expression->left = left;
//           bin_expression->right = ast_buf;
//           left = compose_ast_node(&lexer_copy, bin_expression_ast);
//           *lexer = lexer_copy;
//         }

//       throw:;
//         free_node(&bin_expression_ast, true);
//         return left;
// }

// AstNode *parse_binary_expression(Lexer *lexer) {
//         AstNode *ast_buf;
//         parse_or_throw(lexer, ast_buf, value);
//         return parse_binary_expression_tail(lexer, ast_buf);
//       throw:;
//         return NULL;
// }

// AstNode *parse_expression(Lexer *lexer);

// AstNode *parse_paren_expression(Lexer *lexer) {
//         AstNode *ast_buf;

//         consume_str(lexer, SYMBOL, "(");
//         parse_or_throw(lexer, ast_buf, expression);
//         consume_str(lexer, SYMBOL, ")");

//         return ast_buf;
//       throw:;
//         return NULL;
// }

// AstNode *parse_expression(Lexer *lexer) {
//         AstNode *ast_buf, expression = new_node(Expression, lexer);

//         parse_or_throw(lexer, ast_buf, binary_expression);
//         cast(Expression *, expression.node)->item = ast_buf;

//         return compose_ast_node(lexer, expression);
//       throw:;
//         free_node(&expression, true);
//         return NULL;
// }

// /*
//   Parses full assignments.
//   BNF: TODO
//   EXAPMLES: ```
//     name : type : val
//     name : type = val
//   ```*/
// AstNode *parse_full_assignment(Lexer *lexer) {
//         AstNode *ast_buf,
//             full_assignment_ast = new_node(FullAssignment, lexer);
//         FullAssignment *full_assignment = full_assignment_ast.node;

//         parse_or_throw(lexer, ast_buf, identifier);
//         full_assignment->target = ast_buf;

//         consume_str(lexer, SYMBOL, ":");

//         parse_or_throw(lexer, ast_buf, type);
//         full_assignment->type = ast_buf;

//         if (consume_str(lexer, SYMBOL, ":", "=") == 0) {
//           full_assignment_ast.type = NODE_FULL_CONST_ASSIGNMENT;
//         }

//         parse_or_throw(lexer, ast_buf, expression);
//         full_assignment->value = ast_buf;

//         return compose_ast_node(lexer, full_assignment_ast);
//       throw:;
//         free_node(&full_assignment_ast, true);
//         return NULL;
// }

// AstNode *parse_implied_assignment(Lexer *lexer) {
//         AstNode *ast_buf,
//             implied_assignment_ast = new_node(ImpliedAssignment, lexer);
//         ImpliedAssignment *implied_assignment = implied_assignment_ast.node;

//         parse_or_throw(lexer, ast_buf, identifier);
//         implied_assignment->left = ast_buf;

//         bool is_const = consume_str(lexer, SYMBOL, ":=", "::");
//         implied_assignment->is_const = is_const;

//         if (is_const)
//           parse_or_throw(lexer, ast_buf, type, expression, identifier);
//         else
//           parse_or_throw(lexer, ast_buf, expression, identifier);
//         implied_assignment->right = ast_buf;

//         return compose_ast_node(lexer, implied_assignment_ast);
//       throw:;
//         free_node(&implied_assignment_ast, true);
//         return NULL;
// }

// AstNode *parse_type_or_const_assignment(Lexer *lexer) {
//         AstNode *ast_buf,
//             assignment = new_node(TypeOrConstAssignment, lexer);

//         parse_or_throw(lexer, ast_buf, identifier);
//         cast(ImpliedAssignment *, assignment.node)->left = ast_buf;

//         consume_str(lexer, SYMBOL, ":");

//         parse_or_throw(lexer, ast_buf, identifier, type, expression);
//         cast(ImpliedAssignment *, assignment.node)->right = ast_buf;
//         switch (ast_buf->type) {
//         case NODE_TYPE:
//           assignment.type = NODE_TYPE_ASSIGNMENT;
//           break;
//         case NODE_VALUE:
//           assignment.type = NODE_CONST_ASSIGNMENT;
//           break;
//         default:
//           assignment.type = NODE_TYPE_OR_CONST_ASSIGNMENT;
//         }

//         return compose_ast_node(lexer, assignment);
//       throw:;
//         free_node(&assignment, true);
//         return NULL;
// }

// AstNode *parse_value_assignment(Lexer *lexer) {
//         AstNode *ast_buf,
//             value_assignment = new_node(ValueAssignment, lexer);

//         parse_or_throw(lexer, ast_buf, identifier);
//         cast(ImpliedAssignment *, value_assignment.node)->left = ast_buf;

//         consume_str(lexer, SYMBOL, "=");

//         parse_or_throw(lexer, ast_buf, expression);
//         cast(ImpliedAssignment *, value_assignment.node)->right = ast_buf;

//         return compose_ast_node(lexer, value_assignment);
//       throw:;
//         free_node(&value_assignment, true);
//         return NULL;
// }

// AstNode *parse_assignment(Lexer *lexer) {
//         AstNode *ast_buf;

//         parse_or_throw(lexer, ast_buf, full_assignment, implied_assignment,
//                        type_or_const_assignment, value_assignment);

//         return ast_buf;
//       throw:;
//         return NULL;
// }

// /*
//   Parses c like comments.
//   BNF: NO
//   EXAMPLES: ```
//     // single line comment
//     /\*
//       multiline comment
//     *\/
//     /\*
//       multiline /\*
//         nested
//       *\/ comment
//     *\/
//   ```*/
// AstNode *parse_comment(Lexer *lexer) {
//         AstNode comment = new_node(Comment, lexer);
//         cast(Comment *, comment.node)->str = lexer->current_token.lexeme;
//         consume_tok(lexer, COMMENT);
//         return compose_ast_node(lexer, comment);
//       throw:;
//         free_node(&comment, true);
//         return NULL;
// }

typedef struct ParseError {
  enum {
    ParseError_token,
    ParseError_node,
  } kind;
  char *expected;
} ParseError;

arr_field(const char *) parse_errors = {0};

bool parse_program(Lexer *lexer, void *result);
bool parse_stmt(Lexer *lexer, void *result);
bool parse_switch_case(Lexer *lexer, void *result);
bool parse_case_item(Lexer *lexer, void *result);
bool parse_if_stmt(Lexer *lexer, void *result);
bool parse_for_body(Lexer *lexer, void *result);
bool parse_assign(Lexer *lexer, void *result);
bool parse_assign_eq(Lexer *lexer, void *result);
bool parse_rval(Lexer *lexer, void *result);
bool parse_function(Lexer *lexer, void *result);
bool parse_param_item(Lexer *lexer, void *result);
bool parse_enum_item(Lexer *lexer, void *result);
bool parse_type(Lexer *lexer, void *result);
bool parse_type_no_ident(Lexer *lexer, void *result);
bool parse_expr(Lexer *lexer, void *result);
bool parse_lval(Lexer *lexer, void *result);

#define box(node)                                                              \
  ({                                                                           \
    auto t = node;                                                             \
    typeof(t) *tmp = malloc(sizeof(t));                                        \
    *tmp = (node);                                                             \
    tmp;                                                                       \
  })

bool parse_lval(Lexer *lexer, void *result) {
  debug_this();
  Lval node = new_node(Lval, lexer);
  node.as.id = lexer->current_token;
  if (lexer->current_token.type != TOKEN_IDENTIFIER)
    goto throw;
  lexer_next_token(lexer);
  node.type = NODE_ID;
  if (consume_str_opt(lexer, SYMBOL, ".") == 0) {
    if (lexer->current_token.type != TOKEN_IDENTIFIER) {
      lexer->pos -= 2;
      lexer_next_token(lexer);
    } else {
      Token buf = node.as.id;
      node.as.field.capacity = 0;
      node.as.field.count = 0;
      node.as.field.items = NULL;
      node.type = NODE_FIELD;
      da_init(&node.as.field, 2);
      da_append(&node.as.field, buf);
      da_append(&node.as.field, lexer->current_token);
      lexer_next_token(lexer);
      while (consume_str_opt(lexer, SYMBOL, ".") == 0) {
        if (lexer->current_token.type != TOKEN_IDENTIFIER) {
          lexer->pos -= 2;
          lexer_next_token(lexer);
          break;
        }
        da_append(&node.as.field, lexer->current_token);
        lexer_next_token(lexer);
      }
    }
  }
  *(Lval *)result = compose_node(node, lexer);
  nob_log(INFO, "ok lval");
  print_node(node);
  return true;
throw:;
  padded_puts("", -2);
  dbg_lvl -= 1;
  free_node(node);
  return false;
}

bool parse_expr(Lexer *lexer, void *result) {
  debug_this();
  AstNode ast_buf = {0};
  Expr node = new_node(Expr, lexer);
  switch (consume_str_opt(lexer, SYMBOL, ".", "(", "-", "!")) {
  case 0:
    node.type = NODE_ENUM_VAL;
    node.as.enum_val = lexer->current_token;
    consume_tok(lexer, IDENTIFIER);
    goto ret;
  case 1:
    if (!parse_expr(lexer, &node))
      goto throw;
    consume_str(lexer, SYMBOL, ")");
    node.parens = true;
    node.source.data -= 1;
    node.source.count += 2;
    break;
  case 2:
    node.type = NODE_NEGATIVE;
    goto prefix;
  case 3:
    node.type = NODE_LOGICAL_NOT;
  prefix:;
    node.as.prefix = malloc(sizeof(Expr));
    if (!parse_expr(lexer, node.as.prefix))
      goto throw;
    goto ret;
  default:
  }
  // parsed parens and prefixes
  switch (lexer->current_token.type) {
  case TOKEN_IDENTIFIER:
    if (sv_eq(lexer->current_token.lexeme, sv_from_cstr("true")) ||
        sv_eq(lexer->current_token.lexeme, sv_from_cstr("false")))
      goto lit;
    node.type = NODE_VARIABLE;
    if (!parse_lval(lexer, &node.as.variable))
      goto throw;
    break;
  case TOKEN_STRING:
    node.type = NODE_STRING;
    da_init(&node.as.string, 1);
    while (lexer->current_token.type == TOKEN_STRING) {
      da_append(&node.as.string, lexer->current_token);
      lexer_next_token(lexer);
    }
    break;
  case TOKEN_CHAR:
  case TOKEN_INT:
  case TOKEN_INTU:
  case TOKEN_INTL:
  case TOKEN_INTUL:
  case TOKEN_INTLL:
  case TOKEN_INTULL:
  case TOKEN_FLOAT32:
  case TOKEN_FLOAT64:
  case TOKEN_FLOAT128:
  lit:;
    node.type = NODE_LITERAL;
    node.as.literal = lexer->current_token;
    lexer_next_token(lexer);
    break;
  default:
    goto throw;
  }
  for (;;) {
    switch (consume_str_opt(lexer, SYMBOL, "(", ".*", ".", "[")) {
    case 0: // call
      node.as.postfix.expr = box(compose_node(node, lexer));
      node.as.postfix.expr->source.count -= 1;
      node.as.postfix.call.capacity = 0;
      node.as.postfix.call.count = 0;
      node.as.postfix.call.items = NULL;
      node.type = NODE_CALL;
      da_init(&node.as.postfix.call, 4);
      for (;;) {
        if (!parse_rval(lexer, &ast_buf))
          break;
        da_append(&node.as.postfix.call, ast_buf.rval);
        if (consume_str_opt(lexer, SYMBOL, ",") != 0)
          break;
      }
      consume_str(lexer, SYMBOL, ")");
      continue;
    case 1:
      node.as.postfix.expr = box(node);
      node.type = NODE_DEREF;
      node.as.postfix.deref_count = 1;
      while (consume_str_opt(lexer, SYMBOL, ".*") == 0)
        node.as.postfix.deref_count += 1;
      continue;
    case 2:
      node.as.postfix.expr = box(node);
      node.type = NODE_CAST;
      node.as.postfix.cast = NULL;
      consume_str(lexer, SYMBOL, "(");
      if (!parse_type(lexer, &ast_buf))
        goto throw;
      node.as.postfix.cast = box(ast_buf.type);
      consume_str(lexer, SYMBOL, ")");
      continue;
    case 3:
      node.as.postfix.expr = box(node);
      if (parse_rval(lexer, &ast_buf)) {
        if (consume_str_opt(lexer, SYMBOL, "]") == 0) {
          node.type = NODE_INDEXING;
          node.as.postfix.indexing = box(ast_buf.rval);
          continue;
        }
        node.as.postfix.range.from = box(ast_buf.rval);
      } else {
        node.as.postfix.range.from = NULL;
      }
      node.type = NODE_RANGE;
      node.as.postfix.range.inclusive = consume_str(lexer, SYMBOL, "..", "..=");
      if (parse_rval(lexer, &ast_buf)) {
        node.as.postfix.range.to = box(ast_buf.rval);
      } else {
        node.as.postfix.range.to = NULL;
      }
      consume_str(lexer, SYMBOL, "]");
      continue;
    default:
    }
    break;
  }
  // parsed postfix
  NodeType binop_t = NODE_INVALID;
  for (int i =
           consume_str_opt(lexer, SYMBOL, "*", "/", "%", "&&", "+", "-", "||",
                           "~", "==", "!=", "<=", ">=", "<", ">", "&", "|");
       i != -1;) {
    switch (i) {
    case 0:
      binop_t = NODE_TIMES;
      break;
    case 1:
      binop_t = NODE_DIVIDE;
      break;
    case 2:
      binop_t = NODE_MOD;
      break;
    case 3:
      binop_t = NODE_LOGICAL_AND;
      break;
    case 4:
      binop_t = NODE_PLUS;
      break;
    case 5:
      binop_t = NODE_MINUS;
      break;
    case 6:
      binop_t = NODE_LOGICAL_OR;
      break;
    case 7:
      binop_t = NODE_BIT_NOT;
      break;
    case 8:
      binop_t = NODE_EQUAL;
      break;
    case 9:
      binop_t = NODE_NOT_EQUAL;
      break;
    case 10:
      binop_t = NODE_LE;
      break;
    case 11:
      binop_t = NODE_GE;
      break;
    case 12:
      binop_t = NODE_LT;
      break;
    case 13:
      binop_t = NODE_GT;
      break;
    case 14:
      binop_t = NODE_BIT_AND;
      break;
    case 15:
      binop_t = NODE_BIT_OR;
      break;
    default:
      UNREACHABLE(temp_sprintf("%s", node_type_to_string(node.type)));
    }
    if (!parse_expr(lexer, &ast_buf))
      goto throw;
    for (;;) {
      int left_prec = expr_precedence(&(Expr){.type = binop_t});
      int right_prec = expr_precedence(&ast_buf.expr);
      node.as.binop.left = box(node);
      node.type = binop_t;
      if (right_prec <= 2 ||
          right_prec < left_prec) { // infix left is '<', infix right is '<='
        node.as.binop.right = box(ast_buf.expr);
        break;
      } // infix left performs some unnecessary movements, but it's alright
      node.as.binop.right = ast_buf.expr.as.binop.left;
      binop_t = ast_buf.expr.type;
      Expr *tmp = ast_buf.expr.as.binop.right;
      ast_buf.expr = *tmp;
      free(tmp);
    }
    break;
  }

ret:;
  *(Expr *)result = compose_node(node, lexer);
  nob_log(INFO, "ok expr");
  print_node(node);
  return true;
  // TODO: postfix, and all the binary operations
throw:;
  nob_log(WARNING, "err expr");
  padded_puts("", -2);
  free_node(node);
  dbg_lvl -= 1;
  return false;
}

bool parse_type_no_ident(Lexer *lexer, void *result) {
  debug_this();
  AstNode ast_buf = {0};
  TypeNoIdent node = new_node(TypeNoIdent, lexer);
  static bool enum_warned = false;
  static bool struct_warned = false;
  if (lexer->current_token.type == TOKEN_SYMBOL) {
    consume_str(lexer, SYMBOL, "<");
    if (lexer->current_token.type != TOKEN_IDENTIFIER)
      goto throw;
    da_init(&node.as.unknown, 3);
    da_append(&node.as.unknown, lexer->current_token);
    lexer_next_token(lexer);
    for (;;) {
      if (lexer->current_token.type != TOKEN_IDENTIFIER)
        break;
      da_init(&node.as.unknown, 3);
      da_append(&node.as.unknown, lexer->current_token);
      lexer_next_token(lexer);
    }
    consume_str(lexer, SYMBOL, ">");
  } else if (consume_str(lexer, IDENTIFIER, "enum", "struct") == 0) {
    if (parse_lval(lexer, &ast_buf))
      node.as.enum_.type = box(ast_buf.lval);
    consume_str(lexer, SYMBOL, "{");
    da_init(&node.as.enum_.items, 8);
    for (int delim = -1;;) {
      if (!parse_enum_item(lexer, &ast_buf))
        break;
      da_append(&node.as.enum_.items, ast_buf.enum_item);
      if (delim == -1)
        delim = consume_str_opt(lexer, SYMBOL, ",", ";");
      else if (delim != consume_str_opt(lexer, SYMBOL, ",", ";") &&
               !enum_warned) {
        enum_warned = true;
        nob_log(WARNING, "Mixing delimiters in enum is discouraged");
      }
    }
    consume_str(lexer, SYMBOL, "}");
    node.type = NODE_ENUM;
  } else {
    consume_str(lexer, SYMBOL, "{");
    da_init(&node.as.struct_, 8);
    for (int delim = -1;;) {
      if (!parse_param_item(lexer, &ast_buf))
        break;
      da_append(&node.as.struct_, ast_buf.param_item);
      if (delim == -1)
        delim = consume_str_opt(lexer, SYMBOL, ",", ";");
      else if (delim != consume_str_opt(lexer, SYMBOL, ",", ";") &&
               !struct_warned) {
        enum_warned = true;
        nob_log(WARNING, "Mixing delimiters in struct is discouraged");
      }
    }
    consume_str(lexer, SYMBOL, "}");
    node.type = NODE_STRUCT;
  }
  *(TypeNoIdent *)result = compose_node(node, lexer);
  nob_log(INFO, "ok typeni");
  print_node(node);
  return true;
throw:;
  nob_log(WARNING, "err typeni");
  free_node(node);
  padded_puts("", -2);
  dbg_lvl -= 1;
  return false;
}

bool parse_type(Lexer *lexer, void *result) {
  debug_this();
  AstNode ast_buf = {0};
  Type node = new_node(Type, lexer);
  if (parse_type_no_ident(lexer, &node.as.other))
    node.type = NODE_OTHER;
  else
    switch (consume_str_opt(lexer, SYMBOL, "(", "[]", "*")) {
    case 0:
      da_init(&node.as.tuple, 4);
      for (;;) {
        if (!parse_type(lexer, &ast_buf))
          break;
        da_append(&node.as.tuple, ast_buf.type);
        if (consume_str_opt(lexer, SYMBOL, ",") != 0)
          break;
      }
      consume_str(lexer, SYMBOL, ")");
      if (lexer->current_token.type != TOKEN_ARROW) {
        node.type = NODE_TUPLE;
        break;
      }
      lexer_next_token(lexer);
      node.as.function.params =
          *(typeof(node.as.function.params) *)&node.as.tuple;
      node.as.function.result = malloc(sizeof(Type));
      if (!parse_type(lexer, node.as.function.result))
        goto throw;
      node.type = NODE_FUNCTION;
      break;
    case 1:
      node.type = NODE_SLICE;
      node.as.slice = malloc(sizeof(Type));
      if (!parse_type(lexer, node.as.slice))
        goto throw;
      break;
    case 2:
      node.type = NODE_PTR;
      node.as.ptr = malloc(sizeof(Type));
      if (!parse_type(lexer, node.as.ptr))
        goto throw;
      break;
    default:
      node.type = NODE_ID;
      if (lexer->current_token.type != TOKEN_IDENTIFIER)
        goto throw;
      node.as.ident = lexer->current_token;
      lexer_next_token(lexer);
      break;
    }
  *(Type *)result = compose_node(node, lexer);
  nob_log(INFO, "ok type");
  print_node(node);
  return true;
throw:;
  nob_log(WARNING, "err type");
  free_node(node);
  padded_puts("", -2);
  dbg_lvl -= 1;
  return false;
}

bool parse_enum_item(Lexer *lexer, void *result) {
  debug_this();
  EnumItem node = new_node(EnumItem, lexer);
  node.ident = lexer->current_token;
  consume_tok(lexer, IDENTIFIER);
  if (consume_str_opt(lexer, SYMBOL, "=") == 0) {
    node.expr = malloc(sizeof(Expr));
    if (!parse_expr(lexer, node.expr))
      goto throw;
  }
  *(EnumItem *)result = compose_node(node, lexer);
  nob_log(INFO, "ok enumItem");
  print_node(node);
  return true;
throw:;
  nob_log(WARNING, "err enumItem");
  free_node(node);
  padded_puts("", -2);
  dbg_lvl -= 1;
  return false;
}

bool parse_param_item(Lexer *lexer, void *result) {
  debug_this();
  ParamItem node = new_node(ParamItem, lexer);
  da_init(&node.idents, 2);
  for (;;) {
    da_append(&node.idents, lexer->current_token);
    consume_tok(lexer, IDENTIFIER);
    switch (consume_str(lexer, SYMBOL, ",", ":")) {
    case 0:
      continue;
    case 1:
      goto exit_loop;
    default:
      __builtin_unreachable();
    }
  }
exit_loop:;
  if (!parse_type(lexer, &node.type))
    goto throw;
  *(ParamItem *)result = compose_node(node, lexer);
  nob_log(INFO, "ok paramItem");
  print_node(node);
  return true;
throw:;
  nob_log(WARNING, "err paramItem");
  padded_puts("", -2);
  free_node(node);
  dbg_lvl -= 1;
  return false;
}

bool parse_function(Lexer *lexer, void *result) {
  debug_this();
  AstNode ast_buf = {0};
  Function node = new_node(Function, lexer);
  consume_str(lexer, SYMBOL, "(");
  da_init(&node.params, 4);
  for (;;) {
    if (!parse_param_item(lexer, &ast_buf))
      break;
    da_append(&node.params, ast_buf.param_item);
    if (consume_str_opt(lexer, SYMBOL, ",") != 0)
      break;
  }
  consume_str(lexer, SYMBOL, ")");
  consume_tok(lexer, ARROW);
  if (parse_type(lexer, &ast_buf))
    node.result = box(ast_buf.type);
  consume_str(lexer, SYMBOL, "{");
  node.program = malloc(sizeof(Program));
  if (!parse_program(lexer, node.program))
    goto throw;
  consume_str(lexer, SYMBOL, "}");
  *(Function *)result = compose_node(node, lexer);
  nob_log(INFO, "ok function");
  print_node(node);
  return true;
throw:;
  nob_log(WARNING, "err function");
  free_node(node);
  padded_puts("", -2);
  dbg_lvl -= 1;
  return false;
}

bool parse_rval(Lexer *lexer, void *result) {
  debug_this();
  AstNode ast_buf = {0};
  Rval node = new_node(Rval, lexer);
  if (consume_str_opt(lexer, IDENTIFIER, "switch") == 0) {
    if (!parse_expr(lexer, &node.as.switch_.expr))
      goto throw;
    consume_str(lexer, SYMBOL, "{");
    Lexer copy = *lexer;
    da_init(&node.as.switch_.case_, 8);
    da_init(&node.as.switch_.rval, 8);
    for (;;) {
      if (!parse_switch_case(&copy, &ast_buf))
        break;
      da_append(&node.as.switch_.case_, ast_buf.switch_case);
      if (!parse_rval(&copy, &ast_buf)) {
        node.as.switch_.case_.count -= 1;
        break;
      }
      da_append(&node.as.switch_.rval, ast_buf.rval);
      *lexer = copy;
    }
    consume_str(lexer, SYMBOL, "}");
    node.type = NODE_SWITCH;
  } else {
    switch (parse_(lexer, ast_buf, type_no_ident, expr, function, lval)) {
    case 1: // expr
      node.as.expr = ast_buf.expr;
      node.type = NODE_EXPR;
      break;
    case 2: // function
      node.as.function = ast_buf.function;
      node.type = NODE_FUNCTION;
      break;
    case 0: // type_no_ident
      node.as.type = ast_buf.type_no_ident;
      node.type = NODE_TYPE;
      break;
    case 3: // lval
      node.as.struct_literal.type = box(ast_buf.lval);
      node.type = NODE_STRUCT_LITERAL;
      // falls through
    default:
      consume_str(lexer, SYMBOL, ".{");
      node.type = NODE_STRUCT_LITERAL;
      da_init(&node.as.struct_literal.items, 8);
      for (;;) {
        ast_buf.enum_item = (EnumItem){0};
        if (!parse_enum_item(lexer, &ast_buf))
          break;
        da_append(&node.as.struct_literal.items, ast_buf.enum_item);
        consume_str_opt(lexer, SYMBOL, ",");
      }
      consume_str(lexer, SYMBOL, "}");
      break;
    }
  }
  *(Rval *)result = compose_node(node, lexer);
  nob_log(INFO, "ok rval");
  print_node(node);
  return true;
throw:;
  nob_log(WARNING, "err rval");
  free_node(node);
  padded_puts("", -2);
  return false;
}

bool parse_assign_eq(Lexer *lexer, void *result) {
  debug_this();
  AssignEq node = new_node(AssignEq, lexer);
  if (!parse_lval(lexer, &node.lval))
    goto throw;
  node.kind = lexer->current_token;
  consume_str(lexer, SYMBOL, "=", ":", "+=", "-=", "*=", "/=", "%=");
  if (!parse_rval(lexer, &node.rval) || node.rval.type == NODE_TYPE)
    goto throw;
  *(AssignEq *)result = compose_node(node, lexer);
  nob_log(INFO, "ok assignEq");
  print_node(node);
  return true;
throw:;
  nob_log(WARNING, "err assignEq");
  free_node(node);
  padded_puts("", -2);
  dbg_lvl -= 1;
  return false;
}

bool parse_assign(Lexer *lexer, void *result) {
  debug_this();
  Assign node = new_node(Assign, lexer);
  Lexer copy = *lexer;
  if (!parse_lval(lexer, &node.lval))
    goto throw;
  Token assign_eq_kind = lexer->current_token;
  switch (consume_str(lexer, SYMBOL, "=", ":", "+=", "-=", "*=", "/=", "%=")) {
  case 0: // =
    if (!parse_rval(lexer, &node.rval) || node.rval.type == NODE_TYPE)
      goto throw;
    node.type = NODE_VAL_ASSIGN;
    break;
  case 1: // :
    bool typed = parse_type(lexer, &node.typ);
    int res = consume_str_opt(lexer, SYMBOL, "=", ":");
    // nob_log(INFO, "parse_assign: %d, %d", typed, res);
    if (res != -1) {
      if (!parse_rval(lexer, &node.rval) ||
          (res == 0 && node.rval.type == NODE_TYPE))
        goto throw;
      if (res == 0)
        node.type = typed ? NODE_FULL_ASSIGN : NODE_IMPLIED_ASSIGN;
      else
        node.type = typed ? NODE_FULL_CONST_ASSIGN : NODE_IMPLIED_CONST_ASSIGN;
    } else
      node.type = NODE_TYPE_ASSIGN;
    break;
  case 2: // +=
  case 3: // -=
  case 4: // *=
  case 5: // /=
  case 6: // %=
    if (!parse_rval(lexer, &node.rval) || node.rval.type == NODE_TYPE)
      goto throw;
    node.type = NODE_ASSIGN_EQ;
    node.assign_eq_kind = assign_eq_kind;
    break;
  default:
    __builtin_unreachable();
  }
  *(Assign *)result = compose_node(node, lexer);
  nob_log(INFO, "ok assign");
  print_node(node);
  return true;
throw:;
  *lexer = copy;
  nob_log(WARNING, "err assign");
  free_node(node);
  padded_puts("", -2);
  dbg_lvl -= 1;
  return false;
}

bool parse_for_body(Lexer *lexer, void *result) {
  debug_this();
  AstNode ast_buf = {0};
  ForBody node = new_node(ForBody, lexer);
  Lexer copy = *lexer;
  if (parse_assign(lexer, &ast_buf)) {
    switch (ast_buf.assign.type) {
    default:
      if (consume_str_opt(lexer, SYMBOL, ";") == 0) {
        node.start = box(ast_buf.assign);
        break;
      } // fall through
    case NODE_FULL_CONST_ASSIGN:
    case NODE_IMPLIED_CONST_ASSIGN:
      *lexer = copy;
      break;
    }
  }
  if (!parse_expr(lexer, &node.expr))
    goto throw;
  copy = *lexer;
  if (consume_str_opt(lexer, SYMBOL, ";") == 0) {
    switch (parse_(lexer, ast_buf, assign_eq, expr)) {
    case 0:
      node.end_type = NODE_ASSIGN_EQ;
      node.end.assign_eq = box(ast_buf.assign_eq);
      break;
    case 1:
      node.end_type = NODE_EXPR;
      node.end.expr = box(ast_buf.expr);
      break;
    default:
      *lexer = copy;
      break;
    }
  }
  *(ForBody *)result = compose_node(node, lexer);
  nob_log(INFO, "ok forBody");
  print_node(node);
  return true;
throw:;
  nob_log(WARNING, "err forBody");
  free_node(node);
  padded_puts("", -2);
  dbg_lvl -= 1;
  return false;
}

bool parse_if_stmt(Lexer *lexer, void *result) {
  debug_this();
  AstNode ast_buf = {0}, ast_buf2 = {0};
  IfStmt node = new_node(IfStmt, lexer);
  node.type = NODE_ELSE;
  for (;;) {
    ast_buf.if_ = new_node(IfStmt, lexer);
    consume_str(lexer, IDENTIFIER, "if");
    if (!parse_expr(lexer, &ast_buf.if_.as.if_.expr))
      goto throw;
    consume_str(lexer, SYMBOL, "{");
    if (!parse_program(lexer, &ast_buf2))
      goto throw;
    consume_str(lexer, SYMBOL, "}");
    ast_buf.if_.as.if_.program = box(ast_buf2.program);
    ast_buf.if_.type = NODE_IF;
    if (consume_str_opt(lexer, IDENTIFIER, "else") == 0) {
      if (consume_str_opt(lexer, SYMBOL, "{") == -1) {
        // parse else chain
        if (node.as.else_.capacity == 0)
          da_init(&node.as.else_, 4);
        da_append(&node.as.else_, compose_node(ast_buf.if_, lexer));
        continue;
      }
      // parse tail
      if (node.as.else_.capacity == 0) {
        node.as.tail.start = box(ast_buf.if_);
      } else {
        da_append(&node.as.else_, compose_node(ast_buf.if_, lexer));
        node.as.tail.start = box(node);
      }
      node.type = NODE_TAIL;
      if (!parse_program(lexer, &ast_buf))
        goto throw;
      consume_str(lexer, SYMBOL, "}");
      node.as.tail.end = box(ast_buf.program);
    } else if (node.as.else_.capacity == 0) {
      node = ast_buf.if_;
    } else {
      da_append(&node.as.else_, compose_node(ast_buf.if_, lexer));
    }
    break;
  }
  *(IfStmt *)result = compose_node(node, lexer);
  nob_log(INFO, "ok ifStmt");
  print_node(node);
  return true;
throw:;
  nob_log(WARNING, "err ifStmt");
  free_node(node);
  padded_puts("", -2);
  dbg_lvl -= 1;
  return false;
}

bool parse_case_item(Lexer *lexer, void *result) {
  debug_this();
  AstNode ast_buf = {0};
  CaseItem node = new_node(CaseItem, lexer);

  if (!parse_rval(lexer, &node.rval))
    goto throw;

  if (consume_str_opt(lexer, IDENTIFIER, "if") == 0 &&
      parse_expr(lexer, &ast_buf))
    node.if_expr = box(ast_buf.expr);

  *(CaseItem *)result = compose_node(node, lexer);
  nob_log(INFO, "ok caseItem");
  print_node(node);
  return true;
throw:;
  nob_log(WARNING, "err caseItem");
  free_node(node);
  padded_puts("", -2);
  dbg_lvl -= 1;
  return false;
}

bool parse_switch_case(Lexer *lexer, void *result) {
  debug_this();
  AstNode ast_buf = {0};
  SwitchCase node = new_node(SwitchCase, lexer);

  switch (consume_str_opt(lexer, IDENTIFIER, "default", "case")) {
  case -1:
    da_append(&parse_errors,
              temp_sprintf("expected default or case, but got " SV_Fmt,
                           SV_Arg(lexer->current_token.lexeme)));
    goto throw;
  case 0:
    node.default_ = true;
    break;
  case 1:
    da_init(&node.items, 8);
    for (;;) {
      if (!parse_case_item(lexer, &ast_buf))
        goto throw;
      da_append(&node.items, ast_buf.case_item);
      consume_str_or(goto case_items_exit, lexer, SYMBOL, ",");
    }
  case_items_exit:;
    break;
  default:
    __builtin_unreachable();
  }
  consume_str(lexer, SYMBOL, ":");

  *(SwitchCase *)result = compose_node(node, lexer);
  nob_log(INFO, "ok switchCase");
  print_node(node);
  return true;
throw:;
  nob_log(WARNING, "err switchCase");
  free_node(node);
  padded_puts("", -2);
  dbg_lvl -= 1;
  return false;
}

bool parse_stmt(Lexer *lexer, void *result) {
  debug_this();
  AstNode ast_buf = {0};
  Stmt stmt = new_node(Stmt, lexer);
  Lexer copy = *lexer;

  while (lexer->current_token.type == TOKEN_COMMENT)
    lexer_next_token(lexer);
  Token tok = lexer->current_token;
  switch (consume_str_opt(lexer, IDENTIFIER, "switch", "break", "continue",
                          "return", "for", "while")) {
  case 0: { // 'switch' expr [ '{' (switch_case program)* '}' ]: switch
    if (!parse_expr(lexer, &stmt.as.switch_.expr))
      goto throw;
    consume_str(lexer, SYMBOL, "{");
    Lexer copy = *lexer;
    da_init(&stmt.as.switch_.case_, 8);
    da_init(&stmt.as.switch_.body, 8);
    for (;;) {
      printf("%s: " SV_Fmt "\n",
             token_type_to_string(lexer->current_token.type),
             SV_Arg(lexer->current_token.lexeme));
      if (!parse_switch_case(&copy, &ast_buf))
        break;
      da_append(&stmt.as.switch_.case_, ast_buf.switch_case);
      if (!parse_program(&copy, &ast_buf)) {
        stmt.as.switch_.case_.count -= 1;
        break;
      }
      da_append(&stmt.as.switch_.body, ast_buf.program);
      *lexer = copy;
    }
    consume_str(lexer, SYMBOL, "}");
    stmt.type = NODE_SWITCH;
    break;
  }
  case 1:
  case 2:
  case 3: // ('break' | 'continue' | 'return') (rval | ';'): flow_stmt
    stmt.as.flow_stmt.which = tok;
    if (consume_str_opt(lexer, SYMBOL, ";") != 0) {
      if (!parse_rval(lexer, &ast_buf))
        goto throw;
      stmt.as.flow_stmt.rval = malloc(sizeof(Rval));
      *stmt.as.flow_stmt.rval = ast_buf.rval;
    }
    stmt.type = NODE_FLOW_STMT;
    break;
  case 4: // 'for' [ '{' program '}' ]: loop
    consume_str_or(goto forloop, lexer, SYMBOL, "{");
    if (!parse_program(lexer, &ast_buf))
      goto throw;
    consume_str(lexer, SYMBOL, "}");
    stmt.as.loop = box(ast_buf.program);
    stmt.type = NODE_LOOP;
    break;
  forloop:;
  case 5: // prefix // ('for' | 'while') for_body: for
    if (!parse_for_body(lexer, &stmt.as.for_.head))
      goto throw;
    if (!parse_stmt(lexer, &ast_buf))
      goto throw;
    stmt.as.for_.stmt = box(ast_buf.stmt);
    stmt.type = NODE_FOR;
    break;
  case -1:
    // [ '{' program '}' ]: block
    if (consume_str_opt(lexer, SYMBOL, "{") == 0) {
      if (!parse_program(lexer, &ast_buf))
        goto throw;
      consume_str(lexer, SYMBOL, "}");
      stmt.as.block = box(ast_buf.program);
      stmt.type = NODE_BLOCK;
      break;
    }
    switch (parse_or_throw(lexer, ast_buf, assign, if_stmt, lval)) {
    case 0: // assign: assign
      stmt.as.assign = ast_buf.assign;
      stmt.type = NODE_ASSIGN;
      break;
    case 1: // if_stmt: if
      stmt.as.if_ = ast_buf.if_;
      stmt.type = NODE_IF;
      break;
    case 2: // lval [ '(' (rval (',' rval)*)? ','? ')' ]: call
      stmt.as.call.lval = ast_buf.lval;
      consume_str(lexer, SYMBOL, "(");
      da_init(&stmt.as.call.params, 8);
      for (;;) {
        if (!parse_rval(lexer, &ast_buf))
          break;
        da_append(&stmt.as.call.params, ast_buf.rval);
        if (consume_str_opt(lexer, SYMBOL, ",") != 0)
          break;
      }
      consume_str(lexer, SYMBOL, ")");
      stmt.type = NODE_CALL;
      break;
    default:
      __builtin_unreachable();
    }
    break;
  default:
    __builtin_unreachable();
  }

  *(Stmt *)result = compose_node(stmt, lexer);
  nob_log(INFO, "ok stmt");
  print_node(stmt);
  return true;
throw:;
  *lexer = copy;
  nob_log(WARNING, "err stmt");
  free_node(stmt);
  padded_puts("", -2);
  dbg_lvl -= 1;
  return false;
}

bool parse_program(Lexer *lexer, void *result) {
  debug_this();
  AstNode ast_buf = {0};
  Program program = new_node(Program, lexer);

  choice_depth += 1;
  da_init(&program.stmts, 8);
  while (lexer->current_token.type != TOKEN_EOF) {
    if (!parse_stmt(lexer, &ast_buf))
      break;
    consume_str_opt(lexer, SYMBOL, ";");
    da_append(&program.stmts, ast_buf.stmt);
  };
  choice_depth -= 1;

  *(Program *)result = compose_node(program, lexer);
  nob_log(INFO, "ok program");
  print_node(program);
  return true;
}

AstNode parse_source_file(Lexer lexer) {
  AstNode node = {0};
  lexer_next_token(&lexer);

  parse_program(&lexer, &node);
  return node;
}

// at this point i want to establish transformations that later lead to
// codegen

#define SCOPE_OBJECT_KINDS                                                     \
  X(UNKNOWN)                                                                   \
  X(VALUE)                                                                     \
  X(CONST_VALUE)

sized_enum(char, ScopeObjectKind){
#define X(v) SOK_##v,
    SCOPE_OBJECT_KINDS
#undef X
};

const char *sok_to_string(ScopeObjectKind sok) {
  switch (sok) {
#define X(v)                                                                   \
  case SOK_##v:                                                                \
    return #v;
    SCOPE_OBJECT_KINDS
#undef X
  default:
    __builtin_unreachable();
  }
}

typedef String_View HM_Key;

typedef struct ScopeObject {
  ScopeObjectKind kind;
  union {
    struct {
      AstNode *type, *value;
    } value;
  } as;
} ScopeObject;

// #define predefine_type(_name) \
//   &(AstNode) { \
//     .type = NODE_IDENTIFIER, .source = {sizeof(#_name) - 1, #_name}, \
//     .node = \
//         &(Identifier){.str = {sizeof(#_name) - 1, #_name}, .builtin = true},
//         \
//   }

// AstNode *_type_type = predefine_type(type);
// AstNode *_type_unknown = predefine_type(unknown);
// AstNode *_type_string = predefine_type(string);
// AstNode *_type_int = predefine_type(int);
// AstNode *_type_intl = predefine_type(intl);
// AstNode *_type_intll = predefine_type(intll);
// AstNode *_type_intu = predefine_type(intu);
// AstNode *_type_intul = predefine_type(intul);
// AstNode *_type_intull = predefine_type(intull);
// AstNode *_type_f32 = predefine_type(f32);
// AstNode *_type_f64 = predefine_type(f64);
// AstNode *_type_f128 = predefine_type(f128);
// AstNode *_type_bool = predefine_type(bool);
// AstNode *_type_i8 = predefine_type(i8);
// AstNode *_type_i16 = predefine_type(i16);
// AstNode *_type_i32 = predefine_type(i32);
// AstNode *_type_i64 = predefine_type(i64);
// AstNode *_type_i128 = predefine_type(i128);
// AstNode *_type_u8 = predefine_type(u8);
// AstNode *_type_u16 = predefine_type(u16);
// AstNode *_type_u32 = predefine_type(u32);
// AstNode *_type_u64 = predefine_type(u64);
// AstNode *_type_u128 = predefine_type(u128);

// #undef predefine_type

// void on_node_parsed(AstCallbackContext context, AstNode *ast);

// void on_value_parsed(AstCallbackContext context, AstNode *value_ast) {
//         UNUSED(context);
//         call_ast_callbacks(value_ast);
// }

// void on_multiword_type_parsed(AstCallbackContext context, AstNode *type_ast)
// {
//         UNUSED(context);
//         call_ast_callbacks(type_ast);
// }

// void on_binary_expression_parsed(AstCallbackContext context,
//                                  AstNode *bin_expression_ast) {
//         UNUSED(context);
//         BinaryExpression *bin_expression = bin_expression_ast->node;

//         on_node_parsed(context, bin_expression->left);
//         on_node_parsed(context, bin_expression->right);
//         call_ast_callbacks(bin_expression_ast);
// }

// void on_identifier_parsed(AstCallbackContext context, AstNode
// *identifier_ast) {
//         UNUSED(context);
//         call_ast_callbacks(identifier_ast);
// }

// void on_type_parsed(AstCallbackContext context, AstNode *type_ast) {
//         UNUSED(context);
//         Type *type = type_ast->node;

//         on_node_parsed(context, type->item);
//         call_ast_callbacks(type_ast);

//         switch (type->item->type) {
//         case NODE_IDENTIFIER: {
//           Identifier *identifier = cast(Identifier *, type->item->node);
//           const char *typename = identifier->str.data;
// #define check_type_to(name_, res_) \
//   if (is_str_here(typename, #name_) && type->item->node != _type_##res_) \
//   type->item = _type_##res_
// #define check_type(name_) check_type_to(name_, name_)
//           AstNode *saved = type->item;
//           check_type(type);
//           else check_type(unknown);
//           else check_type(i8);
//           else check_type(u8);
//           else check_type(i16);
//           else check_type(u16);
//           else check_type(i32);
//           else check_type(u32);
//           else check_type(i64);
//           else check_type(u64);
//           else check_type(i128);
//           else check_type(u128);
//           else check_type(f128);
//           else check_type_to(float, f32);
//           else check_type_to(double, f64);
//           else check_type(bool);
//           else check_type_to(_Bool, bool);
//           else check_type(int);
//           else check_type(intu);
//           else check_type(intl);
//           else check_type(intul);
//           else check_type(intll);
//           else check_type(intull);
//           else check_type(string);
//           else check_type_to(char, u8);
//           else check_type_to(int8_t, i8);
//           else check_type_to(int16_t, i16);
//           else check_type_to(int32_t, i32);
//           else check_type_to(int64_t, i64);
//           else check_type_to(int128_t, i128);
//           else check_type_to(uint8_t, u8);
//           else check_type_to(uint16_t, u16);
//           else check_type_to(uint32_t, u32);
//           else check_type_to(uint64_t, u64);
//           else check_type_to(uint128_t, u128);
//           else break;
//           free_node(saved);
//           break;
//         }
// #undef check_type
//         case NODE_MULTIWORD_TYPE: {
//           static bool warned = false;
//           if (!warned) {
//             nob_log(WARNING,
//                     "TODO: multiword types may represent known types such as
//                     " "long int or " "unsigned long long int, this needs
//                     separate parsing");
//             warned = true;
//           }
//           break;
//         }
//         default:
//           TODO(temp_sprintf("type %s",
//           node_type_to_string(type->item->type)));
//         }
//         // nob_log(INFO, "Callback <%s>: %s",
//         // node_type_to_string(type_ast->type),
//         //         node_type_to_string(type->item->type));
// }

// void on_expression_parsed(AstCallbackContext context, AstNode
// *expression_ast) {
//         UNUSED(context);
//         Expression *expression = expression_ast->node;
//         on_node_parsed(context, expression->item);
//         call_ast_callbacks(expression_ast);

//         // nob_log(INFO, "Callback <%s>: %s",
//         // node_type_to_string(expression_ast->type),
//         //         node_type_to_string(expression->item->type));
// }

// void on_value_assignment_parsed(AstCallbackContext context,
//                                 AstNode *assignment_ast) {
//         UNUSED(context);
//         ValueAssignment *assignment = assignment_ast->node;

//         on_node_parsed(context, assignment->left);
//         on_node_parsed(context, assignment->right);
//         call_ast_callbacks(assignment_ast);

//         // nob_log(INFO, "Callback <%s>: %s <> %s",
//         // node_type_to_string(assignment_ast->type),
//         //         node_type_to_string(assignment->left->type),
//         //         node_type_to_string(assignment->right->type));
// }

// void on_type_or_const_assignment_parsed(AstCallbackContext context,
//                                         AstNode *assignment_ast) {
//         UNUSED(context);
//         TypeOrConstAssignment *assignment = assignment_ast->node;

//         on_node_parsed(context, assignment->left);
//         on_node_parsed(context, assignment->right);
//         call_ast_callbacks(assignment_ast);

//         // nob_log(INFO, "Callback <%s>: %s <> %s",
//         // node_type_to_string(assignment_ast->type),
//         //         node_type_to_string(assignment->left->type),
//         //         node_type_to_string(assignment->right->type));
// }

// AstNode *imply_type(HM *scope, AstNode *ast) {
//         switch (ast->type) {
//         case NODE_EXPRESSION:
//           return imply_type(scope, cast(Expression *, ast->node)->item);
//         case NODE_BINARY_EXPRESSION: {
//           AstNode *child =
//               imply_type(scope, cast(BinaryExpression *, ast->node)->left);
//           if (child == NULL)
//             child =
//                 imply_type(scope, cast(BinaryExpression *,
//                 ast->node)->right);
//           return child == NULL ? _type_unknown : child;
//         }
//         case NODE_VALUE: {
//           Value *val = ast->node;
//           switch (val->kind) {
//           case VALUE_IDENTIFIER: {
//             ScopeObject *obj = HM_kwl_get(scope, val->as.identifier.data,
//                                           val->as.identifier.count);
//             if (obj == NULL)
//               return _type_unknown;
//             if (obj->as.value.type == _type_type)
//               return obj->as.value.value;
//             return obj->as.value.type;
//           }
//           case VALUE_STRING:
//             return _type_string;
//           case VALUE_INT:
//             return _type_int;
//           case VALUE_INTU:
//             return _type_intu;
//           case VALUE_INTL:
//             return _type_intl;
//           case VALUE_INTUL:
//             return _type_intul;
//           case VALUE_INTLL:
//             return _type_intll;
//           case VALUE_INTULL:
//             return _type_intull;
//           case VALUE_FLOAT32:
//             return _type_f32;
//           case VALUE_FLOAT64:
//             return _type_f64;
//           case VALUE_FLOAT128:
//             return _type_f128;
//           case VALUE_TRUE:
//             return _type_bool;
//           case VALUE_FALSE:
//             return _type_bool;
//           case VALUE_CHAR:
//             return _type_u8;
//           }
//           // TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_INT, TOKEN_INTU,
//           TOKEN_INTL,
//           //     TOKEN_INTUL, TOKEN_INTLL, TOKEN_INTULL, TOKEN_FLOAT32,
//           //     TOKEN_FLOAT64, TOKEN_FLOAT128, TOKEN_TRUE, TOKEN_FALSE,
//           //     TOKEN_CHAR
//         }
//         default:
//           TODO(temp_sprintf("Trying to imply type of %s",
//                             node_type_to_string(ast->type)));
//         }
// }

// void on_implied_assignment_parsed(AstCallbackContext context,
//                                   AstNode *assignment_ast) {
//         UNUSED(context);
//         ImpliedAssignment *assignment = assignment_ast->node;

//         on_node_parsed(context, assignment->left);
//         on_node_parsed(context, assignment->right);
//         call_ast_callbacks(assignment_ast);

//         Identifier *target = assignment->left->node;
//         assert(assignment->left->type == NODE_IDENTIFIER);

//         AstNode *value_ast = assignment->right;
//         ScopeObject *val =
//             HM_kwl_get(context.scope, target->str.data, target->str.count);
//         if (assignment->is_const) {
//           if (val != NULL) {
//             nob_log(
//                 ERROR,
//                 "%s: Can't redefine variable:\n"
//                 "%d: %.*s\n"
//                 "- Previously defined here:\n"
//                 "%d: %.*s",
//                 context.file_name, assignment_ast->line,
//                 (int)(assignment_ast->source.count + assignment_ast->col -
//                 2), assignment_ast->source.data - assignment_ast->col + 2,
//                 val->as.value.value->line,
//                 (int)(val->as.value.value->source.count +
//                       val->as.value.value->col - 2),
//                 val->as.value.value->source.data - val->as.value.value->col +
//                     2);
//             __builtin_trap();
//           }
//           switch (value_ast->type) {
//           case NODE_TYPE: {
//             Type *type = value_ast->node;
//             HM_kwl_set(context.scope, target->str.data, target->str.count,
//                        &(ScopeObject){.kind = SOK_CONST_VALUE,
//                                       .as = {.value = {.type = _type_type,
//                                                        .value =
//                                                        type->item}}});
//             // context.scope
//             break;
//           }
//           case NODE_EXPRESSION: {
//             Expression *expr = value_ast->node;
//             HM_kwl_set(
//                 context.scope, target->str.data, target->str.count,
//                 &(ScopeObject){.kind = SOK_CONST_VALUE,
//                                .as = {.value = {.type = imply_type(
//                                                     context.scope,
//                                                     expr->item),
//                                                 .value = expr->item}}});
//             break;
//           }
//           default:
//             UNREACHABLE(temp_sprintf("Unhandled assignment value type: %s",
//                                      node_type_to_string(value_ast->type)));
//           }
//         } else {
//           // := assignment
//           if (val != NULL) {
//             nob_log(
//                 ERROR,
//                 "%s: Can't redefine variable:\n"
//                 "%d: %.*s\n"
//                 "- Previously defined here:\n"
//                 "%d: %.*s",
//                 context.file_name, assignment_ast->line,
//                 (int)(assignment_ast->source.count + assignment_ast->col -
//                 2), assignment_ast->source.data - assignment_ast->col + 2,
//                 val->as.value.value->line,
//                 (int)(val->as.value.value->source.count +
//                       val->as.value.value->col - 2),
//                 val->as.value.value->source.data - val->as.value.value->col +
//                     2);
//           }
//           AstNode *type = imply_type(context.scope, value_ast);
//           // printf("implied type: %p\nast_node_name: %s\nstr: %.*s\n", type,
//           //        node_type_to_string(type->type),
//           //        (int)cast(Identifier *, type->node)->str.count,
//           //        cast(Identifier *, type->node)->str.data);
//           HM_kwl_set(context.scope, target->str.data, target->str.count,
//                      &(ScopeObject){.kind = SOK_VALUE,
//                                     .as = {.value = {
//                                                .type = type,
//                                                .value = value_ast,
//                                            }}});
//         }

//         // nob_log(INFO, "Callback <%s>: %s <> %s",
//         // node_type_to_string(assignment_ast->type),
//         //         node_type_to_string(assignment->left->type),
//         //         node_type_to_string(assignment->right->type));
// }

// void on_full_assignment_parsed(AstCallbackContext context,
//                                AstNode *assignment_ast) {
//         FullAssignment *assignment = assignment_ast->node;

//         on_node_parsed(context, assignment->target);
//         on_node_parsed(context, assignment->type);
//         on_node_parsed(context, assignment->value);
//         call_ast_callbacks(assignment_ast);

//         Identifier *target = assignment->target->node;
//         assert(assignment->target->type == NODE_IDENTIFIER);
//         Type *type = assignment->type->node;
//         AstNode *value_ast = assignment->value;

//         ScopeObject *val =
//             HM_kwl_get(context.scope, target->str.data, target->str.count);
//         if (val == NULL) {
//           HM_kwl_set(context.scope, target->str.data, target->str.count,
//                      &(ScopeObject){.kind = assignment_ast->type ==
//                                                     NODE_FULL_CONST_ASSIGNMENT
//                                                 ? SOK_CONST_VALUE
//                                                 : SOK_VALUE,
//                                     .as = {.value = {
//                                                .type = type->item,
//                                                .value = value_ast,
//                                            }}});
//         } else {
//           AstNode *prev_def;
//           switch (val->kind) {
//           case SOK_CONST_VALUE:
//             prev_def = val->as.value.value;
//             break;
//           case SOK_VALUE:
//             prev_def = val->as.value.type;
//             break;
//           default:
//             __builtin_unreachable();
//           }
//           nob_log(ERROR,
//                   "%s: Can't redefine variable:\n"
//                   "%d: %.*s\n"
//                   "- Previously defined here:\n"
//                   "%d: %.*s",
//                   context.file_name, assignment_ast->line,
//                   (int)(assignment_ast->source.count + assignment_ast->col -
//                   2), assignment_ast->source.data - assignment_ast->col + 2,
//                   prev_def->line,
//                   (int)(prev_def->source.count + prev_def->col - 2),
//                   prev_def->source.data - prev_def->col + 2);
//           __builtin_trap();
//           // TODOOOOOO: figure out why col is wrong... or deprecate it all
//           // together since my tokenizer is strange :(
//         }

//         // nob_log(INFO, "Callback <%s>: %s <> %s <> %s",
//         // node_type_to_string(assignment_ast->type),
//         //         node_type_to_string(assignment->target->type),
//         //         node_type_to_string(assignment->type->type),
//         //         node_type_to_string(assignment->value->type));
// }

// void on_assign_parsed(AstCallbackContext context, AstNode *ast) {
//   UNUSED(context);
//   Assign *assignment = (Assign *)ast;

//   on_node_parsed(context, assignment->item);
//   call_ast_callbacks(assignment_ast);

//   // nob_log(INFO, "Callback <assignment>: %s",
//   //         node_type_to_string(assignment->item->type));
// }

// void on_comment_parsed(AstCallbackContext context, AstNode *comment_ast) {
//         UNUSED(context);
//         call_ast_callbacks(comment_ast);

//         // nob_log(INFO, "Callback <comment>: %.*s",
//         //         (int)cast(Comment *, comment_ast->node)->str.count,
//         //         cast(Comment *, comment_ast->node)->str.data);
// }

void on_program_parsed(AstCallbackContext context, AstNode *ast);
void on_statement_parsed(AstCallbackContext context, AstNode *ast);
void on_program_parsed(AstCallbackContext context, AstNode *ast);
void on_expr_parsed(AstCallbackContext context, AstNode *ast);
void on_rval_parsed(AstCallbackContext context, AstNode *ast);
void on_lval_parsed(AstCallbackContext context, AstNode *ast);
void on_assign_parsed(AstCallbackContext context, AstNode *ast);
void on_if_stmt_parsed(AstCallbackContext context, AstNode *ast);
void on_switch_case_parsed(AstCallbackContext context, AstNode *ast);
void on_type_parsed(AstCallbackContext context, AstNode *ast);
void on_type_no_ident_parsed(AstCallbackContext context, AstNode *ast);
void on_param_item_parsed(AstCallbackContext context, AstNode *ast);
void on_enum_item_parsed(AstCallbackContext context, AstNode *ast);

#define on_node_parsed(ctx, ast)                                               \
  _Generic((ast),                                                              \
      Program *: on_program_parsed,                                            \
      Stmt *: on_statement_parsed,                                             \
      Expr *: on_expr_parsed,                                                  \
      Rval *: on_rval_parsed,                                                  \
      Lval *: on_lval_parsed,                                                  \
      Assign *: on_assign_parsed,                                              \
      IfStmt *: on_if_stmt_parsed,                                             \
      SwitchCase *: on_switch_case_parsed,                                     \
      Type *: on_type_parsed,                                                  \
      TypeNoIdent *: on_type_no_ident_parsed,                                  \
      ParamItem *: on_param_item_parsed,                                       \
      EnumItem *: on_enum_item_parsed)(ctx, (AstNode *)(ast))

/* Lval */
void on_lval_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  Lval *node = (Lval *)ast;

  switch (node->type) {
  case NODE_ID:
  case NODE_FIELD:
    break;
  default:
    UNREACHABLE(
        temp_sprintf("unexpected node %s", node_type_to_string(node->type)));
  }

  call_ast_callbacks(Lval, ast);
}

/* Expr */
void on_expr_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  Expr *node = (Expr *)ast;

  switch (node->type) {
  case NODE_ENUM_VAL:
  case NODE_LITERAL:
  case NODE_STRING:
    break;
  case NODE_VARIABLE:
    on_lval_parsed(context, (AstNode *)&node->as.variable);
    break;
  case NODE_NEGATIVE:
  case NODE_LOGICAL_NOT:
    if (node->as.prefix)
      on_expr_parsed(context, (AstNode *)node->as.prefix);
    break;
  case NODE_CALL:
    if (node->as.postfix.expr)
      on_expr_parsed(context, (AstNode *)node->as.postfix.expr);
    iterate(&node->as.postfix.call) on_rval_parsed(context, (AstNode *)it);
    break;
  case NODE_CAST:
    if (node->as.postfix.expr)
      on_expr_parsed(context, (AstNode *)node->as.postfix.expr);
    if (node->as.postfix.cast)
      on_type_parsed(context, (AstNode *)node->as.postfix.cast);
    break;
  case NODE_DEREF:
    if (node->as.postfix.expr)
      on_expr_parsed(context, (AstNode *)node->as.postfix.expr);
    break;
  case NODE_INDEXING:
    if (node->as.postfix.expr)
      on_expr_parsed(context, (AstNode *)node->as.postfix.expr);
    if (node->as.postfix.indexing)
      on_rval_parsed(context, (AstNode *)node->as.postfix.indexing);
    break;
  case NODE_RANGE:
    if (node->as.postfix.expr)
      on_expr_parsed(context, (AstNode *)node->as.postfix.expr);
    if (node->as.postfix.range.from)
      on_rval_parsed(context, (AstNode *)node->as.postfix.range.from);
    if (node->as.postfix.range.to)
      on_rval_parsed(context, (AstNode *)node->as.postfix.range.to);
    break;
  case NODE_TIMES:
  case NODE_DIVIDE:
  case NODE_MOD:
  case NODE_BIT_AND:
  case NODE_PLUS:
  case NODE_MINUS:
  case NODE_BIT_OR:
  case NODE_BIT_NOT:
  case NODE_EQUAL:
  case NODE_NOT_EQUAL:
  case NODE_LT:
  case NODE_GT:
  case NODE_LE:
  case NODE_GE:
  case NODE_LOGICAL_AND:
  case NODE_LOGICAL_OR:
    if (node->as.logical_or.left)
      on_expr_parsed(context, (AstNode *)node->as.binop.left);
    if (node->as.logical_or.right)
      on_expr_parsed(context, (AstNode *)node->as.binop.right);
    break;
  default:
    UNREACHABLE(
        temp_sprintf("unexpected node %s", node_type_to_string(node->type)));
  }

  call_ast_callbacks(Expr, ast);
}

/* Rval */
void on_rval_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  Rval *node = (Rval *)ast;

  switch (node->type) {
  case NODE_EXPR:
    on_expr_parsed(context, (AstNode *)&node->as.expr);
    break;
  case NODE_FUNCTION:
    iterate(&node->as.function.params) {
      on_param_item_parsed(context, (AstNode *)it);
    }
    if (node->as.function.result)
      on_type_parsed(context, (AstNode *)node->as.function.result);
    if (node->as.function.program)
      on_program_parsed(context, (AstNode *)node->as.function.program);
    break;
  case NODE_STRUCT_LITERAL:
    if (node->as.struct_literal.type)
      on_lval_parsed(context, (AstNode *)node->as.struct_literal.type);
    iterate(&node->as.struct_literal.items) {
      on_enum_item_parsed(context, (AstNode *)it);
    }
    break;
  case NODE_TYPE:
    on_type_no_ident_parsed(context, (AstNode *)&node->as.type);
    break;
  case NODE_SWITCH:
    on_expr_parsed(context, (AstNode *)&node->as.switch_.expr);
    iterate(&node->as.switch_.case_) {
      int id = it - node->as.switch_.case_.items;
      on_switch_case_parsed(context, (AstNode *)it);
      on_rval_parsed(context, (AstNode *)&node->as.switch_.rval.items[id]);
    }
    break;
  case NODE_IF:
    if (node->as.if_.val)
      on_rval_parsed(context, (AstNode *)node->as.if_.val);
    on_expr_parsed(context, (AstNode *)&node->as.if_.expr);
    if (node->as.if_.else_val)
      on_rval_parsed(context, (AstNode *)node->as.if_.else_val);
    break;
  default:
    UNREACHABLE(
        temp_sprintf("unexpected node %s", node_type_to_string(node->type)));
  }

  call_ast_callbacks(Rval, ast);
}

/* AssignEq */
void on_assign_eq_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  AssignEq *node = (AssignEq *)ast;

  on_lval_parsed(context, (AstNode *)&node->lval);
  on_rval_parsed(context, (AstNode *)&node->rval);

  call_ast_callbacks(AssignEq, ast);
}

/* Assign */
void on_assign_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  Assign *node = (Assign *)ast;

  on_lval_parsed(context, (AstNode *)&node->lval);

  /* typ may be present */
  if (node->typ.type != NODE_INVALID)
    on_type_parsed(context, (AstNode *)&node->typ);

  on_rval_parsed(context, (AstNode *)&node->rval);

  call_ast_callbacks(Assign, ast);
}

/* ForBody */
void on_for_body_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  ForBody *node = (ForBody *)ast;

  if (node->start)
    on_assign_parsed(context, (AstNode *)node->start);

  on_expr_parsed(context, (AstNode *)&node->expr);

  /* end can be assign_eq or expr depending on end_type */
  switch (node->end_type) {
  case NODE_ASSIGN_EQ:
    if (node->end.assign_eq)
      on_assign_eq_parsed(context, (AstNode *)node->end.assign_eq);
    break;
  case NODE_EXPR:
    if (node->end.expr)
      on_expr_parsed(context, (AstNode *)node->end.expr);
    break;
  case NODE_INVALID:
    break;
  default:
    break;
  }

  call_ast_callbacks(ForBody, ast);
}

/* IfStmt */
void on_if_stmt_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  IfStmt *node = (IfStmt *)ast;

  switch (node->type) {
  case NODE_IF:
    on_expr_parsed(context, (AstNode *)&node->as.if_.expr);
    if (node->as.if_.program)
      on_program_parsed(context, (AstNode *)node->as.if_.program);
    break;
  case NODE_ELSE:
    iterate(&node->as.else_) { on_if_stmt_parsed(context, (AstNode *)it); }
    break;
  case NODE_TAIL:
    if (node->as.tail.start)
      on_if_stmt_parsed(context, (AstNode *)node->as.tail.start);
    if (node->as.tail.end)
      on_program_parsed(context, (AstNode *)node->as.tail.end);
    break;
  default:
    UNREACHABLE(
        temp_sprintf("unexpected node %s", node_type_to_string(node->type)));
  }

  call_ast_callbacks(IfStmt, ast);
}

/* CaseItem */
void on_case_item_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  CaseItem *node = (CaseItem *)ast;

  on_rval_parsed(context, (AstNode *)&node->rval);
  if (node->if_expr)
    on_expr_parsed(context, (AstNode *)node->if_expr);

  call_ast_callbacks(CaseItem, ast);
}

/* SwitchCase */
void on_switch_case_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  SwitchCase *node = (SwitchCase *)ast;

  iterate(&node->items) on_case_item_parsed(context, (AstNode *)it);

  call_ast_callbacks(SwitchCase, ast);
}

/* Function */
void on_function_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  Function *node = (Function *)ast;

  iterate(&node->params) { on_param_item_parsed(context, (AstNode *)it); }

  if (node->result)
    on_type_parsed(context, (AstNode *)node->result);
  if (node->program)
    on_program_parsed(context, (AstNode *)node->program);

  call_ast_callbacks(Function, ast);
}

/* ParamItem */
void on_param_item_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  ParamItem *node = (ParamItem *)ast;

  iterate(&node->idents) { /* identifier tokens are leaves */ }
  on_type_parsed(context, (AstNode *)&node->type);

  call_ast_callbacks(ParamItem, ast);
}

/* EnumItem */
void on_enum_item_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  EnumItem *node = (EnumItem *)ast;

  /* ident token leaf */
  if (node->expr)
    on_expr_parsed(context, (AstNode *)node->expr);

  call_ast_callbacks(EnumItem, ast);
}

/* TypeNoIdent */
void on_type_no_ident_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  TypeNoIdent *node = (TypeNoIdent *)ast;

  switch (node->type) {
  case NODE_UNKNOWN:
    // iterate(&tn->as.unknown.items) { /* token leaves */ }
    break;
  case NODE_ENUM:
    if (node->as.enum_.type)
      on_lval_parsed(context, (AstNode *)node->as.enum_.type);
    iterate(&node->as.enum_.items) {
      on_enum_item_parsed(context, (AstNode *)it);
    }
    break;
  case NODE_STRUCT:
    iterate(&node->as.struct_) { on_param_item_parsed(context, (AstNode *)it); }
    break;
  default:
    UNREACHABLE(
        temp_sprintf("unexpected node %s", node_type_to_string(node->type)));
  }

  call_ast_callbacks(TypeNoIdent, ast);
}

/* Type */
void on_type_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  Type *node = (Type *)ast;

  switch (node->type) {
  case NODE_ID:
    /* ident token leaf */
    break;
  case NODE_OTHER:
    on_type_no_ident_parsed(context, (AstNode *)&node->as.other);
    break;
  case NODE_TUPLE:
    iterate(&node->as.tuple) { on_type_parsed(context, (AstNode *)it); }
    break;
  case NODE_SLICE:
    if (node->as.slice)
      on_type_parsed(context, (AstNode *)node->as.slice);
    break;
  case NODE_PTR:
    if (node->as.ptr)
      on_type_parsed(context, (AstNode *)node->as.ptr);
    break;
  case NODE_FUNCTION:
    iterate(&node->as.function.params) {
      on_type_parsed(context, (AstNode *)it);
    }
    if (node->as.function.result)
      on_type_parsed(context, (AstNode *)node->as.function.result);
    break;
  default:
    UNREACHABLE(
        temp_sprintf("unexpected node %s", node_type_to_string(node->type)));
  }

  call_ast_callbacks(Type, ast);
}

/* Program */
void on_program_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  Program *node = (Program *)ast;

  iterate(&node->stmts) { on_statement_parsed(context, (AstNode *)it); }

  call_ast_callbacks(Program, ast);
}

/* Statement */
void on_statement_parsed(AstCallbackContext context, AstNode *ast) {
  UNUSED(context);
  Stmt *node = (Stmt *)ast;

  switch (node->type) {
  case NODE_ASSIGN:
    on_assign_parsed(context, (AstNode *)&node->as.assign);
    break;
  case NODE_CALL:
    on_lval_parsed(context, (AstNode *)&node->as.call.lval);
    iterate(&node->as.call.params) { on_rval_parsed(context, (AstNode *)it); }
    break;
  case NODE_IF:
    on_if_stmt_parsed(context, (AstNode *)&node->as.if_);
    break;
  case NODE_SWITCH:
    on_expr_parsed(context, (AstNode *)&node->as.switch_.expr);
    iterate(&node->as.switch_.case_) {
      int id = it - node->as.switch_.case_.items;
      on_switch_case_parsed(context, (AstNode *)it);
      on_program_parsed(context, (AstNode *)&node->as.switch_.body.items[id]);
    }
    break;
  case NODE_FLOW_STMT:
    if (node->as.flow_stmt.rval)
      on_rval_parsed(context, (AstNode *)node->as.flow_stmt.rval);
    break;
  case NODE_BLOCK:
    if (node->as.block)
      on_program_parsed(context, (AstNode *)node->as.block);
    break;
  case NODE_LOOP:
    if (node->as.loop)
      on_program_parsed(context, (AstNode *)node->as.loop);
    break;
  case NODE_FOR:
    on_for_body_parsed(context, (AstNode *)&node->as.for_.head);
    if (node->as.for_.stmt)
      on_statement_parsed(context, (AstNode *)node->as.for_.stmt);
    break;
  default:
    UNREACHABLE(
        temp_sprintf("unexpected node %s", node_type_to_string(node->type)));
  }

  call_ast_callbacks(Stmt, ast);
}

void usage(FILE *stream) {
  fprintf(stream, "Usage: ./main [OPTIONS] <INPUT FILES...>\n");
  fprintf(stream, "OPTIONS:\n");
  flag_print_options(stream);
}

int main(int argc, char **argv) {
  bool *help =
      flag_bool("help", false, "Print this help to stdout and exit with 0");
  // char **output_path = flag_str("o", NULL, "Line to output to the
  // file");

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

  set_log_handler(log_handler_stdout);

#define seq(a, b) a b

  seq(let_buf(char, file_buf, 8 << 10),
      let_custom(HM, global_scope,
                 HM_init(&global_scope, sizeof(ScopeObject), 0),
                 HM_deinit(&global_scope))) {
    String_Builder source_file = {file_buf, 0, 8 << 10};

    if (!read_entire_file(*rest_argv, &source_file))
      return 1;

    Lexer lexer = {
        .input = sb_to_sv(source_file),
        .source_file_path = *rest_argv,
        .pos = 0,
        .current_token = {0},
    };
    let_custom(Program, program, program = parse_source_file(lexer).program,
               free_node(program)) {
      AstCallbackContext ctx = {&global_scope, *rest_argv, source_file.items};
      on_program_parsed(ctx, &(AstNode){program});

      // for (HM_Iterator i = NULL; HM_iterate(&global_scope, &i);) {
      //   HM_Entry *ent = HM_entry_index(&global_scope, *i);
      //   printf("key: %.*s\n", (int)ent->key_len, ent->key);
      //   ScopeObject *obj = (void *)ent->value;
      //   printf("val: %s", sok_to_string(obj->kind));
      //   print_node(*obj->as.value.type);
      //   print_node(*obj->as.value.value);
      //   printf("\n\n");
      // }
    }
  }

  return 0;
}
