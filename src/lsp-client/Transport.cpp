#include "Transport.h"
#include "Log.h"

int JsonTransport::loop(MessageHandler &handler) {
  while (true) {
    try {
      value value;
      if (readJson(value)) {
        if (value.count("id")) {
          if (value.contains("method")) {
            handler.onRequest(value["method"].get<std::string>(),
                              value["params"], value["id"]);
          } else if (value.contains("result")) {
            handler.onResponse(value["id"].get<std::string>(), value["result"]);
          } else if (value.contains("error")) {
            handler.onError(value["id"].get<std::string>(), value["error"]);
          }
        } else if (value.contains("method")) {
          if (value.contains("params")) {
            handler.onNotify(value["method"].get<std::string>(),
                             value["params"]);
          }
        }
      } else {
        return -1;
      }
    } catch (std::exception &e) {
      LogError("JsonTransport:loop exception -> %s\n", e.what());
    }
  }
  return 0;
}
void JsonTransport::notify(string_ref method, value &params) {
  json value = {{"jsonrpc", jsonrpc}, {"method", method}, {"params", params}};
  writeJson(value);
}
void JsonTransport::request(string_ref method, value &params, RequestID &id) {
  json rpc = {
      {"jsonrpc", jsonrpc}, {"id", id}, {"method", method}, {"params", params}};
  writeJson(rpc);
}
