#ifndef REACH_H
#define REACH_H

#include "../ast/ast.h"
#include "../pass/pass.h"
#include "../codegen/gendebug.h"
#include "../../libponyrt/ds/hash.h"
#include "../../libponyrt/ds/stack.h"

#include <llvm-c/Core.h>

PONY_EXTERN_C_BEGIN

typedef struct reach_method_t reach_method_t;
typedef struct reach_method_name_t reach_method_name_t;
typedef struct reach_field_t reach_field_t;
typedef struct reach_param_t reach_param_t;
typedef struct reach_type_t reach_type_t;

DECLARE_STACK(reachable_expr_stack, reachable_expr_stack_t, ast_t);
DECLARE_STACK(reach_method_stack, reach_method_stack_t, reach_method_t);
DECLARE_HASHMAP_SERIALISE(reach_methods, reach_methods_t, reach_method_t);
DECLARE_HASHMAP_SERIALISE(reach_mangled, reach_mangled_t, reach_method_t);
DECLARE_HASHMAP_SERIALISE(reach_method_names, reach_method_names_t,
  reach_method_name_t);
DECLARE_HASHMAP_SERIALISE(reach_types, reach_types_t, reach_type_t);
DECLARE_HASHMAP_SERIALISE(reach_type_cache, reach_type_cache_t, reach_type_t);

struct reach_method_t
{
  const char* name;
  const char* mangled_name;
  const char* full_name;

  token_id cap;
  ast_t* typeargs;
  ast_t* r_fun;
  uint32_t vtable_index;

  LLVMTypeRef func_type;
  LLVMTypeRef msg_type;
  LLVMValueRef func;
  LLVMValueRef func_handler;
  LLVMMetadataRef di_method;
  LLVMMetadataRef di_file;

  // Mark as true if the compiler supplies an implementation.
  bool intrinsic;

  // Mark as true if the compiler supplies an implementation and the function
  // isn't exposed to the user at all. The method will also be automatically
  // added to supertypes.
  bool internal;

  // Mark as true if the method is a forwarding method.
  bool forwarding;

  // Linked list of instantiations that use the same func.
  reach_method_t* subordinate;

  size_t param_count;
  reach_param_t* params;
  reach_type_t* result;
};

struct reach_method_name_t
{
  token_id id;
  token_id cap;
  const char* name;
  reach_methods_t r_methods;
  reach_mangled_t r_mangled;
  bool internal;
};

struct reach_field_t
{
  ast_t* ast;
  reach_type_t* type;
  bool embed;
};

struct reach_param_t
{
  reach_type_t* type;
  token_id cap;
};

struct reach_type_t
{
  const char* name;
  const char* mangle;
  ast_t* ast;
  ast_t* ast_cap;
  token_id underlying;

  reach_method_names_t methods;
  reach_method_t* bare_method;
  reach_type_cache_t subtypes;
  uint32_t type_id;
  size_t abi_size;
  uint32_t vtable_size;
  bool can_be_boxed;
  bool is_trait;

  LLVMTypeRef structure;
  LLVMTypeRef structure_ptr;
  LLVMTypeRef primitive;
  LLVMTypeRef use_type;

  LLVMTypeRef desc_type;
  LLVMValueRef desc;
  LLVMValueRef instance;
  LLVMValueRef trace_fn;
  LLVMValueRef serialise_trace_fn;
  LLVMValueRef serialise_fn;
  LLVMValueRef deserialise_fn;
  LLVMValueRef custom_serialise_space_fn;
  LLVMValueRef custom_serialise_fn;
  LLVMValueRef custom_deserialise_fn;
  LLVMValueRef final_fn;
  LLVMValueRef dispatch_fn;
  LLVMValueRef dispatch_switch;

  LLVMMetadataRef di_file;
  LLVMMetadataRef di_type;
  LLVMMetadataRef di_type_embed;

  uint32_t field_count;
  reach_field_t* fields;
};

typedef struct
{
  reach_types_t types;
  reachable_expr_stack_t* expr_stack;
  reach_method_stack_t* method_stack;
  uint32_t object_type_count;
  uint32_t numeric_type_count;
  uint32_t tuple_type_count;
  uint32_t total_type_count;
  uint32_t trait_type_count;
} reach_t;

/// Allocate a new set of reachable types.
reach_t* reach_new();

/// Free a set of reachable types.
void reach_free(reach_t* r);

/** Determine code reachability for a method in a type.
 *
 * The type should be a nominal, including any typeargs. The supplied method
 * typeargs can be NULL if there are none.
 */
void reach(reach_t* r, ast_t* type, const char* name, ast_t* typeargs,
  pass_opt_t* opt);

/** Finalise reached types before use.
 *
 * This must be called once all the necessary reachability analysis has been
 * done and before using the data.
 */
void reach_done(reach_t* r, pass_opt_t* opt);

reach_type_t* reach_type(reach_t* r, ast_t* type);

reach_type_t* reach_type_name(reach_t* r, const char* name);

reach_method_t* reach_method(reach_type_t* t, token_id cap,
  const char* name, ast_t* typeargs);

reach_method_name_t* reach_method_name(reach_type_t* t,
  const char* name);

uint32_t reach_vtable_index(reach_type_t* t, const char* name);

void reach_dump(reach_t* r);

pony_type_t* reach_method_pony_type();

pony_type_t* reach_method_name_pony_type();

pony_type_t* reach_field_pony_type();

pony_type_t* reach_param_pony_type();

pony_type_t* reach_type_pony_type();

pony_type_t* reach_pony_type();

PONY_EXTERN_C_END

#endif
