# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: envoy/config/trace/v3/opentelemetry.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf import symbol_database as _symbol_database
from google.protobuf.internal import builder as _builder
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


from envoy.config.core.v3 import grpc_service_pb2 as envoy_dot_config_dot_core_dot_v3_dot_grpc__service__pb2
from udpa.annotations import status_pb2 as udpa_dot_annotations_dot_status__pb2


DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(b'\n)envoy/config/trace/v3/opentelemetry.proto\x12\x15\x65nvoy.config.trace.v3\x1a\'envoy/config/core/v3/grpc_service.proto\x1a\x1dudpa/annotations/status.proto\"d\n\x13OpenTelemetryConfig\x12\x37\n\x0cgrpc_service\x18\x01 \x01(\x0b\x32!.envoy.config.core.v3.GrpcService\x12\x14\n\x0cservice_name\x18\x02 \x01(\tB\x89\x01\n#io.envoyproxy.envoy.config.trace.v3B\x12OpentelemetryProtoP\x01ZDgithub.com/envoyproxy/go-control-plane/envoy/config/trace/v3;tracev3\xba\x80\xc8\xd1\x06\x02\x10\x02\x62\x06proto3')

_globals = globals()
_builder.BuildMessageAndEnumDescriptors(DESCRIPTOR, _globals)
_builder.BuildTopDescriptorsAndMessages(DESCRIPTOR, 'envoy.config.trace.v3.opentelemetry_pb2', _globals)
if _descriptor._USE_C_DESCRIPTORS == False:
  DESCRIPTOR._options = None
  DESCRIPTOR._serialized_options = b'\n#io.envoyproxy.envoy.config.trace.v3B\022OpentelemetryProtoP\001ZDgithub.com/envoyproxy/go-control-plane/envoy/config/trace/v3;tracev3\272\200\310\321\006\002\020\002'
  _globals['_OPENTELEMETRYCONFIG']._serialized_start=140
  _globals['_OPENTELEMETRYCONFIG']._serialized_end=240
# @@protoc_insertion_point(module_scope)
