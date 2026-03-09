#include <stddef.h>

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

#define va_items_stringify(...) VFUNC(va_items_stringify_, __VA_ARGS__)
#define va_items_stringify_1(val) "<" #val ">"
#define va_items_stringify_2(val, ...)                                         \
  "<" #val ">, " va_items_stringify_1(__VA_ARGS__)
#define va_items_stringify_3(val, ...)                                         \
  "<" #val ">, " va_items_stringify_2(__VA_ARGS__)
#define va_items_stringify_4(val, ...)                                         \
  "<" #val ">, " va_items_stringify_3(__VA_ARGS__)
#define va_items_stringify_5(val, ...)                                         \
  "<" #val ">, " va_items_stringify_4(__VA_ARGS__)
#define va_items_stringify_6(val, ...)                                         \
  "<" #val ">, " va_items_stringify_5(__VA_ARGS__)
#define va_items_stringify_7(val, ...)                                         \
  "<" #val ">, " va_items_stringify_6(__VA_ARGS__)
#define va_items_stringify_8(val, ...)                                         \
  "<" #val ">, " va_items_stringify_7(__VA_ARGS__)
#define va_items_stringify_9(val, ...)                                         \
  "<" #val ">, " va_items_stringify_8(__VA_ARGS__)
#define va_items_stringify_10(val, ...)                                        \
  "<" #val ">, " va_items_stringify_9(__VA_ARGS__)
#define va_items_stringify_11(val, ...)                                        \
  "<" #val ">, " va_items_stringify_10(__VA_ARGS__)
#define va_items_stringify_12(val, ...)                                        \
  "<" #val ">, " va_items_stringify_11(__VA_ARGS__)
#define va_items_stringify_13(val, ...)                                        \
  "<" #val ">, " va_items_stringify_12(__VA_ARGS__)
#define va_items_stringify_14(val, ...)                                        \
  "<" #val ">, " va_items_stringify_13(__VA_ARGS__)
#define va_items_stringify_15(val, ...)                                        \
  "<" #val ">, " va_items_stringify_14(__VA_ARGS__)
#define va_items_stringify_16(val, ...)                                        \
  "<" #val ">, " va_items_stringify_15(__VA_ARGS__)

#define va_strs_stringify(...) VFUNC(va_strs_stringify_, __VA_ARGS__)
#define va_strs_stringify_1(val) "\"" #val "\""
#define va_strs_stringify_2(val, ...)                                          \
  "\"" #val "\", " va_strs_stringify_1(__VA_ARGS__)
#define va_strs_stringify_3(val, ...)                                          \
  "\"" #val "\", " va_strs_stringify_2(__VA_ARGS__)
#define va_strs_stringify_4(val, ...)                                          \
  "\"" #val "\", " va_strs_stringify_3(__VA_ARGS__)
#define va_strs_stringify_5(val, ...)                                          \
  "\"" #val "\", " va_strs_stringify_4(__VA_ARGS__)
#define va_strs_stringify_6(val, ...)                                          \
  "\"" #val "\", " va_strs_stringify_5(__VA_ARGS__)
#define va_strs_stringify_7(val, ...)                                          \
  "\"" #val "\", " va_strs_stringify_6(__VA_ARGS__)
#define va_strs_stringify_8(val, ...)                                          \
  "\"" #val "\", " va_strs_stringify_7(__VA_ARGS__)
#define va_strs_stringify_9(val, ...)                                          \
  "\"" #val "\", " va_strs_stringify_8(__VA_ARGS__)

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
#define __PREPEND_10(name, v, ...) name##v, __PREPEND_9(name, __VA_ARGS__)
#define __PREPEND_11(name, v, ...) name##v, __PREPEND_10(name, __VA_ARGS__)
#define __PREPEND_12(name, v, ...) name##v, __PREPEND_11(name, __VA_ARGS__)
#define __PREPEND_13(name, v, ...) name##v, __PREPEND_12(name, __VA_ARGS__)
#define __PREPEND_14(name, v, ...) name##v, __PREPEND_13(name, __VA_ARGS__)
#define __PREPEND_15(name, v, ...) name##v, __PREPEND_14(name, __VA_ARGS__)
#define __PREPEND_16(name, v, ...) name##v, __PREPEND_15(name, __VA_ARGS__)

#define debug_this(...) (dbg_lvl += 1, padded_puts("", 2), nob_log(INFO, "%s:%d: %s", __FILE__, __LINE__, __FUNCTION__))
#ifdef PARSER_DEBUG
#define __consume_tok_log_searching(...)                                       \
  nob_log(INFO, "\\ %s: Searching tokens: " va_items_stringify(__VA_ARGS__),   \
          __FUNCTION__);
#define __consume_tok_log_found(...)                                           \
  nob_log(INFO, "/ %s: Found token <%s>, lexeme '%.*s'",          \
          __FUNCTION__, token_type_to_string(found.type),                      \
          (int)found.lexeme.count, found.lexeme.data);
#define __consume_tok_log_wrong(...)                                           \
  nob_log(NOB_WARNING,                                                         \
          "/ %s:%d: %s: Found <%s> '%.*s', but expected one "                  \
          "of: " va_items_stringify(__VA_ARGS__),                              \
          __FILE__, __LINE__, __FUNCTION__,                                    \
          token_type_to_string((lexer)->current_token.type),                   \
          (int)(lexer)->current_token.lexeme.count,                            \
          (lexer)->current_token.lexeme.data);
#else
#define __consume_tok_log_searching(...)
#define __consume_tok_log_found(...)
#define __consume_tok_log_wrong(...)
#endif // PARSER_DEBUG

#define __consume_tok_body(on_error, lexer, ...)                               \
  do {                                                                         \
    ++choice_depth;                                                            \
    __consume_tok_log_searching(__VA_ARGS__);                                  \
    TokenType types[] = {PREPEND(TOKEN_, __VA_ARGS__)};                        \
    Token found = {0};                                                         \
    for (size_t i = 0; i < __NARG__(__VA_ARGS__); i++) {                       \
      if ((lexer)->current_token.type == types[i]) {                           \
        found = (lexer)->current_token;                                        \
        __consume_tok_log_found(__VA_ARGS__);                                  \
        lexer_next_token(lexer);                                               \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    --choice_depth;                                                            \
    if (found.type == 0) {                                            \
      __consume_tok_log_wrong(__VA_ARGS__);                                    \
      if (!choice_depth)                                                       \
        __builtin_trap();                                                      \
      on_error;                                                                \
    }                                                                          \
  } while (0)

#define consume_tok_opt(lexer, ...) __consume_tok_body(, lexer, __VA_ARGS__)

#define consume_tok(lexer, ...)                                                \
  __consume_tok_body(goto throw, lexer, __VA_ARGS__)

#ifdef PARSER_DEBUG
#define consume_str_or(on_error, lexer, type, ...)                             \
  ({                                                                           \
    int res =                                                                  \
        __consume_str(lexer, TOKEN_##type, (const char *[]){__VA_ARGS__},      \
                      __NARG__(__VA_ARGS__), #__VA_ARGS__, __FUNCTION__,       \
                      __FILE__, __LINE__);                                     \
    if (res < 0) {                                                             \
      on_error;                                                                \
    }                                                                          \
    res;                                                                       \
  })
#else
#define consume_str_or(on_error, lexer, type, ...)                             \
  ({                                                                           \
    int res =                                                                  \
        __consume_str(lexer, TOKEN_##type, (const char *[]){__VA_ARGS__},      \
                      __NARG__(__VA_ARGS__));                                  \
    if (res < 0) {                                                             \
      on_error;                                                                \
    }                                                                          \
    res;                                                                       \
  })
#endif // PARSER_DEBUG

#define consume_str_opt(lexer, type, ...)                                      \
  consume_str_or(, lexer, type, __VA_ARGS__)

#define consume_str(lexer, type, ...)                                          \
  consume_str_or(goto throw, lexer, type, __VA_ARGS__)

#ifdef PARSER_DEBUG
#define parse_or(on_error, lexer, ast_node, ...)                               \
  ({                                                                           \
    int res = __parse(                                                         \
        (lexer), (AstNode *)&(ast_node),                                       \
        (bool (*[])(Lexer *, void *)){PREPEND(parse_, __VA_ARGS__)},        \
        __NARG__(__VA_ARGS__), (const char *[]){va_stringify(__VA_ARGS__)}, __FUNCTION__,  \
        __FILE__, __LINE__);                                                   \
    if (res < 0) {                                                             \
      on_error;                                                                \
    }                                                                          \
    res;                                                                       \
  })
#else
#define parse_or(on_error, lexer, ast_node, ...)                               \
  ({                                                                           \
    int res = __parse(                                                         \
        lexer, (AstNode *)&(ast_node),                                                      \
        (bool (*[])(Lexer *, void *)){PREPEND(parse_, __VA_ARGS__)},        \
        __NARG__(__VA_ARGS__));                                                \
    if (res < 0) {                                                             \
      on_error;                                                                \
    }                                                                          \
    res;                                                                       \
  })
#endif // PARSER_DEBUG

#define parse_or_throw(lexer, ast_node, ...)                                   \
  parse_or(goto throw, lexer, ast_node, __VA_ARGS__)
#define parse_(lexer, ast_node, ...) parse_or(, lexer, ast_node, __VA_ARGS__)

#define new_node(Type, lexer)                                              \
  ((Type){.source = {.data = (lexer)->current_token.lexeme.data, .count = 0}})
// ((AstNode){ \
//     .source = {.data = (lexer)->current_token.lexeme.data, .count = 0}, \
//     .node = calloc(1, sizeof(Type)), \
// })

#define cast(type, val) ((type)val)

#define sized_enum(type, name)                                                 \
  typedef type name;                                                           \
  enum
// #define block(start, end) \
//   for (int __tmp_block_##__LINE__ = ((start), 1); __tmp_block_##__LINE__--; \
//        (end))
#define let_custom(typ, name, start, end)                                      \
  for (typ name = {0}, *__tmp_block_##__LINE__ = NULL;                         \
       (__tmp_block_##__LINE__ == NULL ? (void)(start) : (void)1,              \
        __tmp_block_##__LINE__ == NULL);                                       \
       end, __tmp_block_##__LINE__ = (void *)1)
#define let_buf(typ, varname, count)                                           \
  for (typ *varname = malloc(sizeof(typ) * (count)); varname;                  \
       free(varname), varname = NULL)
#define let(typ, varname) let_buf(typ, varname, 1)

#define iterate(da) da_foreach(auto, it, da)
