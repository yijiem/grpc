# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: envoy/extensions/filters/http/header_mutation/v3/header_mutation.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf import symbol_database as _symbol_database
from google.protobuf.internal import builder as _builder
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


from envoy.config.common.mutation_rules.v3 import mutation_rules_pb2 as envoy_dot_config_dot_common_dot_mutation__rules_dot_v3_dot_mutation__rules__pb2
from udpa.annotations import status_pb2 as udpa_dot_annotations_dot_status__pb2


DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(b'\nFenvoy/extensions/filters/http/header_mutation/v3/header_mutation.proto\x12\x30\x65nvoy.extensions.filters.http.header_mutation.v3\x1a:envoy/config/common/mutation_rules/v3/mutation_rules.proto\x1a\x1dudpa/annotations/status.proto\"\xb0\x01\n\tMutations\x12P\n\x11request_mutations\x18\x01 \x03(\x0b\x32\x35.envoy.config.common.mutation_rules.v3.HeaderMutation\x12Q\n\x12response_mutations\x18\x02 \x03(\x0b\x32\x35.envoy.config.common.mutation_rules.v3.HeaderMutation\"h\n\x16HeaderMutationPerRoute\x12N\n\tmutations\x18\x01 \x01(\x0b\x32;.envoy.extensions.filters.http.header_mutation.v3.Mutations\"`\n\x0eHeaderMutation\x12N\n\tmutations\x18\x01 \x01(\x0b\x32;.envoy.extensions.filters.http.header_mutation.v3.MutationsB\xca\x01\n>io.envoyproxy.envoy.extensions.filters.http.header_mutation.v3B\x13HeaderMutationProtoP\x01Zigithub.com/envoyproxy/go-control-plane/envoy/extensions/filters/http/header_mutation/v3;header_mutationv3\xba\x80\xc8\xd1\x06\x02\x10\x02\x62\x06proto3')

_globals = globals()
_builder.BuildMessageAndEnumDescriptors(DESCRIPTOR, _globals)
_builder.BuildTopDescriptorsAndMessages(DESCRIPTOR, 'envoy.extensions.filters.http.header_mutation.v3.header_mutation_pb2', _globals)
if _descriptor._USE_C_DESCRIPTORS == False:
  DESCRIPTOR._options = None
  DESCRIPTOR._serialized_options = b'\n>io.envoyproxy.envoy.extensions.filters.http.header_mutation.v3B\023HeaderMutationProtoP\001Zigithub.com/envoyproxy/go-control-plane/envoy/extensions/filters/http/header_mutation/v3;header_mutationv3\272\200\310\321\006\002\020\002'
  _globals['_MUTATIONS']._serialized_start=216
  _globals['_MUTATIONS']._serialized_end=392
  _globals['_HEADERMUTATIONPERROUTE']._serialized_start=394
  _globals['_HEADERMUTATIONPERROUTE']._serialized_end=498
  _globals['_HEADERMUTATION']._serialized_start=500
  _globals['_HEADERMUTATION']._serialized_end=596
# @@protoc_insertion_point(module_scope)
