# odin-protoc-plugin
![build status](https://github.com/lordhippo/odin-protoc-plugin/actions/workflows/build.yml/badge.svg)

A Protobuf compiler [plugin](https://protobuf.dev/reference/other/#plugins) for Odin. It should be used with the runtime [odin-protobuf](https://github.com/lordhippo/odin-protobuf) library.

## Usage
Put this plugin's binary next to a [Protobuf Compiler (protoc)](https://github.com/protocolbuffers/protobuf) or in the PATH. Then use `--odin_out` argument to generate Odin files.

## Sample output
For this example proto file:

```proto
message SearchRequest {
  string query = 1;
  int32 page_number = 2;
  int32 results_per_page = 3;
}
```

This plugin will generate this Odin output:
```odin
SearchRequest :: struct {
  query : string `id:"1" type:"9"`,
  page_number : i32 `id:"2" type:"5"`,
  results_per_page : i32 `id:"3" type:"5"`,
}
```

## Missing features
- [Default values](https://github.com/lordhippo/odin-protoc-plugin/issues/5)
- [Dependencies](https://github.com/lordhippo/odin-protoc-plugin/issues/7)
- [Oneof check](https://github.com/lordhippo/odin-protoc-plugin)
