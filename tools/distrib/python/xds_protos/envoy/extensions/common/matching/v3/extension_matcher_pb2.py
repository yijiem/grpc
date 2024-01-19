# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: envoy/extensions/common/matching/v3/extension_matcher.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf import symbol_database as _symbol_database
from google.protobuf.internal import builder as _builder
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


from envoy.config.common.matcher.v3 import matcher_pb2 as envoy_dot_config_dot_common_dot_matcher_dot_v3_dot_matcher__pb2
from envoy.config.core.v3 import extension_pb2 as envoy_dot_config_dot_core_dot_v3_dot_extension__pb2
from xds.annotations.v3 import status_pb2 as xds_dot_annotations_dot_v3_dot_status__pb2
from xds.type.matcher.v3 import matcher_pb2 as xds_dot_type_dot_matcher_dot_v3_dot_matcher__pb2
from envoy.annotations import deprecation_pb2 as envoy_dot_annotations_dot_deprecation__pb2
from udpa.annotations import status_pb2 as udpa_dot_annotations_dot_status__pb2
from validate import validate_pb2 as validate_dot_validate__pb2


DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(b'\n;envoy/extensions/common/matching/v3/extension_matcher.proto\x12#envoy.extensions.common.matching.v3\x1a,envoy/config/common/matcher/v3/matcher.proto\x1a$envoy/config/core/v3/extension.proto\x1a\x1fxds/annotations/v3/status.proto\x1a!xds/type/matcher/v3/matcher.proto\x1a#envoy/annotations/deprecation.proto\x1a\x1dudpa/annotations/status.proto\x1a\x17validate/validate.proto\"\xea\x01\n\x14\x45xtensionWithMatcher\x12\x45\n\x07matcher\x18\x01 \x01(\x0b\x32\'.envoy.config.common.matcher.v3.MatcherB\x0b\x18\x01\x92\xc7\x86\xd8\x04\x03\x33.0\x12\x31\n\x0bxds_matcher\x18\x03 \x01(\x0b\x32\x1c.xds.type.matcher.v3.Matcher\x12N\n\x10\x65xtension_config\x18\x02 \x01(\x0b\x32*.envoy.config.core.v3.TypedExtensionConfigB\x08\xfa\x42\x05\x8a\x01\x02\x10\x01:\x08\xd2\xc6\xa4\xe1\x06\x02\x08\x01\"Q\n\x1c\x45xtensionWithMatcherPerRoute\x12\x31\n\x0bxds_matcher\x18\x01 \x01(\x0b\x32\x1c.xds.type.matcher.v3.MatcherB\xab\x01\n1io.envoyproxy.envoy.extensions.common.matching.v3B\x15\x45xtensionMatcherProtoP\x01ZUgithub.com/envoyproxy/go-control-plane/envoy/extensions/common/matching/v3;matchingv3\xba\x80\xc8\xd1\x06\x02\x10\x02\x62\x06proto3')

_globals = globals()
_builder.BuildMessageAndEnumDescriptors(DESCRIPTOR, _globals)
_builder.BuildTopDescriptorsAndMessages(DESCRIPTOR, 'envoy.extensions.common.matching.v3.extension_matcher_pb2', _globals)
if _descriptor._USE_C_DESCRIPTORS == False:
  DESCRIPTOR._options = None
  DESCRIPTOR._serialized_options = b'\n1io.envoyproxy.envoy.extensions.common.matching.v3B\025ExtensionMatcherProtoP\001ZUgithub.com/envoyproxy/go-control-plane/envoy/extensions/common/matching/v3;matchingv3\272\200\310\321\006\002\020\002'
  _EXTENSIONWITHMATCHER.fields_by_name['matcher']._options = None
  _EXTENSIONWITHMATCHER.fields_by_name['matcher']._serialized_options = b'\030\001\222\307\206\330\004\0033.0'
  _EXTENSIONWITHMATCHER.fields_by_name['extension_config']._options = None
  _EXTENSIONWITHMATCHER.fields_by_name['extension_config']._serialized_options = b'\372B\005\212\001\002\020\001'
  _EXTENSIONWITHMATCHER._options = None
  _EXTENSIONWITHMATCHER._serialized_options = b'\322\306\244\341\006\002\010\001'
  _globals['_EXTENSIONWITHMATCHER']._serialized_start=346
  _globals['_EXTENSIONWITHMATCHER']._serialized_end=580
  _globals['_EXTENSIONWITHMATCHERPERROUTE']._serialized_start=582
  _globals['_EXTENSIONWITHMATCHERPERROUTE']._serialized_end=663
# @@protoc_insertion_point(module_scope)
