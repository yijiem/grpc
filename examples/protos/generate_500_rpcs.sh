#!/bin/bash

FILE=./examples/protos/helloworld500.proto

if test -f $FILE; then
  echo "$FILE exists."
fi

echo "syntax = \"proto3\";" > $FILE
echo >> $FILE
echo "package helloworld500;" >> $FILE
echo >> $FILE
echo "service Greeter {" >> $FILE
for i in {1..500}
do
  echo "  rpc SayHello$i (HelloRequest) returns (HelloReply) {}" >> $FILE
done
echo "}" >> $FILE
echo >> $FILE
echo "message HelloRequest {" >> $FILE
echo "  string name = 1;" >> $FILE
echo "}" >> $FILE
echo >> $FILE
echo "message HelloReply {" >> $FILE
echo "  string message = 1;" >> $FILE
echo "}" >> $FILE
echo >> $FILE
