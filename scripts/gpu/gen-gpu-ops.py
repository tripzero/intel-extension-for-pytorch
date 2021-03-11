from __future__ import print_function

import argparse
import collections
import lark
import os
import re
import string
import sys

from collections import namedtuple
if sys.version_info < (3, 3):
    from collections import Mapping
else:
    from collections.abc import Mapping

def namedtuple_with_defaults(typename, field_names, default_values=()):
  ntuple = namedtuple(typename, field_names)
  ntuple.__new__.__defaults__ = (None,) * len(ntuple._fields)
  if isinstance(default_values, Mapping):
    prototype = ntuple(**default_values)
  else:
    prototype = ntuple(*default_values)
  ntuple.__new__.__defaults__ = tuple(prototype)
  return ntuple


class ArgTemplate(string.Template):
  idpattern = r'[a-z0-9_]+'


FuncDef = namedtuple_with_defaults('FuncDef', 'cpp_sig, aten_sig')

FuncGen = namedtuple_with_defaults(
    'FuncGen',
    'tree, xtree, rwxtree, func, xfunc, code, sig, rwsig, cppsig, funsig, mapsig, aten_sig'
)

FuncOpts = namedtuple_with_defaults(
    'FuncOpts',
    'ref_param, device_param, wparams, outfn_template, outfn_name, shape_check_indices'
)

_GRAMMAR = r"""
    start: type fnname "(" params ")"
    type: CONST? core_type refspec?
    fnname: CNAME
    refspec: REF
           | PTR
    core_type: template
        | TNAME
    template: TNAME "<" typelist ">"
    typelist: type
            | type "," typelist
    REF: "&"
    PTR: "*"
    CONST: "const"
    TNAME: /[a-zA-Z0-9_:]+/
    HEXNUMBER: /0x[0-9a-fA-F]+/
    params: param
          | param "," params
    param: type param_name param_defval?
    param_name: CNAME

    param_defval: "=" init_value
    init_value: "true"
              | "false"
              | "{}"
              | NUMBER
              | SIGNED_NUMBER
              | HEXNUMBER
              | ESCAPED_STRING

    %import common.CNAME -> CNAME
    %import common.NUMBER -> NUMBER
    %import common.SIGNED_NUMBER -> SIGNED_NUMBER
    %import common.ESCAPED_STRING -> ESCAPED_STRING
    %import common.WS
    %ignore WS
    """

_PARSER = lark.Lark(_GRAMMAR, parser='lalr', propagate_positions=True)

_XPARSER = lark.Lark(
    _GRAMMAR, parser='lalr', propagate_positions=True, keep_all_tokens=True)

_FN_BLACKLIST = set([
#    'numel',
#    'ones',
#    'ones_like',
#    'result_type',
#    'zero_',
#    'zeros',
#    'zeros_like',
])

_FN_BLKFMT_SUPPORTED = set([
    'convolution_overrideable',
    'relu',
    'relu_',
    'native_batch_norm',
    'add_',
    'add',
    'add_out',
    'avg_pool2d',
    'avg_pool2d_out',
    '_adaptive_avg_pool2d',
    'max_pool2d_with_indices',
    'max_pool2d_with_indices_out',
    'quantize_per_tensor',
    'quantize_per_channel',
    'dequantize',
])

_FN_BLACKLIST_REGEX = [
    # ATEN functions
    r'[^(]*cudnn',
    # IPEX functions
]

_FN_OUT = {
    'add_out':
        FuncOpts(),
    'arange_out(Tensor, Scalar, Scalar, Scalar) -> Tensor':
        FuncOpts(
            outfn_template=ArgTemplate(
                'AtenIpexType::arange($1, $2, $3, $0.options())')),
    'bitwise_not_out':
        FuncOpts(),
    'clamp_out':
        FuncOpts(),
    'div_out':
        FuncOpts(),
    'gather_out':
        FuncOpts(),
    'kthvalue_out':
        FuncOpts(),
    'index_select_out':
        FuncOpts(),
    'log_out':
        FuncOpts(),
    'topk_out':
        FuncOpts(),
}

# List of tuples with the regex match first, and the corresponding FuncOpts()
# second.
_FN_OUT_REGEX = []

_FN_REMAP = {
    '_th_eq(Tensor, Scalar) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::eq'),
    '_th_eq(Tensor, Tensor) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::eq'),
    '_th_ge(Tensor, Scalar) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::ge'),
    '_th_ge(Tensor, Tensor) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::ge'),
    '_th_gt(Tensor, Scalar) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::gt'),
    '_th_gt(Tensor, Tensor) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::gt'),
    '_th_le(Tensor, Scalar) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::le'),
    '_th_le(Tensor, Tensor) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::le'),
    '_th_lt(Tensor, Scalar) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::lt'),
    '_th_lt(Tensor, Tensor) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::lt'),
    '_th_ne(Tensor, Scalar) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::ne'),
    '_th_ne(Tensor, Tensor) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::ne'),
    's__th_and(Tensor, Tensor) -> Tensor':
        FuncOpts(
            outfn_name='AtenIpexType::__and__', shape_check_indices=((0, 1),)),
    's__th_or(Tensor, Tensor) -> Tensor':
        FuncOpts(
            outfn_name='AtenIpexType::__or__', shape_check_indices=((0, 1),)),
    's__th_xor(Tensor, Tensor) -> Tensor':
        FuncOpts(
            outfn_name='AtenIpexType::__xor__', shape_check_indices=((0, 1),)),
    '_s_where(Tensor, Tensor, Tensor) -> Tensor':
        FuncOpts(
            outfn_name='AtenIpexType::where',
            shape_check_indices=(
                (0, 1),
                (0, 2),
            )),
    's__th_eq(Tensor, Tensor) -> Tensor':
        FuncOpts(outfn_name='AtenIpexType::eq', shape_check_indices=((0, 1),)),
}

_TYPE_NSMAP = {
    'Tensor': 'at::Tensor',
    'TensorList': 'at::TensorList',
    'Scalar': 'at::Scalar',
    'Storage': 'at::Storage',
    'IntList': 'at::IntList',
    'IntArrayRef': 'at::IntArrayRef',
    'Generator': 'at::Generator',
    'ScalarType': 'at::ScalarType',
    'TensorOptions': 'at::TensorOptions',
    'SparseTensorRef': 'at::SparseTensorRef',
    'Device': 'c10::Device',
    'optional': 'c10::optional',
    'MemoryFormat': 'at::MemoryFormat',
    'QScheme': 'at::QScheme',
    'ConstQuantizerPtr': 'at::ConstQuantizerPtr',
    'Dimname': 'at::Dimname',  # namedtensor-only
    'DimnameList': 'at::DimnameList',  # namedtensor-only
}

_H_HEADER = """// Autogenerated file by {gen}. Do not edit directly!

#include <ATen/Tensor.h>

namespace at {{

class AtenIpexTypeDefault {{
 public:
{hfuncs}
}};

void RegisterAtenTypeFunctions();

}}  // namespace at
"""

_H_DPCPP_HEADER = """// Autogenerated file by {gen}. Do not edit directly!

#include <ATen/Tensor.h>
#include <ATen/ipex_type_dpcpp_customized.h>

namespace at {{

namespace AtenIpexTypeDPCPP {{
{hfuncs}

}} // namespace AtenIpexTypeDPCPP
}} // namespace at
"""

_CPP_HEADER = """// Autogenerated file by {gen}. Do not edit directly!
#include <ATen/aten_ipex_type_default.h>
#include <ATen/aten_ipex_type_dpcpp.h>

#include <ATen/Context.h>
#include <ATen/core/op_registration/op_registration.h>

namespace at {{

{funcs}

{regs}
}}  // namespace at
"""

_IPEX_FUNCTIONS = {}

_CTOR_FUNCTIONS = {
    'empty': '.device(at::DeviceType::CPU)',
    'linspace': '.device(at::DeviceType::CPU)',
    'logspace': '.device(at::DeviceType::CPU)',
    'rand': '.device(at::DeviceType::CPU)',
    'rand_like': '.device(at::DeviceType::CPU)',
    'randn': '.device(at::DeviceType::CPU)',
    'randn_like': '.device(at::DeviceType::CPU)',
    'randint': '.device(at::DeviceType::CPU)',
    'randint_like': '.device(at::DeviceType::CPU)',
    'randperm': '.device(at::DeviceType::CPU)',
    'scalar_tensor': '.device(at::DeviceType::CPU)',
}

_FUNCTION_OPTIONS = {
    'slice(Tensor, int64_t, int64_t, int64_t, int64_t) -> Tensor':
        FuncOpts(wparams=['self']),
}

_RESULT_NAME = 'x_result'


class Context(object):

  def __init__(self, functions):
    with open(functions, 'r') as ff:
      self.functions_data = ff.read()

  def get_function(self, name):
    if self.functions_data.find(' {}('.format(name)) >= 0:
      return 'at::{}'.format(name)


class StringEmit(object):

  def __init__(self, sref):
    self.sref = sref
    self.sval = ''
    self.pos = -1

  def __repr__(self):
    return self.sval

  def advance(self, t):
    start = t.column - 1
    end = t.end_column - 1
    pos = self.pos if self.pos >= 0 else start
    if start > pos:
      self.sval += self.sref[pos:start]
    self.sval += t.value
    self.pos = end

  def skip(self, t):
    self.pos = last_match(t) if self.pos >= 0 else -1

  def append(self, s):
    self.sval += s
    self.pos = -1


class TensorFetcher(object):

  def __init__(self, var_name):
    self.var_name = var_name
    self.tvar_name = '{}_tensors'.format(self.var_name)
    self.tensors = []
    self.writeable = []

  def add(self, name, writeable):
    if writeable:
      self.writeable.append(len(self.tensors))
    self.tensors.append(name)
    return '{}[{}]'.format(self.var_name, len(self.tensors) - 1)

  def generate_fetches(self):
    code = ''
    code += '  std::vector<at::Tensor> {} = {{{}}};\n'.format(
        self.tvar_name, ', '.join(self.tensors))
    code += ('  auto {} = bridge::IpexCreateTensorList({});\n').format(
        self.var_name, self.tvar_name)
    return code

  def generate_updates(self):
    code = ''
    if self.writeable:
      ivar_name = '{}_update_indices'.format(self.var_name)
      code += '  std::vector<size_t> {} = {{{}}};\n'.format(
          ivar_name, ', '.join(str(x) for x in self.writeable))
      code += '  bridge::IpexUpdateTensors({}, {}, {});\n'.format(
          self.tvar_name, self.var_name, ivar_name)
    return code


def list_get(l, n):
  return l[n] if n < len(l) else None


def is_blacklisted_fn(fname, mapsig):
  if fname in _FN_BLACKLIST or mapsig in _FN_BLACKLIST:
    return True
  for frx in _FN_BLACKLIST_REGEX:
    if re.match(frx, fname) or re.match(frx, mapsig):
      return True
  return False


def is_blkfmt_supported(fname):
  if fname in _FN_BLKFMT_SUPPORTED:
    return True
  else:
    return False


def get_outfn_options(fname, mapsig):
  for name in [fname, mapsig]:
    fnopts = _FN_OUT.get(name, None)
    if fnopts is not None:
      return fnopts
  for frx, fnopts in _FN_OUT_REGEX:
    if re.match(frx, fname) or re.match(frx, mapsig):
      return fnopts


def get_remapfn_options(fname, mapsig):
  for name in [fname, mapsig]:
    fnopts = _FN_REMAP.get(name, None)
    if fnopts is not None:
      return fnopts


def is_write_param(fnopts, pname, defval):
  if fnopts and fnopts.wparams:
    if pname in fnopts.wparams:
      return True
  return defval


def first_match(t):
  if isinstance(t, lark.lexer.Token):
    return t.column - 1
  assert isinstance(t, lark.tree.Tree)
  return first_match(t.children[0])


def last_match(t):
  if isinstance(t, lark.lexer.Token):
    return t.end_column - 1
  assert isinstance(t, lark.tree.Tree)
  return last_match(t.children[-1])


def for_every_token(t, fn):
  if isinstance(t, lark.lexer.Token):
    fn(t)
  else:
    assert isinstance(t, lark.tree.Tree)
    for c in t.children:
      for_every_token(c, fn)


def emit_string(t, emit, emit_fn):
  status = emit_fn(t)
  if status > 0:

    def do_emit(tok):
      emit.advance(tok)

    for_every_token(t, do_emit)
  elif status == 0:
    if isinstance(t, lark.lexer.Token):
      emit.advance(t)
    else:
      assert isinstance(t, lark.tree.Tree)
      for c in t.children:
        emit_string(c, emit, emit_fn)
  else:
    emit.skip(t)


def typed_child(t, n, ttype):
  assert isinstance(t, lark.tree.Tree)
  assert n < len(t.children)
  c = t.children[n]
  assert isinstance(c, lark.tree.Tree)
  assert c.data == ttype, t.pretty()
  return c


def rewrite_sig(tree, orig_sig, emit_fn=lambda x: 0):
  emit = StringEmit(orig_sig)
  emit_string(tree, emit, emit_fn)
  return str(emit)


def rewrite_signature(sig, tmap):

  def rewrite(t):
    if t.type == 'TNAME':
      new_type = tmap.get(t.value, None)
      if new_type is not None:
        t.value = new_type

  def emit_fn(t):
    if isinstance(t, lark.lexer.Token):
      return 0
    return -1 if t.data == 'param_defval' else 0

  xtree = _XPARSER.parse(sig)
  for_every_token(xtree, rewrite)
  return rewrite_sig(xtree, sig, emit_fn=emit_fn)


def create_stdfunc_sig(tree, orig_sig):

  def emit_fn(t):
    if isinstance(t, lark.lexer.Token):
      return 0
    return -1 if t.data == 'param_name' else 0

  emit = StringEmit(orig_sig)
  # Emit full function return type.
  emit_string(typed_child(tree, 0, 'type'), emit, emit_fn)
  emit.append('(')
  # Emit parameter list w/out parameter names.
  emit_string(typed_child(tree, 3, 'params'), emit, emit_fn)
  emit.append(')')
  return str(emit)


def create_map_sig(tree, orig_sig):

  def emit_fn(t):
    if isinstance(t, lark.lexer.Token):
      return -1 if t.type in ['CONST', 'REF', 'PTR'] else 0
    return -1 if t.data in ['param_name', 'param_defval'] else 0

  emit = StringEmit(orig_sig)
  # Emit full function return type.
  emit_string(typed_child(tree, 1, 'fnname'), emit, emit_fn)
  emit.append('(')
  # Emit parameter list w/out parameter names.
  emit_string(typed_child(tree, 3, 'params'), emit, emit_fn)
  emit.append(') -> ')
  emit_string(typed_child(tree, 0, 'type'), emit, emit_fn)
  return str(emit)


def type_core(t):
  assert isinstance(t, lark.tree.Tree)
  for c in t.children:
    if isinstance(c, lark.tree.Tree) and c.data == 'core_type':
      c = c.children[0]
      if isinstance(c, lark.lexer.Token):
        return c.value
      assert isinstance(c, lark.tree.Tree) and c.data == 'template'
      return c.children[0].value
  raise RuntimeError('Not a type tree: {}'.format(t))


def type_is_const(t):
  assert isinstance(t, lark.tree.Tree)
  c = t.children[0]
  return isinstance(c, lark.lexer.Token) and c.value == 'const'


def type_is_refptr(t, kind):
  assert isinstance(t, lark.tree.Tree)
  c = t.children[-1]
  if not isinstance(c, lark.tree.Tree) or c.data != 'refspec':
    return False
  c = c.children[0]
  return isinstance(c, lark.lexer.Token) and c.value == kind


def extract_list(t, l):
  assert isinstance(t, lark.tree.Tree)
  l.append(t.children[0])
  if len(t.children) == 2:
    c = t.children[1]
    if isinstance(c, lark.tree.Tree) and c.data == t.data:
      extract_list(c, l)
  return l


def tuple_type_list(t):
  assert isinstance(t, lark.tree.Tree)
  c = t.children[0]
  assert isinstance(c, lark.tree.Tree) and c.data == 'core_type'
  c = c.children[0]
  assert isinstance(c, lark.tree.Tree) and c.data == 'template'
  types = []
  return extract_list(c.children[1], types)


def get_function_name(t):
  assert isinstance(t, lark.tree.Tree)
  fname = t.children[1]
  assert isinstance(fname, lark.tree.Tree)
  assert fname.data == 'fnname'
  return fname.children[0].value


def get_function_signature(t, orig_sig, namefn):
  emit = StringEmit(orig_sig)
  # Emit full function return type.
  emit_string(typed_child(t, 0, 'type'), emit, lambda t: 0)
  fnname = typed_child(t, 1, 'fnname').children[0]
  xfname = namefn(fnname.value)
  emit.append(' {}('.format(xfname))
  # Emit parameter list w/out parameter names.
  emit_string(typed_child(t, 3, 'params'), emit, lambda t: 0)
  emit.append(')')
  return str(emit), fnname.value, xfname


def get_parameters(t):
  assert isinstance(t, lark.tree.Tree)
  c = t.children[2]
  assert isinstance(c, lark.tree.Tree)
  assert c.data == 'params'
  params = []
  extract_list(c, params)
  return params


def param_name(t):
  assert isinstance(t, lark.tree.Tree)
  c = t.children[1]
  assert isinstance(c, lark.tree.Tree)
  assert c.data == 'param_name'
  token = c.children[0]
  assert isinstance(token, lark.lexer.Token)
  return token.value


def param_type(t):
  assert isinstance(t, lark.tree.Tree)
  c = t.children[0]
  assert isinstance(c, lark.tree.Tree)
  return c


def get_optional(fnopts, name, defval=None):
  if fnopts is None or not hasattr(fnopts, name):
    return defval
  return getattr(fnopts, name, defval) or defval


def get_return_value(rtype, rname, param, var, ref_param, fnopts):
  crtype = type_core(rtype)
  if type_is_const(rtype) or type_is_refptr(rtype, '&'):
    # If the return type is a const or a reference, return the matching
    # parameter. In these cases we operated on IPEX tensors data (the ATEN one),
    # but the returned references are the input parameters.
    assert param
    return param_name(param)
  elif crtype != 'Tensor':
    return rname
  else:
    # If instead the return type is a value Tensor, we create a new one by
    # wrapping the proper local variable which has been created by calling
    # into the CPU tensor implementation.
    return 'bridge::CreateIpexTensor({}, bridge::GetIpexDevice({}))'.format(
        rname, get_optional(fnopts, 'device_param', param_name(ref_param)))


def get_reference_param(params, fnopts=None):
  # The reference parameter is the Tensor object which we use to extract the
  # result Tensor device, if any.
  ref_param = None
  other = None
  for p in params:
    ptype = param_type(p)
    cptype = type_core(ptype)
    pname = param_name(p)
    if get_optional(fnopts, 'ref_param') == pname:
      return p
    if not other and (cptype == 'TensorOptions' or cptype == 'TensorList'):
      other = p
    if cptype != 'Tensor':
      continue
    if not ref_param and (pname == 'self' or type_is_const(ptype)):
      ref_param = p
    other = p
  return ref_param or other


def get_tuple_return(rtype, rtype_str, rname, params, param_vars, ref_param,
                     fnopts):
  types = tuple_type_list(rtype)
  retstr = '{}('.format(rtype_str)
  for i, ttype in enumerate(types):
    if i > 0:
      retstr += ', '
    tuple_var = 'std::get<{}>({})'.format(i, rname)
    retstr += get_return_value(ttype, tuple_var, list_get(params, i),
                               list_get(param_vars, i), ref_param, fnopts)
  return retstr + ')'


def get_return_type_str(t, orig_sig):
  assert isinstance(t, lark.tree.Tree)
  fname = t.children[1]
  assert isinstance(fname, lark.tree.Tree)
  assert fname.data == 'fnname'
  token = fname.children[0]
  assert isinstance(token, lark.lexer.Token)
  return orig_sig[0:token.column - 2]


def generate_entry_debug_code(t, fname, params, fname_ns='aten'):
  # Emits debug code for a given intercepted ATEN type function.
  code = ''
  code += '  TORCH_WARN("{}::{}"'.format(fname_ns, fname)
  for p in params:
    ptype = param_type(p)
    cptype = type_core(ptype)
    pname = param_name(p)
    if cptype == 'Tensor':
      code += ', " {}=", {}.toString'.format(pname, pname)
  code += ');\n'
  return code


def generate_exit_debug_code(t, fname, rname, params, param_vars):
  code = ''
  return code


def generate_return_stmt(t, rtype_str, fname, rname, params, param_vars,
                         ref_param, fnopts):
  assert isinstance(t, lark.tree.Tree)
  rtype = t.children[0]
  ctype = type_core(rtype)
  if ctype == 'std::tuple':
    retstr = get_tuple_return(rtype, rtype_str, rname, params, param_vars,
                              ref_param, fnopts)
  elif ctype == 'std::vector':
    retstr = 'bridge::CreateIpexTensors({}, bridge::GetIpexDevice({}))'.format(
        rname, get_optional(fnopts, 'device_param', param_name(ref_param)))
  elif ctype == 'Tensor':
    retstr = get_return_value(rtype, rname, params[0], param_vars[0], ref_param,
                              fnopts)
  elif ctype == 'void' and not type_is_refptr(rtype, '*'):
    return ''
  else:
    retstr = rname
  return '  return {};\n'.format(retstr)


def generate_result_assignment(t, rname):
  assert isinstance(t, lark.tree.Tree)
  rtype = t.children[0]
  ctype = type_core(rtype)
  if ctype == 'void' and not type_is_refptr(rtype, '*'):
    return ''
  return 'auto&& {} = '.format(rname)


def get_handling_function(ctx, fname, ipex_ref_param, param_vars):
  function = _IPEX_FUNCTIONS.get(fname, None) or ctx.get_function(fname)
  if function:
    code = '{}({})'.format(function, ', '.join(param_vars))
  else:
    other_params = list(param_vars)
    # other_params.remove(ipex_ref_param)
    # code = '{}.{}({})'.format(ipex_ref_param, fname, ', '.join(other_params))
    code = '{}({})'.format('AtenIpexTypeDPCPP::' + fname, ', '.join(other_params))
  return code


def rewrite_tensor_options(fname, pname):
  rw = _CTOR_FUNCTIONS.get(fname, None)
  if rw is None:
    return '', pname
  xname = 'o_{}'.format(pname)
  code = '  at::TensorOptions {} = {}{};\n'.format(xname, pname, rw)
  return code, xname


def get_param_names(params):
  param_vars = []
  for p in params:
    pname = param_name(p)
    param_vars.append(pname)
  return param_vars


def expand_fn_template(tmpl, param_vars):
  mdict = {}
  for i, pname in enumerate(param_vars):
    mdict[str(i)] = pname
  return tmpl.substitute(mdict)


def create_call(fname, param_vars):
  return '{}({})'.format(fname, ', '.join(param_vars))


def generate_shape_checks(param_vars, shape_check_indices, fname):
  code = ''
  for i, j in shape_check_indices:
    code += ('  TORCH_CHECK({}.sizes() == {}.sizes()) << "Operand shapes must be '
             'identical for {}, mismatch for arguments {} and {}";\n').format(
                 param_vars[i], param_vars[j], fname, i + 1, j + 1)
  return code


def generate_aten_remap(ctx, fname, sig, params, fnopts):
  code = '{} {{\n'.format(sig)

  param_vars = get_param_names(params)
  if fnopts.outfn_template is not None:
    fcall = expand_fn_template(fnopts.outfn_template, param_vars)
  else:
    assert fnopts.outfn_name
    fcall = create_call(fnopts.outfn_name, param_vars)

  if fnopts.shape_check_indices is not None:
    code += generate_shape_checks(param_vars, fnopts.shape_check_indices, fname)
  code += '  return {};\n'.format(fcall)
  code += '}'
  return code


def generate_outfn_result_copy(dest, src):
  return '  {}.unsafeGetTensorImpl()->shallow_copy_from({}.getIntrusivePtr());\n'.format(
      dest, src)


def generate_aten_out(ctx, tree, rwxtree, fname, sig, rwsig, params, fnopts):
  rtype = tree.children[0]
  num_outputs = None
  if type_core(rtype) == 'std::tuple':
    num_outputs = len(tuple_type_list(rtype))

  code = '{} {{\n'.format(sig)
  # code += generate_entry_debug_code(tree, fname, params, fname_ns='ipex')

  param_vars = get_param_names(params)
  if fnopts.outfn_template is not None:
    fcall = expand_fn_template(fnopts.outfn_template, param_vars)
  else:
    m = re.match(r'(.*)_out$', fname)
    assert m is not None, fname
    out_count = num_outputs if num_outputs is not None else 1
    fcall = create_call('AtenIpexType::{}'.format(m.group(1)),
                        param_vars[out_count:])

  tmp_result = '{}_tmp'.format(fname)
  code += '  auto {} = {};\n'.format(tmp_result, fcall)
  if num_outputs is None:
    code += generate_outfn_result_copy(param_vars[0], tmp_result)
    code += generate_exit_debug_code(tree, fname, param_vars[0], params,
                                     param_vars)
    code += '  return {};\n'.format(param_vars[0])
  else:
    for i in range(0, num_outputs):
      code += generate_outfn_result_copy(
          param_vars[i], 'std::get<{}>({})'.format(i, tmp_result))
    code += generate_exit_debug_code(tree, fname, param_vars[0:num_outputs],
                                     params, param_vars)
    code += '  return {}('.format(get_return_type_str(rwxtree, rwsig))
    for i in range(0, num_outputs):
      if i > 0:
        code += ', '
      code += param_vars[i]
    code += ');\n'
  code += '}'
  return code


def generate_aten_to_ipex(ctx, tree, rwxtree, fname, sig, rwsig, params, fnopts):
  ref_param = get_reference_param(params, fnopts=fnopts)

  code = '{} {{\n'.format(sig)
  # code += generate_entry_debug_code(tree, fname, params)
  ipex_ref_param = param_name(ref_param) if ref_param else None
  tfetcher = TensorFetcher('ipextens')
  param_vars = []
  for p in params:
    ptype = param_type(p)
    cptype = type_core(ptype)
    pname = param_name(p)
    if not is_blkfmt_supported(fname):
      if cptype == 'TensorList':
        _pname = "_{}".format(pname)
        pname_vec = "{}_vec".format(pname)
        code += ('  auto {} = AtenIpexTypeDPCPP::to_plain_if_needed({});\n').format(
            pname_vec, pname)
        code += ('  auto {} = at::TensorList({});\n').format(
            _pname, pname_vec)
        param_vars.append(_pname)
      elif cptype == 'Tensor':
        if not type_is_const(ptype):
          code += ('  {} = AtenIpexTypeDPCPP::to_plain_if_needed_({});\n').format(
              pname, pname)
          param_vars.append(pname)
        else:
          _pname = "_{}".format(pname)
          code += ('  auto {} = AtenIpexTypeDPCPP::to_plain_if_needed({});\n').format(
              _pname, pname)
          param_vars.append(_pname)
      else:
        param_vars.append(pname)
    else:
      param_vars.append(pname)
    # if cptype == 'TensorList':
    #   xname = 'l_{}'.format(pname)
    #   code += ('  auto {} = bridge::IpexCreateTensorList({});\n').format(
    #       xname, pname)
    #   param_vars.append(xname)
    # elif cptype == 'TensorOptions':
    #   gcode, xname = rewrite_tensor_options(fname, pname)
    #   code += gcode
    #   param_vars.append(xname)
    # elif cptype != 'Tensor':
    #   param_vars.append(pname)
    # elif type_is_const(ptype):
    #   xname = tfetcher.add(pname, is_write_param(fnopts, pname, False))
    #   param_vars.append(xname)
    # else:
    #   xname = tfetcher.add(pname, is_write_param(fnopts, pname, True))
    #   param_vars.append(xname)

    if p == ref_param and not get_optional(fnopts, 'ref_param'):
      ipex_ref_param = param_vars[-1]
  # code += tfetcher.generate_fetches()
  # result_assign = generate_result_assignment(tree, _RESULT_NAME)
  # code += '  {}{};\n'.format(
  #     result_assign, get_handling_function(ctx, fname, ipex_ref_param,
  #                                          param_vars))
  code += '  return {};\n'.format(
      get_handling_function(ctx, fname, ipex_ref_param, param_vars))
  # code += tfetcher.generate_updates()
  # if result_assign:
  #   code += ('  static_cast<void>({}); // Avoid warnings in case not '
  #            'used\n'.format(_RESULT_NAME))
  # code += generate_exit_debug_code(tree, fname,
  #                                  _RESULT_NAME if result_assign else None,
  #                                  params, param_vars)
  # code += generate_return_stmt(tree, get_return_type_str(rwxtree, rwsig), fname,
  #                              _RESULT_NAME if result_assign else None, params,
  #                              param_vars, ref_param, fnopts)
  code += '}'
  return code


def get_ipex_wrapper(fndef, ctx):
  tree = _PARSER.parse(fndef.cpp_sig)
  xtree = _XPARSER.parse(fndef.cpp_sig)
  mapsig = create_map_sig(xtree, fndef.cpp_sig)
  rwsig = rewrite_signature(fndef.cpp_sig, _TYPE_NSMAP)
  rwxtree = _XPARSER.parse(rwsig)
  params = get_parameters(tree)
  fnopts = _FUNCTION_OPTIONS.get(mapsig, None)


  def gen_fnname(x):
    return 'AtenIpexTypeDefault::{}'.format(x)

  sig, fname, xfname = get_function_signature(rwxtree, rwsig, gen_fnname)
  if not is_blacklisted_fn(fname, mapsig):
    # ofnopts = get_outfn_options(fname, mapsig)
    # rfnopts = get_remapfn_options(fname, mapsig)
    # if ofnopts is not None:
    #   code = generate_aten_out(ctx, tree, rwxtree, fname, sig, rwsig, params,
    #                            ofnopts)
    # elif rfnopts is not None:
    #   code = generate_aten_remap(ctx, fname, sig, params, rfnopts)
    # else:
    #   code = generate_aten_to_ipex(ctx, tree, rwxtree, fname, sig, rwsig, params,
    #                               fnopts)
    code = generate_aten_to_ipex(ctx, tree, rwxtree, fname, sig, rwsig, params,
                                fnopts)
  else:
    code = None
  return FuncGen(
      tree=tree,
      xtree=xtree,
      rwxtree=rwxtree,
      func=fname,
      xfunc=xfname,
      code=code,
      sig=fndef.cpp_sig,
      rwsig=rwsig,
      cppsig=sig,
      mapsig=mapsig,
      funsig=create_stdfunc_sig(rwxtree, rwsig),
      aten_sig=fndef.aten_sig)


def is_tensor_api(fndef):
  fndef = fndef.replace('at::', '')
  fndef = fndef.replace('c10::Device', 'Device')
  m = re.search(r'\bTensor\b', fndef)
  return m is not None, fndef


def extract_functions(path):
  functions = []
  errors = []
  for line in open(path, 'r'):
    m = re.match(r'\s*([^\s].*); //\s+(.*)', line)
    if not m:
      continue
    fndef = m.group(1)
    try:
      _XPARSER.parse(fndef)
      functions.append(FuncDef(cpp_sig=fndef, aten_sig=m.group(2)))
    except Exception as e:
      if is_tensor_api(fndef)[0]:
        errors.append((fndef, str(e)))
        print('Error parsing "{}": {}'.format(fndef, e), file=sys.stderr)
  return functions, errors


def get_mapsig_key(mapsig):
  # PyTorch generates std::tuple<> without space among the tuple types,
  # which would require special understanding in the string rewriter.
  # Since we are using this as simple key, we can just string the spaces.
  return mapsig.replace(' ', '')


def parse_local_overrides(path):
  functions = []
  fndef = None
  for line in open(path, 'r'):
    line = line.strip()
    if not fndef:
      m = re.match(r'static\s+(.*);', line)
      if m:
        functions.append(m.group(1))
        continue
      m = re.match(r'static\s+(.*)', line)
      if m:
        fndef = m.group(1)
    else:
      fndef = '{} {}'.format(fndef, line)
      if fndef.endswith(';'):
        functions.append(fndef[:-1])
        fndef = None
  assert fndef is None

  overrides = {}
  for fndef in functions:
    # Discard static IPEX type functions which are not ATEN.
    is_tensor, fndef = is_tensor_api(fndef)
    if is_tensor:
      xtree = _XPARSER.parse(fndef)
      mapsig_key = get_mapsig_key(create_map_sig(xtree, fndef))
      overrides[mapsig_key] = fndef
  return overrides


def generate_registrations(fgens, overrides, overrides_qint):
  code = 'void RegisterAtenTypeFunctions() {\n'
  code += '  static auto dispatch = torch::RegisterOperators()\n'
  overridden = set()
  for fgen in fgens:
    override_fn = ''
    override_fn_qint = ''
    mapsig_key = get_mapsig_key(fgen.mapsig)
    if mapsig_key in overrides:
      override_fn = 'AtenIpexTypeDefault::{}'.format(fgen.func)
      overridden.add(mapsig_key)
    else:
      override_fn = fgen.xfunc if fgen.code else None
    if mapsig_key in overrides_qint:
      override_fn_qint = 'AtenIpexTypeDefault::{}'.format(fgen.func)
      overridden.add(mapsig_key)    
    if override_fn != '':
      code += (
          '  .op(torch::RegisterOperators::options().schema("{}")\n      '
          '.impl_unboxedOnlyKernel<{}, &{}>(c10::DispatchKey::XPU)\n'
          '      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))\n'.format(
              fgen.aten_sig, fgen.funsig, override_fn, override_fn,
              fgen.aten_sig))
    if override_fn_qint != '':
      code += (
          '  .op(torch::RegisterOperators::options().schema("{}")\n      '
          '.impl_unboxedOnlyKernel<{}, &{}>(c10::DispatchKey::QuantizedDPCPPTensorId)\n'
          '      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))\n'.format(
              fgen.aten_sig, fgen.funsig, override_fn_qint, override_fn_qint,
              fgen.aten_sig))
  return code + ';\n}\n', overridden


def generate_functions(fgens):
  code = ''
  for fgen in fgens:
    if fgen.code:
      code += '{}\n\n'.format(fgen.code)
  return code


def generate_class_functions(fgens):
  code = ''
  for fgen in fgens:
    if fgen.code:
      code += '  static {};\n'.format(fgen.rwsig)
  return code


def generate_namespace_functions(fgens):
  code = ''
  for fgen in fgens:
    if fgen.code:
      code += '  {};\n'.format(fgen.rwsig)
  return code


def gen_output_file(args, name):
  if not args.output_folder:
    return sys.stdout
  return open(os.path.join(args.output_folder, name), 'w')


def gen_h_output_file(args):
  return gen_output_file(args, 'aten_ipex_type_default.h.in')


def gen_cpp_output_file(args):
  return gen_output_file(args, 'aten_ipex_type_default.cpp.in')


def gen_h_dpcpp_file(args):
  return gen_output_file(args, 'aten_ipex_type_dpcpp.h.in')


def check_overrides(overrides, overridden):
  misses = 0
  for mapsig, cpp_sig in overrides.items():
    mapsig_key = get_mapsig_key(mapsig)
    if not mapsig_key in overridden:
      misses += 1
      print(
          'AtenIpexType function missed override: {}; // {}'.format(
              cpp_sig, mapsig),
          file=sys.stderr)
  return misses == 0


def generate(args):
  fndefs, errors = extract_functions(args.typedef)
  print(
      'Extracted {} functions ({} errors) from {}'.format(
          len(fndefs), len(errors), args.typedef),
      file=sys.stderr)
  assert len(errors) == 0

  overrides = parse_local_overrides(args.ipextype)
  overrides_qint = parse_local_overrides(args.ipextype_qint)
  print(
      '{} function overrides in {}'.format(len(overrides), args.ipextype),
      file=sys.stderr)

  fgens = []
  ctx = Context(args.functions)
  for ts in fndefs:
    try:
      fgen = get_ipex_wrapper(ts, ctx)
      if fgen:
        fgens.append(fgen)
    except Exception as e:
      print(
          'Failed to generate wrapper for {}: {}'.format(ts, e),
          file=sys.stderr)
  print(
      'Generated {} wrappers for {}'.format(len(fgens), args.typedef),
      file=sys.stderr)

  functions = generate_functions(fgens)
  hfunctions = generate_class_functions(fgens)
  hnfunctions = generate_namespace_functions(fgens)
  regs, overridden = generate_registrations(fgens, overrides, overrides_qint)
  # assert check_overrides(overrides, overridden)
  # Create output files ...
  print(
      _H_HEADER.format(gen=os.path.basename(sys.argv[0]), hfuncs=hfunctions),
      file=gen_h_output_file(args))
  print(
      _CPP_HEADER.format(
          gen=os.path.basename(sys.argv[0]), funcs=functions, regs=regs),
      file=gen_cpp_output_file(args))
  print(
      _H_DPCPP_HEADER.format(gen=os.path.basename(sys.argv[0]), hfuncs=hnfunctions),
      file=gen_h_dpcpp_file(args))


if __name__ == '__main__':
  arg_parser = argparse.ArgumentParser()
  arg_parser.add_argument('--output_folder', type=str)
  arg_parser.add_argument(
      'ipextype',
      type=str,
      metavar='IPEX_TYPE_FILE',
      help='The path to the IPEX ATEN overrides file')
  arg_parser.add_argument(
      'ipextype_qint',
      type=str,
      metavar='IPEX_TYPE_FILE_INT8',
      help='The path to the IPEX ATEN overrides file')
  arg_parser.add_argument(
      'typedef',
      type=str,
      metavar='TYPE_DEFAULT_FILE',
      help='The path to the TypeDefault.h file')
  arg_parser.add_argument(
      'functions',
      type=str,
      metavar='FUNCTIONS_FILE',
      help='The path to the Functions.h file')
  args, files = arg_parser.parse_known_args()
  for file in files:
    print(file)
  generate(args)