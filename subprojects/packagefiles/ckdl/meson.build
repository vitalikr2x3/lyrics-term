# Build file for ckdl, copied into the cloned subproject by its wrap (patch_directory).
# ckdl ships only CMake files; this builds the C library directly.
project('ckdl', 'c', version : '0.8.0', default_options : ['warning_level=1'])

ckdl_inc = include_directories('include')

ckdl_lib = static_library(
  'kdl',
  files(
    'src/bigint.c',
    'src/compat.c',
    'src/emitter.c',
    'src/parser.c',
    'src/str.c',
    'src/tokenizer.c',
    'src/utf8.c',
  ),
  include_directories : ckdl_inc,
  c_args : ['-DKDL_STATIC_LIB'],
  pic : true,
)

ckdl_dep = declare_dependency(
  link_with : ckdl_lib,
  include_directories : ckdl_inc,
  compile_args : ['-DKDL_STATIC_LIB'],
)
