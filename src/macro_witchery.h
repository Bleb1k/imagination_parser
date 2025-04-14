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
#define one_of_1(val)
#define one_of_2(val, cmp) (val == cmp)
#define one_of_3(val, cmp, ...) (val == cmp) || one_of_2(val, __VA_ARGS__)
#define one_of_4(val, cmp, ...) (val == cmp) || one_of_3(val, __VA_ARGS__)
#define one_of_5(val, cmp, ...) (val == cmp) || one_of_4(val, __VA_ARGS__)
#define one_of_6(val, cmp, ...) (val == cmp) || one_of_5(val, __VA_ARGS__)
#define one_of_7(val, cmp, ...) (val == cmp) || one_of_6(val, __VA_ARGS__)
#define one_of_8(val, cmp, ...) (val == cmp) || one_of_7(val, __VA_ARGS__)
#define one_of_9(val, cmp, ...) (val == cmp) || one_of_8(val, __VA_ARGS__)

#define va_stringify(...) VFUNC(va_stringify_, __VA_ARGS__)
#define va_stringify_1(val) #val
#define va_stringify_2(val, ...) #val, va_stringify_1(__VA_ARGS__)
#define va_stringify_3(val, ...) #val, va_stringify_2(__VA_ARGS__)
#define va_stringify_4(val, ...) #val, va_stringify_3(__VA_ARGS__)
#define va_stringify_5(val, ...) #val, va_stringify_4(__VA_ARGS__)
#define va_stringify_6(val, ...) #val, va_stringify_5(__VA_ARGS__)
#define va_stringify_7(val, ...) #val, va_stringify_6(__VA_ARGS__)
#define va_stringify_8(val, ...) #val, va_stringify_7(__VA_ARGS__)
#define va_stringify_9(val, ...) #val, va_stringify_8(__VA_ARGS__)

#define PREPEND(name, ...) VFUNC(__PREPEND_, name, __VA_ARGS__)
#define __PREPEND_1(name)
#define __PREPEND_2(name, ...) name##__VA_ARGS__
#define __PREPEND_3(name, v, ...) name##v, __PREPEND_2(name, __VA_ARGS__)
#define __PREPEND_4(name, v, ...) name##v, __PREPEND_3(name, __VA_ARGS__)
#define __PREPEND_5(name, v, ...) name##v, __PREPEND_4(name, __VA_ARGS__)
#define __PREPEND_6(name, v, ...) name##v, __PREPEND_5(name, __VA_ARGS__)
#define __PREPEND_7(name, v, ...) name##v, __PREPEND_6(name, __VA_ARGS__)
#define __PREPEND_8(name, v, ...) name##v, __PREPEND_7(name, __VA_ARGS__)
#define __PREPEND_9(name, v, ...) name##v, __PREPEND_8(name, __VA_ARGS__)

#define ok_or_return(bool_val, ret_val)                                        \
  do {                                                                         \
    if (!(bool_val)) {                                                         \
      /*nob_log(NOB_ERROR, "%s:%d: %s: Code is not okay\n", __FILE__,          \
         __LINE__,                                                             \
                __FUNCTION__);*/                                               \
      return (ret_val);                                                        \
    }                                                                          \
  } while (0)

#define PARSING_ERROR(...)                                                     \
  do {                                                                         \
    char *types[] = {__VA_ARGS__};                                             \
    nob_log(NOB_ERROR, "%s:%d:%d  (%s:%d)\nFound: '%s'\nExpected one of:",     \
            (lexer)->source_file_path, lexer->line_count,                      \
            (lexer)->pos - (lexer)->last_newline_pos, __FILE__, __LINE__,      \
            token_type_to_string((lexer)->current_token.type));                \
    for (size_t i = 0; i < __NARG__(__VA_ARGS__); i++) {                       \
      fprintf(stderr, "  - '%s'\n", types[i]);                                 \
    }                                                                          \
    exit(123);                                                                 \
  } while (0)

#define consume_selection(lexer, ...)                                          \
  do {                                                                         \
    choice_depth += 1;                                                         \
    TokenType types[] = {__VA_ARGS__};                                         \
    Token found = {0};                                                         \
    for (size_t i = 0; i < __NARG__(__VA_ARGS__); i++) {                       \
      if ((lexer)->current_token.type == types[i]) {                           \
        found = (lexer)->current_token;                                        \
        printf("Found token with type:'%s', lexeme:'%s', at pos:'%d'\n",       \
               token_type_to_string(found.type), found.lexeme,                 \
               found.char_pos);                                                \
        lexer_next_token(lexer);                                               \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    if (!--choice_depth && found.lexeme == NULL && found.type == TOKEN_NULL) { \
      PARSING_ERROR(va_stringify(__VA_ARGS__));                                \
    }                                                                          \
  } while (0)

#define consume(lexer, ...) consume_selection(lexer, __VA_ARGS__)
#define consume_str(lexer, ...)                                                \
  do {                                                                         \
    choice_depth += 1;                                                         \
    const char *symbols[] = {__VA_ARGS__};                                     \
    size_t pos = 0;                                                            \
    for (size_t i = 0; i < __NARG__(__VA_ARGS__); i++) {                       \
      Lexer copy = *(lexer);                                                   \
      const char *sym = symbols[i];                                            \
      size_t len = strlen(sym);                                                \
      pos = 0;                                                                 \
      while (pos < len) {                                                      \
        /* printf("%s(%s)\n", token_type_to_string(copy.current_token.type),*/ \
        /*copy.current_token.lexeme);*/                                        \
        if (copy.current_token.type != TOKEN_SYMBOL ||                         \
            copy.current_token.lexeme[0] != sym[pos]) {                        \
          break;                                                               \
        }                                                                      \
        lexer_next_token(&copy);                                               \
        pos++;                                                                 \
      }                                                                        \
      if (pos == len) {                                                        \
        *(lexer) = copy;                                                       \
        break;                                                                 \
      }                                                                        \
      if (i == (__NARG__(__VA_ARGS__) - 1) && !--choice_depth) {               \
        PARSING_ERROR(__VA_ARGS__);                                            \
      }                                                                        \
    }                                                                          \
  } while (0)

#define parse_(lexer, ast_node, ...)                                           \
  do {                                                                         \
    choice_depth += 1;                                                         \
    AstNode (*parsers[])(Lexer *) = {PREPEND(parse_, __VA_ARGS__)};            \
    for (size_t i = 0; i < __NARG__(__VA_ARGS__); i++) {                       \
      Lexer lexer_copy = *(lexer);                                             \
      (ast_node) = parsers[i](&lexer_copy);                                    \
      if ((ast_node).type != NODE_NULL) {                                      \
        *(lexer) = lexer_copy;                                                 \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    if (!--choice_depth && (ast_node).type == NODE_NULL) {                     \
      PARSING_ERROR(va_stringify(__VA_ARGS__));                                \
    }                                                                          \
  } while (0)

#define new_ast_node(Type, lexer)                                              \
  ((AstNode){                                                                  \
      .type = 0,                                                               \
      .source = {.count = 0,                                                   \
                 .items =                                                      \
                     ((const char *)((lexer)->input.data + (lexer)->pos))},    \
      .node = &(Type){0},                                                      \
  })
#define compose_ast_node(lexer, _node, node_type)                              \
  ((AstNode){                                                                  \
      .type = (node_type),                                                     \
      .source = {.count = (int)(((lexer)->input.data + (lexer)->pos) -         \
                                (_node).source.items),                         \
                 .items = (_node).source.items},                               \
      .node = (_node).node,                                                    \
  })
