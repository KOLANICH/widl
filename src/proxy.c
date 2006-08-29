/*
 * IDL Compiler
 *
 * Copyright 2002 Ove Kaaven
 * Copyright 2004 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typegen.h"

#define END_OF_LIST(list)       \
  do {                          \
    if (list) {                 \
      while (NEXT_LINK(list))   \
        list = NEXT_LINK(list); \
    }                           \
  } while(0)

static FILE* proxy;
static int indent = 0;

/* FIXME: support generation of stubless proxies */

static int print_proxy( const char *format, ... )
{
  va_list va;
  int i, r;

  va_start( va, format );
  if ( format[0] != '\n' )
    for( i=0; i<indent; i++ )
      fprintf( proxy, "    " );
  r = vfprintf( proxy, format, va );
  va_end( va );
  return r;
}


static type_t *get_base_type( var_t *arg )
{
  type_t *t = arg->type;
  while( (t->type == 0) && t->ref )
    t = t->ref;
  return t;
}

static void write_stubdescproto(void)
{
  print_proxy( "extern const MIDL_STUB_DESC Object_StubDesc;\n");
  print_proxy( "\n");
}

static void write_stubdesc(void)
{
  print_proxy( "const MIDL_STUB_DESC Object_StubDesc = {\n");
  print_proxy( "    0,\n");
  print_proxy( "    NdrOleAllocate,\n");
  print_proxy( "    NdrOleFree,\n");
  print_proxy( "    {0}, 0, 0, 0, 0,\n");
  print_proxy( "    0 /* __MIDL_TypeFormatString.Format */\n");
  print_proxy( "};\n");
  print_proxy( "\n");
}

static void init_proxy(ifref_t *ifaces)
{
  if (proxy) return;
  if(!(proxy = fopen(proxy_name, "w")))
    error("Could not open %s for output\n", proxy_name);
  print_proxy( "/*** Autogenerated by WIDL %s from %s - Do not edit ***/\n", PACKAGE_VERSION, input_name);
  print_proxy( "\n");
  print_proxy( "#ifndef __REDQ_RPCPROXY_H_VERSION__\n");
  print_proxy( "#define __REQUIRED_RPCPROXY_H_VERSION__ 440\n");
  print_proxy( "#endif /* __REDQ_RPCPROXY_H_VERSION__ */\n");
  print_proxy( "\n");
  print_proxy( "#include \"rpcproxy.h\"\n");
  print_proxy( "#ifndef __RPCPROXY_H_VERSION__\n");
  print_proxy( "#error This code needs a newer version of rpcproxy.h\n");
  print_proxy( "#endif /* __RPCPROXY_H_VERSION__ */\n");
  print_proxy( "\n");
  print_proxy( "#include \"%s\"\n", header_name);
  print_proxy( "\n");
  write_formatstringsdecl(proxy, indent, ifaces, 1);
  write_stubdescproto();
}

static void clear_output_vars( var_t *arg )
{
  END_OF_LIST(arg);
  while (arg) {
    if (is_attr(arg->attrs, ATTR_OUT) && !is_attr(arg->attrs, ATTR_IN)) {
      print_proxy( "if(%s)\n", arg->name );
      indent++;
      print_proxy( "MIDL_memset( %s, 0, sizeof( *%s ));\n", arg->name, arg->name );
      indent--;
    }
    arg = PREV_LINK(arg);
  }
}

int is_var_ptr(var_t *v)
{
  return v->ptr_level || is_ptr(v->type);
}

int cant_be_null(var_t *v)
{
  /* Search backwards for the most recent pointer attribute.  */
  const attr_t *attrs = v->attrs;
  const type_t *type = v->type;

  if (! attrs && type)
  {
    attrs = type->attrs;
    type = type->ref;
  }

  while (attrs)
  {
    int t = get_attrv(attrs, ATTR_POINTERTYPE);

    if (t == RPC_FC_FP || t == RPC_FC_OP || t == RPC_FC_UP)
      return 0;

    if (t == RPC_FC_RP)
      return 1;

    if (type)
    {
      attrs = type->attrs;
      type = type->ref;
    }
    else
      attrs = NULL;
  }

  return 1;                             /* Default is RPC_FC_RP.  */
}

static int is_user_derived(var_t *v)
{
  const attr_t *attrs = v->attrs;
  const type_t *type = v->type;

  if (! attrs && type)
  {
    attrs = type->attrs;
    type = type->ref;
  }

  while (attrs)
  {
    if (is_attr(attrs, ATTR_WIREMARSHAL))
      return 1;

    if (type)
    {
      attrs = type->attrs;
      type = type->ref;
    }
    else
      attrs = NULL;
  }

  return 0;
}

static void proxy_check_pointers( var_t *arg )
{
  END_OF_LIST(arg);
  while (arg) {
    if (is_var_ptr(arg) && cant_be_null(arg)) {
        print_proxy( "if(!%s)\n", arg->name );
        indent++;
        print_proxy( "RpcRaiseException(RPC_X_NULL_REF_POINTER);\n");
        indent--;
    }
    arg = PREV_LINK(arg);
  }
}

static void marshall_size_arg( var_t *arg )
{
  int index = 0;
  const type_t *type = get_base_type(arg);
  expr_t *expr;

  expr = get_attrp( arg->attrs, ATTR_SIZEIS );
  if (expr)
  {
    print_proxy( "_StubMsg.MaxCount = ", arg->name );
    write_expr(proxy, expr, 0);
    fprintf(proxy, ";\n\n");
    print_proxy( "NdrConformantArrayBufferSize( &_StubMsg, (unsigned char*)%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d]);\n", index );
    return;
  }

  if (is_user_derived(arg))
  {
    print_proxy("NdrUserMarshalBufferSize( &_StubMsg, (unsigned char*)%s, ", arg->name);
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d] );\n", index);
    return;
  }

  switch( type->type )
  {
  case RPC_FC_BYTE:
  case RPC_FC_CHAR:
    print_proxy( "_StubMsg.BufferLength += %d; /* %s */\n", 1, arg->name );
    break;

  case RPC_FC_WCHAR:
  case RPC_FC_SHORT:
  case RPC_FC_USHORT:
  case RPC_FC_ENUM16:
    print_proxy( "_StubMsg.BufferLength += %d; /* %s */\n", 2, arg->name );
    break;

  case RPC_FC_LONG:
  case RPC_FC_ULONG:
  case RPC_FC_ENUM32:
    print_proxy( "_StubMsg.BufferLength += %d; /* %s */\n", 4, arg->name );
    break;
      
  case RPC_FC_STRUCT:
    print_proxy( "NdrSimpleStructBufferSize(&_StubMsg, (unsigned char*)%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d] );\n", index );
    break;

  case RPC_FC_C_CSTRING:
  case RPC_FC_C_WSTRING:
  case RPC_FC_CARRAY:
    print_proxy( "NdrConformantArrayBufferSize( &_StubMsg, (unsigned char*)%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d]);\n", index );
    break;

  case RPC_FC_BOGUS_STRUCT:
    print_proxy( "NdrComplexStructBufferSize(&_StubMsg, (unsigned char*)%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d] );\n", index );
    break;

  case RPC_FC_FP:
    {
      var_t temp;
      memset( &temp, 0, sizeof temp );
      temp.type = type->ref;
      temp.name = arg->name; /* FIXME */
#if 0
      print_proxy( "/* FIXME: %s use the right name for %s */\n", __FUNCTION__, arg->name );
#endif
      marshall_size_arg( &temp );
    }
    break;

  case RPC_FC_IP:
    print_proxy( "NdrPointerBufferSize( &_StubMsg, (unsigned char*)%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d]);\n", index );
    break;

  default:
    print_proxy("/* FIXME: %s code for %s type %d missing */\n", __FUNCTION__, arg->name, type->type );
  }
}

static void proxy_gen_marshall_size( var_t *arg )
{
  print_proxy( "_StubMsg.BufferLength = 0U;\n" );

  END_OF_LIST(arg);
  while (arg) {
    if (is_attr(arg->attrs, ATTR_IN)) 
    {
      marshall_size_arg( arg );
      fprintf(proxy, "\n");
    }
    arg = PREV_LINK(arg);
  }
}

static void marshall_copy_arg( var_t *arg )
{
  int index = 0;
  type_t *type = get_base_type(arg);
  expr_t *expr;

  expr = get_attrp( arg->attrs, ATTR_SIZEIS );
  if (expr)
  {
    print_proxy( "_StubMsg.MaxCount = ", arg->name );
    write_expr(proxy, expr, 0);
    fprintf(proxy, ";\n\n");
    print_proxy( "NdrConformantArrayMarshall( &_StubMsg, (unsigned char*)%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d]);\n", index );
    return;
  }

  if (is_user_derived(arg))
  {
    print_proxy("NdrUserMarshalMarshall( &_StubMsg, (unsigned char*)%s, ", arg->name);
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d] );\n", index);
    return;
  }

  switch( type->type )
  {
  case RPC_FC_BYTE:
  case RPC_FC_CHAR:
  case RPC_FC_WCHAR:
  case RPC_FC_SHORT:
  case RPC_FC_USHORT:
  case RPC_FC_ENUM16:
  case RPC_FC_LONG:
  case RPC_FC_ULONG:
  case RPC_FC_ENUM32:
    print_proxy( "*(");
    write_type(proxy, arg->type, arg, arg->tname);
    fprintf(proxy, " *)_StubMsg.Buffer = %s;\n", arg->name );
    print_proxy("_StubMsg.Buffer += sizeof(");
    write_type(proxy, arg->type, arg, arg->tname);
    fprintf(proxy, ");\n");
    break;
      
  case RPC_FC_STRUCT:
    /* FIXME: add the format string, and set the index below */
    print_proxy( "NdrSimpleStructMarshall(&_StubMsg, (unsigned char*)%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d]);\n", index );
    break;

  case RPC_FC_C_CSTRING:
  case RPC_FC_C_WSTRING:
  case RPC_FC_CARRAY:
    break;

  case RPC_FC_BOGUS_STRUCT:
    print_proxy( "NdrComplexStructMarshall(&_StubMsg, (unsigned char*)%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d] );\n", index );
    break;

  case RPC_FC_FP:
    {
      var_t temp;
      memset( &temp, 0, sizeof temp );
      temp.type = type->ref;
      temp.name = arg->name; /* FIXME */
#if 0
      print_proxy( "/* FIXME: %s use the right name for %s */\n", __FUNCTION__, arg->name );
#endif
      marshall_copy_arg( &temp );
    }
    break;

  case RPC_FC_IP:
    print_proxy( "NdrPointerMarshall( &_StubMsg, (unsigned char*)%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d]);\n", index );
    break;

  default:
    print_proxy("/* FIXME: %s code for %s type %d missing */\n", __FUNCTION__, arg->name, type->type );
  }
}

static void gen_marshall_copydata( var_t *arg )
{
  END_OF_LIST(arg);
  while (arg) {
    if (is_attr(arg->attrs, ATTR_IN)) 
    {
      marshall_copy_arg( arg );
      fprintf(proxy, "\n");
    }
    arg = PREV_LINK(arg);
  }
}

static void gen_marshall( var_t *arg )
{
  /* generated code to determine the size of the buffer required */
  proxy_gen_marshall_size( arg );

  /* generated code to allocate the buffer */
  print_proxy( "NdrProxyGetBuffer(This, &_StubMsg);\n" );

  /* generated code to copy the args into the buffer */
  gen_marshall_copydata( arg );

  print_proxy( "\n");
}

static void unmarshall_copy_arg( var_t *arg )
{
  int index = 0;
  type_t *type = get_base_type(arg);
  expr_t *expr;

  expr = get_attrp( arg->attrs, ATTR_SIZEIS );
  if (expr)
  {
    print_proxy( "NdrConformantArrayUnmarshall( &_StubMsg, (unsigned char**)&%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d], 0);\n", index );
    return;
  }

  if (is_user_derived(arg))
  {
    print_proxy("NdrUserMarshalUnmarshall( &_StubMsg, (unsigned char**)&%s, ", arg->name);
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d], 0 );\n", index);
    return;
  }

  switch( type->type )
  {
  case RPC_FC_BYTE:
  case RPC_FC_CHAR:
  case RPC_FC_WCHAR:
  case RPC_FC_SHORT:
  case RPC_FC_USHORT:
  case RPC_FC_ENUM16:
  case RPC_FC_LONG:
  case RPC_FC_ULONG:
  case RPC_FC_ENUM32:
    print_proxy( "%s = *(", arg->name );
    write_type(proxy, arg->type, arg, arg->tname);
    fprintf(proxy," *)_StubMsg.Buffer;\n");
    print_proxy("_StubMsg.Buffer += sizeof(");
    write_type(proxy, arg->type, arg, arg->tname);
    fprintf(proxy, ");\n");
    break;
      
  case RPC_FC_STRUCT:
    print_proxy( "NdrSimpleStructUnmarshall(&_StubMsg, (unsigned char**)%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d], 0);\n", index );
    break;

  case RPC_FC_C_CSTRING:
  case RPC_FC_C_WSTRING:
  case RPC_FC_CARRAY:
    print_proxy( "NdrConformantArrayUnmarshall( &_StubMsg, (unsigned char**)&%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d], 0);\n", index );
    break;

  case RPC_FC_BOGUS_STRUCT:
    print_proxy( "NdrComplexStructUnmarshall(&_StubMsg, (unsigned char**)&%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d], 0 );\n", index );
    break;

  case RPC_FC_FP:
    {
      var_t temp;
      memset( &temp, 0, sizeof temp );
      temp.type = type->ref;
      temp.name = arg->name; /* FIXME */
#if 1
      print_proxy( "/* FIXME: %s use the right name for %s */\n", __FUNCTION__, arg->name );
#endif
      unmarshall_copy_arg( &temp );
    }
    break;

  case RPC_FC_IP:
    print_proxy( "NdrPointerUnmarshall(&_StubMsg, (unsigned char**)&%s, ", arg->name );
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d], 0);\n", index );
    break;

  default:
    print_proxy("/* FIXME: %s code for %s type %d missing */\n", __FUNCTION__, arg->name, type->type );
  }
}

static void gen_unmarshall( var_t *arg )
{
  END_OF_LIST(arg);
  while (arg) {
    if (is_attr(arg->attrs, ATTR_OUT)) 
    {
      unmarshall_copy_arg( arg );
      fprintf(proxy, "\n");
    }
    arg = PREV_LINK(arg);
  }
}

static void free_variable( var_t *arg )
{
  var_t *constraint;
  int index = 0; /* FIXME */
  type_t *type;
  expr_t *expr;

  expr = get_attrp( arg->attrs, ATTR_SIZEIS );
  if (expr)
  {
    print_proxy( "_StubMsg.MaxCount = ", arg->name );
    write_expr(proxy, expr, 0);
    fprintf(proxy, ";\n\n");
    print_proxy( "NdrClearOutParameters( &_StubMsg, ");
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d], ", index );
    fprintf(proxy, "(void*)%s );\n", arg->name );
    return;
  }

  type = get_base_type(arg);
  switch( type->type )
  {
  case RPC_FC_BYTE:
  case RPC_FC_CHAR:
  case RPC_FC_WCHAR:
  case RPC_FC_SHORT:
  case RPC_FC_USHORT:
  case RPC_FC_ENUM16:
  case RPC_FC_LONG:
  case RPC_FC_ULONG:
  case RPC_FC_ENUM32:
  case RPC_FC_STRUCT:
    break;

  case RPC_FC_FP:
  case RPC_FC_IP:
    constraint = get_attrp( arg->attrs, ATTR_IIDIS );
    if( constraint )
      print_proxy( "_StubMsg.MaxCount = (unsigned long) ( %s );\n",constraint->name);
    print_proxy( "NdrClearOutParameters( &_StubMsg, ");
    fprintf(proxy, "&__MIDL_TypeFormatString.Format[%d], ", index );
    fprintf(proxy, "(void*)%s );\n", arg->name );
    break;

  default:
    print_proxy("/* FIXME: %s code for %s type %d missing */\n", __FUNCTION__, arg->name, type->type );
  }
}

static void proxy_free_variables( var_t *arg )
{
  END_OF_LIST(arg);
  while (arg) {
    if (is_attr(arg->attrs, ATTR_OUT)) 
    {
      free_variable( arg );
      fprintf(proxy, "\n");
    }
    arg = PREV_LINK(arg);
  }
}

static void gen_proxy(type_t *iface, func_t *cur, int idx)
{
  var_t *def = cur->def;
  int has_ret = !is_void(def->type, def);

  indent = 0;
  write_type(proxy, def->type, def, def->tname);
  print_proxy( " STDMETHODCALLTYPE %s_", iface->name);
  write_name(proxy, def);
  print_proxy( "_Proxy(\n");
  write_args(proxy, cur->args, iface->name, 1, TRUE);
  print_proxy( ")\n");
  print_proxy( "{\n");
  indent ++;
  /* local variables */
  if (has_ret) {
    print_proxy( "" );
    write_type(proxy, def->type, def, def->tname);
    print_proxy( " _RetVal;\n");
  }
  print_proxy( "RPC_MESSAGE _Msg;\n" );
  print_proxy( "MIDL_STUB_MESSAGE _StubMsg;\n" );
  print_proxy( "\n");

  /* FIXME: trace */
  clear_output_vars( cur->args );

  print_proxy( "RpcTryExcept\n" );
  print_proxy( "{\n" );
  indent++;
  print_proxy( "NdrProxyInitialize(This, &_Msg, &_StubMsg, &Object_StubDesc, %d);\n", idx);
  proxy_check_pointers( cur->args );

  print_proxy( "RpcTryFinally\n" );
  print_proxy( "{\n" );
  indent++;

  gen_marshall( cur->args );

  print_proxy( "NdrProxySendReceive(This, &_StubMsg);\n" );
  fprintf(proxy, "\n");
  print_proxy("if ((_Msg.DataRepresentation&0xffff) != NDR_LOCAL_DATA_REPRESENTATION)\n");
  indent++;
  print_proxy("NdrConvert( &_StubMsg, &__MIDL_ProcFormatString.Format[0]);\n" );
  indent--;
  fprintf(proxy, "\n");

  gen_unmarshall( cur->args );
  if (has_ret) {
    /* 
     * FIXME: We only need to round the buffer up if it could be unaligned...
     *    We should calculate how much buffer we used and output the following
     *    line only if necessary.
     */
    print_proxy( "_StubMsg.Buffer = (unsigned char *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);\n");

    print_proxy( "_RetVal = *(" );
    write_type(proxy, def->type, def, def->tname);
    fprintf(proxy, " *)_StubMsg.Buffer;\n");
    print_proxy("_StubMsg.Buffer += sizeof(");
    write_type(proxy, def->type, def, def->tname);
    fprintf(proxy, ");\n");
  }

  indent--;
  print_proxy( "}\n");
  print_proxy( "RpcFinally\n" );
  print_proxy( "{\n" );
  indent++;
  print_proxy( "NdrProxyFreeBuffer(This, &_StubMsg);\n" );
  indent--;
  print_proxy( "}\n");
  print_proxy( "RpcEndFinally\n" );
  indent--;
  print_proxy( "}\n" );
  print_proxy( "RpcExcept(_StubMsg.dwStubPhase != PROXY_SENDRECEIVE)\n" );
  print_proxy( "{\n" );
  if (has_ret) {
    indent++;
    proxy_free_variables( cur->args );
    print_proxy( "_RetVal = NdrProxyErrorHandler(RpcExceptionCode());\n" );
    indent--;
  }
  print_proxy( "}\n" );
  print_proxy( "RpcEndExcept\n" );

  if (has_ret) {
    print_proxy( "return _RetVal;\n" );
  }
  indent--;
  print_proxy( "}\n");
  print_proxy( "\n");
}

static void stub_write_locals( var_t *arg )
{
  int n = 0;
  END_OF_LIST(arg);
  while (arg) {
    int outptr = is_attr(arg->attrs, ATTR_OUT)
                 && ! is_attr(arg->attrs, ATTR_IN);

    /* create a temporary variable to store the output */
    if (outptr) {
      var_t temp;
      memset( &temp, 0, sizeof temp );
      temp.ptr_level = arg->ptr_level - 1; /* dereference once */
      print_proxy("");
      write_type(proxy, arg->type, &temp, arg->tname);
      fprintf(proxy, " _M%d;\n",n++);
    }
    print_proxy("");
    write_type(proxy, arg->type, arg, arg->tname);
    fprintf(proxy, " ");
    write_name(proxy, arg);
    fprintf(proxy, ";\n");
    arg = PREV_LINK(arg);
  }
}

static void stub_unmarshall( var_t *arg )
{
  int n = 0;
  END_OF_LIST(arg);
  while (arg) {
    if (is_attr(arg->attrs, ATTR_IN))
    {
      unmarshall_copy_arg( arg );
      fprintf(proxy,"\n");
    }
    else if (is_attr(arg->attrs, ATTR_OUT)) {
      type_t *type = get_base_type(arg);
      switch( type->type )
      {
      case RPC_FC_STRUCT:
        print_proxy("MIDL_memset(");
        write_name(proxy, arg);
        fprintf(proxy,", 0, sizeof(");
        write_type(proxy, arg->type, arg, arg->tname);
        fprintf(proxy,"));\n");
        break;
      default:
        print_proxy("");
        write_name(proxy, arg);
        fprintf(proxy," = &_M%d;\n", n);
        print_proxy("MIDL_memset(&_M%d, 0, sizeof _M%d);\n", n, n);
        ++n;
        break;
      }
    }
    arg = PREV_LINK(arg);
  }
}

static void stub_gen_marshall_size( var_t *arg )
{
  print_proxy( "_StubMsg.BufferLength = 0U;\n" );

  END_OF_LIST(arg);
  while (arg) {
    if (is_attr(arg->attrs, ATTR_OUT))
      marshall_size_arg( arg );
    arg = PREV_LINK(arg);
  }
}

static void stub_gen_marshall_copydata( var_t *arg )
{
  END_OF_LIST(arg);
  while (arg) {
    if (is_attr(arg->attrs, ATTR_OUT))
      marshall_copy_arg( arg );
    arg = PREV_LINK(arg);
  }
}

static void stub_genmarshall( var_t *args )
{
  /* FIXME: size buffer */
  stub_gen_marshall_size( args );

  print_proxy("NdrStubGetBuffer(This, pRpcChannelBuffer, &_StubMsg);\n");

  stub_gen_marshall_copydata( args );
}

static void gen_stub(type_t *iface, func_t *cur, const char *cas)
{
  var_t *def = cur->def;
  var_t *arg;
  int has_ret = !is_void(def->type, def);

  indent = 0;
  print_proxy( "void __RPC_STUB %s_", iface->name);
  write_name(proxy, def);
  print_proxy( "_Stub(\n");
  indent++;
  print_proxy( "IRpcStubBuffer* This,\n");
  print_proxy( "IRpcChannelBuffer* pRpcChannelBuffer,\n");
  print_proxy( "PRPC_MESSAGE _Msg,\n");
  print_proxy( "DWORD* _pdwStubPhase)\n");
  indent--;
  print_proxy( "{\n");
  indent++;
  /* local variables */
  if (has_ret) {
    print_proxy("");
    write_type(proxy, def->type, def, def->tname);
    fprintf(proxy, " _RetVal;\n");
  }
  print_proxy("%s * _This = (%s*)((CStdStubBuffer*)This)->pvServerObject;\n", iface->name, iface->name);
  print_proxy("MIDL_STUB_MESSAGE _StubMsg;\n");
  stub_write_locals( cur->args );
  fprintf(proxy, "\n");

  /* FIXME: trace */

  print_proxy("NdrStubInitialize(_Msg, &_StubMsg, &Object_StubDesc, pRpcChannelBuffer);\n");
  fprintf(proxy, "\n");

  print_proxy("RpcTryFinally\n");
  print_proxy("{\n");
  indent++;
  print_proxy("if ((_Msg->DataRepresentation&0xffff) != NDR_LOCAL_DATA_REPRESENTATION)\n");
  indent++;
  print_proxy("NdrConvert( &_StubMsg, &__MIDL_ProcFormatString.Format[0]);\n" );
  indent--;
  fprintf(proxy, "\n");

  stub_unmarshall( cur->args );
  fprintf(proxy, "\n");

  print_proxy("*_pdwStubPhase = STUB_CALL_SERVER;\n");
  fprintf(proxy, "\n");
  print_proxy("");
  if (has_ret) fprintf(proxy, "_RetVal = ");
  fprintf(proxy, "%s_", iface->name);
  if (cas) fprintf(proxy, "%s_Stub", cas);
  else write_name(proxy, def);
  fprintf(proxy, "(_This");
  arg = cur->args;
  if (arg) {
    END_OF_LIST(arg);
    while (arg) {
      fprintf(proxy, ", ");
      write_name(proxy, arg);
      arg = PREV_LINK(arg);
    }
  }
  fprintf(proxy, ");\n");
  fprintf(proxy, "\n");
  print_proxy("*_pdwStubPhase = STUB_MARSHAL;\n");
  fprintf(proxy, "\n");

  stub_genmarshall( cur->args );
  fprintf(proxy, "\n");

  if (has_ret) {
    /* 
     * FIXME: We only need to round the buffer up if it could be unaligned...
     *    We should calculate how much buffer we used and output the following
     *    line only if necessary.
     */
    print_proxy( "_StubMsg.Buffer = (unsigned char *)(((long)_StubMsg.Buffer + 3) & ~ 0x3);\n");

    print_proxy( "*(" );
    write_type(proxy, def->type, def, def->tname);
    fprintf(proxy, " *)_StubMsg.Buffer = _RetVal;\n");
    print_proxy("_StubMsg.Buffer += sizeof(");
    write_type(proxy, def->type, def, def->tname);
    fprintf(proxy, ");\n");
  }

  indent--;
  print_proxy("}\n");
  print_proxy("RpcFinally\n");
  print_proxy("{\n");
  print_proxy("}\n");
  print_proxy("RpcEndFinally\n");

  print_proxy("_Msg->BufferLength = _StubMsg.Buffer - (unsigned char *)_Msg->Buffer;\n");
  indent--;

  print_proxy("}\n");
  print_proxy("\n");
}

static int write_proxy_methods(type_t *iface)
{
  func_t *cur = iface->funcs;
  int i = 0;

  END_OF_LIST(cur);

  if (iface->ref) i = write_proxy_methods(iface->ref);
  while (cur) {
    var_t *def = cur->def;
    if (!is_callas(def->attrs)) {
      if (i) fprintf(proxy, ",\n");
      print_proxy( "%s_", iface->name);
      write_name(proxy, def);
      fprintf(proxy, "_Proxy");
      i++;
    }
    cur = PREV_LINK(cur);
  }
  return i;
}

static int write_stub_methods(type_t *iface)
{
  func_t *cur = iface->funcs;
  int i = 0;

  END_OF_LIST(cur);

  if (iface->ref) i = write_stub_methods(iface->ref);
  else return i; /* skip IUnknown */
  while (cur) {
    var_t *def = cur->def;
    if (!is_local(def->attrs)) {
      if (i) fprintf(proxy,",\n");
      print_proxy( "%s_", iface->name);
      write_name(proxy, def);
      fprintf(proxy, "_Stub");
      i++;
    }
    cur = PREV_LINK(cur);
  }
  return i;
}

static void write_proxy(type_t *iface)
{
  int midx = -1, stubs;
  func_t *cur = iface->funcs;

  if (!cur) return;

  END_OF_LIST(cur);

  /* FIXME: check for [oleautomation], shouldn't generate proxies/stubs if specified */

  fprintf(proxy, "/*****************************************************************************\n");
  fprintf(proxy, " * %s interface\n", iface->name);
  fprintf(proxy, " */\n");
  while (cur) {
    const var_t *def = cur->def;
    if (!is_local(def->attrs)) {
      const var_t *cas = is_callas(def->attrs);
      const char *cname = cas ? cas->name : NULL;
      int idx = cur->idx;
      if (cname) {
        const func_t *m = iface->funcs;
        while (m && strcmp(get_name(m->def), cname))
          m = NEXT_LINK(m);
        idx = m->idx;
      }
      gen_proxy(iface, cur, idx);
      gen_stub(iface, cur, cname);
      if (midx == -1) midx = idx;
      else if (midx != idx) yyerror("method index mismatch in write_proxy");
      midx++;
    }
    cur = PREV_LINK(cur);
  }

  /* proxy vtable */
  print_proxy( "const CINTERFACE_PROXY_VTABLE(%d) _%sProxyVtbl =\n", midx, iface->name);
  print_proxy( "{\n");
  indent++;
  print_proxy( "{\n", iface->name);
  indent++;
  print_proxy( "&IID_%s,\n", iface->name);
  indent--;
  print_proxy( "},\n");
  print_proxy( "{\n");
  indent++;
  write_proxy_methods(iface);
  fprintf(proxy, "\n");
  indent--;
  print_proxy( "}\n");
  indent--;
  print_proxy( "};\n");
  fprintf(proxy, "\n\n");

  /* stub vtable */
  print_proxy( "static const PRPC_STUB_FUNCTION %s_table[] =\n", iface->name);
  print_proxy( "{\n");
  indent++;
  stubs = write_stub_methods(iface);
  fprintf(proxy, "\n");
  indent--;
  fprintf(proxy, "};\n");
  print_proxy( "\n");
  print_proxy( "const CInterfaceStubVtbl _%sStubVtbl =\n", iface->name);
  print_proxy( "{\n");
  indent++;
  print_proxy( "{\n");
  indent++;
  print_proxy( "&IID_%s,\n", iface->name);
  print_proxy( "0,\n");
  print_proxy( "%d,\n", stubs+3);
  print_proxy( "&%s_table[-3],\n", iface->name);
  indent--;
  print_proxy( "},\n", iface->name);
  print_proxy( "{\n");
  indent++;
  print_proxy( "CStdStubBuffer_METHODS\n");
  indent--;
  print_proxy( "}\n");
  indent--;
  print_proxy( "};\n");
  print_proxy( "\n");
}

void write_proxies(ifref_t *ifaces)
{
  ifref_t *lcur = ifaces;
  ifref_t *cur;
  char *file_id = proxy_token;
  int c;

  if (!do_proxies) return;
  if (!lcur) return;
  END_OF_LIST(lcur);

  init_proxy(ifaces);
  if(!proxy) return;

  cur = lcur;
  while (cur) {
    if (is_object(cur->iface->attrs) && !is_local(cur->iface->attrs))
      write_proxy(cur->iface);
    cur = PREV_LINK(cur);
  }

  if (!proxy) return;

  write_stubdesc();

  print_proxy( "#if !defined(__RPC_WIN32__)\n");
  print_proxy( "#error Currently only Wine and WIN32 are supported.\n");
  print_proxy( "#endif\n");
  print_proxy( "\n");
  write_procformatstring(proxy, ifaces, 1);
  write_typeformatstring(proxy, ifaces, 1);

  fprintf(proxy, "const CInterfaceProxyVtbl* _%s_ProxyVtblList[] =\n", file_id);
  fprintf(proxy, "{\n");
  cur = lcur;
  while (cur) {
    if(cur->iface->ref && cur->iface->funcs &&
       is_object(cur->iface->attrs) && !is_local(cur->iface->attrs))
      fprintf(proxy, "    (CInterfaceProxyVtbl*)&_%sProxyVtbl,\n", cur->iface->name);
    cur = PREV_LINK(cur);
  }
  fprintf(proxy, "    0\n");
  fprintf(proxy, "};\n");
  fprintf(proxy, "\n");

  fprintf(proxy, "const CInterfaceStubVtbl* _%s_StubVtblList[] =\n", file_id);
  fprintf(proxy, "{\n");
  cur = lcur;
  while (cur) {
    if(cur->iface->ref && cur->iface->funcs &&
       is_object(cur->iface->attrs) && !is_local(cur->iface->attrs))
      fprintf(proxy, "    (CInterfaceStubVtbl*)&_%sStubVtbl,\n", cur->iface->name);
    cur = PREV_LINK(cur);
  }
  fprintf(proxy, "    0\n");
  fprintf(proxy, "};\n");
  fprintf(proxy, "\n");

  fprintf(proxy, "PCInterfaceName const _%s_InterfaceNamesList[] =\n", file_id);
  fprintf(proxy, "{\n");
  cur = lcur;
  while (cur) {
    if(cur->iface->ref && cur->iface->funcs &&
       is_object(cur->iface->attrs) && !is_local(cur->iface->attrs))
      fprintf(proxy, "    \"%s\",\n", cur->iface->name);
    cur = PREV_LINK(cur);
  }
  fprintf(proxy, "    0\n");
  fprintf(proxy, "};\n");
  fprintf(proxy, "\n");

  fprintf(proxy, "#define _%s_CHECK_IID(n) IID_GENERIC_CHECK_IID(_%s, pIID, n)\n", file_id, file_id);
  fprintf(proxy, "\n");
  fprintf(proxy, "int __stdcall _%s_IID_Lookup(const IID* pIID, int* pIndex)\n", file_id);
  fprintf(proxy, "{\n");
  cur = lcur;
  c = 0;
  while (cur) {
    if(cur->iface->ref)
    {
      fprintf(proxy, "    if (!_%s_CHECK_IID(%d))\n", file_id, c);
      fprintf(proxy, "    {\n");
      fprintf(proxy, "        *pIndex = %d;\n", c);
      fprintf(proxy, "        return 1;\n");
      fprintf(proxy, "    }\n");
      c++;
    }
    cur = PREV_LINK(cur);
  }
  fprintf(proxy, "    return 0;\n");
  fprintf(proxy, "}\n");
  fprintf(proxy, "\n");

  fprintf(proxy, "const ExtendedProxyFileInfo %s_ProxyFileInfo =\n", file_id);
  fprintf(proxy, "{\n");
  fprintf(proxy, "    (PCInterfaceProxyVtblList*)&_%s_ProxyVtblList,\n", file_id);
  fprintf(proxy, "    (PCInterfaceStubVtblList*)&_%s_StubVtblList,\n", file_id);
  fprintf(proxy, "    (const PCInterfaceName*)&_%s_InterfaceNamesList,\n", file_id);
  fprintf(proxy, "    0,\n");
  fprintf(proxy, "    &_%s_IID_Lookup,\n", file_id);
  fprintf(proxy, "    %d,\n", c);
  fprintf(proxy, "    1,\n");
  fprintf(proxy, "    0,\n");
  fprintf(proxy, "    0,\n");
  fprintf(proxy, "    0,\n");
  fprintf(proxy, "    0\n");
  fprintf(proxy, "};\n");

  fclose(proxy);
}
