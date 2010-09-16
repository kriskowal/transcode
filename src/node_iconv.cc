// -- kriskowal Kris Kowal Copyright (C) 2010 MIT License
#include <node_iconv.h>
#include <node/node_buffer.h>
#include <errno.h> // errno
#include <string.h> // strerror

namespace node {

using namespace v8;

Persistent<FunctionTemplate> Transcoder::constructor_template;
static Persistent<String> errno_symbol;
static Persistent<String> source_symbol;
static Persistent<String> source_start_symbol;
static Persistent<String> source_stop_symbol;
static Persistent<String> target_symbol;
static Persistent<String> target_start_symbol;
static Persistent<String> target_stop_symbol;
static Persistent<String> nonreverse_symbol;
static Persistent<String> error_symbol;
static Persistent<String> error_message_symbol;

static inline Local<Value> ErrnoException(int errorno) {
  Local<Value> error = Exception::Error(String::NewSymbol(
    strerror(errorno)));
  Local<Object> object = error->ToObject();
  object->Set(errno_symbol, Integer::New(errorno));
  return error;
}

// sourceCharset, targetCharset
Handle<Value> Transcoder::New(const Arguments &args) {
  HandleScope scope;
  Handle<Object> that = args.This();
  Transcoder *transcoder;
  Handle<String> sourceString, targetString;
  char *source, *target;

  if (!Transcoder::HasInstance(that))
    return ThrowException(Exception::Error(String::New(
      "Transcoder must be constructed with 'new'")));

  if (args[0]->IsUndefined())
    return ThrowException(Exception::Error(String::New(
      "Transcoder must be constructed with a source charset")));
  if (args[1]->IsUndefined())
    return ThrowException(Exception::Error(String::New(
      "Transcoder must be constructed with a target charset")));

  // source charset string
  sourceString = args[0]->ToString();
  source = new char[sourceString->Length() + 1];
  if (!sourceString->WriteAscii(source)) {
    delete source;
    return ThrowException(Exception::Error(String::New(
      "Source charset must be an ASCII value")));
  }

  // target charset string
  targetString = args[1]->ToString();
  target = new char[targetString->Length() + 1];
  if (!targetString->WriteAscii(target)) {
    delete source;
    delete target;
    return ThrowException(Exception::Error(String::New(
      "Source charset must be an ASCII value")));
  }

  iconv_t descriptor = iconv_open(source, target);
  delete source;
  delete target;

  if (descriptor == (iconv_t)-1)
    return ThrowException(Exception::Error(String::New(
      "Cannot construct a Transcoder for the given charsets")));

  transcoder = new Transcoder();
  transcoder->descriptor_ = descriptor;
  transcoder->Wrap(that);

  return that;
}

// source, target, source_start, source_stop, target_start, target_stop
Handle<Value> Transcoder::Transcode(const Arguments &args) {
  HandleScope scope;
  Transcoder *transcoder;

  if (!Transcoder::HasInstance(args.This()))
    return ThrowException(Exception::Error(String::New(
      "Transcoder.prototype.transcode must receive a Transcoder "
      "object as 'this'")));
  transcoder = ObjectWrap::Unwrap<Transcoder>(args.This());
  iconv_t descriptor = transcoder->descriptor_;

  if (args.Length() != 1 || !args[0]->IsObject())
    return ThrowException(Exception::Error(String::New(
      "Transcoder.prototype.transcode must receive exactly 1 "
      "argument, the state object")));
  Local<Object> state = args[0]->ToObject();

  Local<Object> source_object = state->Get(source_symbol)->ToObject();
  /*
  if (!Buffer::HasInstance(source_object))
    return ThrowException(Exception::Error(String::New("
    */
  Buffer *source = ObjectWrap::Unwrap<Buffer>(source_object);

  Local<Object> target_object = state->Get(target_symbol)->ToObject();
  /*
  if (!Buffer::HasInstance(target_object))
    return ThrowException(Exception::Error(String::New("
    */
  Buffer *target = ObjectWrap::Unwrap<Buffer>(target_object);

  // source start
  size_t source_start;
  source_start = state->Get(source_start_symbol)->Int32Value();
  if (source_start < 0 || source_start > source->length()) {
    return ThrowException(Exception::Error(String::New(
      "transcode() sourceStart out of bounds")));
  }

  // source stop
  size_t source_stop;
  if (state->Has(source_stop_symbol)) {
    source_stop = state->Get(source_stop_symbol)->Int32Value();
    if (source_stop < 0 || source_stop > source->length()) {
      return ThrowException(Exception::Error(String::New(
        "transcode() sourceStop out of bounds")));
    }
  } else {
    source_stop = source->length();
  }

  // target start
  size_t target_start;
  target_start = state->Get(target_start_symbol)->Int32Value();
  if (target_start < 0 || target_start > target->length()) {
    return ThrowException(Exception::Error(String::New(
      "transcode() targetStart out of bounds")));
  }

  // target stop
  size_t target_stop;
  if (state->Has(target_stop_symbol)) {
    target_stop = state->Get(target_stop_symbol)->Int32Value();
    if (target_stop < 0 || target_stop > target->length()) {
      return ThrowException(Exception::Error(String::New(
        "transcode() targetStop out of bounds")));
    }
  } else {
    target_stop = target->length();
  }

  #ifdef __FreeBSD__
  const
  #endif
  char *source_data = source->data() + source_start;

  char *target_data = target->data() + target_start;
  size_t source_capacity = source_stop - source_start;
  size_t target_capacity = target_stop - target_start;

  signed int nonreverse = iconv(
    descriptor,
    &source_data, &source_capacity, 
    &target_data, &target_capacity
  );

  const char *error = 0;
  if (nonreverse < 0) {
    if (errno == E2BIG) {
      error = "resize";
      state->Set(error_message_symbol, String::New(
        "There is not sufficient capacity in the target buffer range"));
    } else if (errno == EILSEQ) {
      error = "invalid";
      state->Set(error_message_symbol, String::New(
        "An invalid multi-byte sequence has been encountered in the "
        "souce buffer"));
    } else if (errno == EINVAL) {
      error = "incomplete";
      state->Set(error_message_symbol, String::New(
        "An incomplete multibyte sequence "
        "has been encountered in the source buffer"));
    } else {
      //return ThrowException(ErrnoException(errno));
    }
    nonreverse = 0;
  }

  state->Set(source_start_symbol, Integer::NewFromUnsigned(
      source_stop - source_capacity
  )); // source
  state->Set(target_start_symbol, Integer::NewFromUnsigned(
      target_stop - target_capacity
  )); // target
  state->Set(source_stop_symbol, Integer::NewFromUnsigned(
      source_stop
  )); // source
  state->Set(target_stop_symbol, Integer::NewFromUnsigned(
      target_stop
  )); // target
  state->Set(nonreverse_symbol, Integer::NewFromUnsigned(
      nonreverse + state->Get(nonreverse_symbol)->Int32Value()
  )); // nonreverse
  if (error)
    state->Set(error_symbol, String::New(error)); // error

  return scope.Close(state);
}

// target, start, stop
// flushes a reset sequence, closes, returns bytes written
Handle<Value> Transcoder::Close(const Arguments &args) {
  HandleScope scope;
  Transcoder *transcoder;

  if (!Transcoder::HasInstance(args.This()))
    return ThrowException(Exception::Error(String::New("Transcoder.prototype.close must "
      "receive a Transcoder object as 'this'")));

    /*
  Buffer *target;
  if (!args[0]->IsUndefined()) {
    if (!Buffer::HasInstance(args[0]))
      return ThrowException(Exception::Error(String::New("Transcoder.prototype.transcode "
        "must receive a Buffer object as arguments[0].")));
    target = ObjectWrap::Unwrap<Buffer>(args[0]->ToObject());
  } else {
  }
  */

  transcoder = ObjectWrap::Unwrap<Transcoder>(args.This());
  iconv_close(transcoder->descriptor_);

  return args.This();
}

void Transcoder::Initialize(Handle<Object> target) {
  HandleScope scope;

  errno_symbol = NODE_PSYMBOL("errno");
  source_symbol = NODE_PSYMBOL("source");
  source_start_symbol = NODE_PSYMBOL("sourceStart");
  source_stop_symbol = NODE_PSYMBOL("sourceStop");
  target_symbol = NODE_PSYMBOL("target");
  target_start_symbol = NODE_PSYMBOL("targetStart");
  target_stop_symbol = NODE_PSYMBOL("targetStop");
  nonreverse_symbol = NODE_PSYMBOL("nonReversible");
  error_symbol = NODE_PSYMBOL("error");
  error_message_symbol = NODE_PSYMBOL("message");

  // function Transcoder () {}
  Local<FunctionTemplate> transcoder_new = FunctionTemplate::New(
    Transcoder::New);
  constructor_template = Persistent<FunctionTemplate>::New(transcoder_new);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("Transcoder"));

  // Transcoder.prototype
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "transcode",
    Transcoder::Transcode);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "close",
    Transcoder::Close);

  // target.Transcoder = Transcoder
  target->Set(String::NewSymbol("Transcoder"),
    constructor_template->GetFunction());
}

}

extern "C" void init(v8::Handle<v8::Object> target) {
  node::Transcoder::Initialize(target);
}

