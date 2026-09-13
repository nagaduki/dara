# meson.build #
project('parser_combinator', 'cpp',
  version : '0.1.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++23',
    'b_sanitize=address',
  ]
)

inc_dir = include_directories('.')

source_files = files(
    'test_main.cpp',
    'test_combinator.cpp',
    'test_lexer.cpp',
    'test_parser.cpp',
    'test_interpreter.cpp',
    'test_printer.cpp',
  #'main.cpp',
    'ast.cpp',
  #'parser.cpp',
    'parser_expr.cpp',
    'parser_decl.cpp',
    'parser_stmt.cpp',
    'lexer.cpp',
    'utils.cpp',
    'value.cpp',
    'class.cpp',
    'instance.cpp',
    'builtin.cpp',
  #  'printer.cpp',
    'printer_dot.cpp',
    'printer_string.cpp',
    'callable.cpp',
    'interpreter.cpp',
)

test_exe = executable('test_combinator',
  sources : source_files,
  include_directories : inc_dir
)


# args オプションに -fc=1 を追加
# test('parser tests', test_exe, args: ['-s', '-fc=1'])
test('parser tests', test_exe)

