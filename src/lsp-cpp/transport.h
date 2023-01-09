//
// Created by Alex on 2020/1/28.
//

#ifndef LSP_TRANSPORT_H
#define LSP_TRANSPORT_H

#include "uri.h"
#include <functional>
#include <utility>

#define VERBOSE 0

using value = json;
using RequestID = std::string;

class MessageHandler {
public:
    MessageHandler() = default;
    virtual void onNotify(string_ref method, value &params) {}
    virtual void onResponse(value &ID, value &result) {}
    virtual void onError(value &ID, value &error) {}
    virtual void onRequest(string_ref method, value &params, value &ID) {}

};
class MapMessageHandler : public MessageHandler {
public:
    std::map<std::string, std::function<void(value &, RequestID)>> m_calls;
    std::map<std::string, std::function<void(value &)>> m_notify;
    std::vector<std::pair<RequestID, std::function<void(value &)>>> m_requests;
    MapMessageHandler() = default;
    template<typename Param>
    void bindRequest(const char *method, std::function<void(Param &, RequestID)> func) {
        m_calls[method] = [=](json &params, json &id) {
            Param param = params.get<Param>();
            func(param, id.get<RequestID>());
        };
    }
    void bindRequest(const char *method, std::function<void(value &, RequestID)> func) {
        m_calls[method] = std::move(func);
    }
    template<typename Param>
    void bindNotify(const char *method, std::function<void(Param &)> func) {
        m_notify[method] = [=](json &params) {
            Param param = params.get<Param>();
            func(param);
        };
    }
    void bindNotify(const char *method, std::function<void(value &)> func) {
        m_notify[method] = std::move(func);
    }
    void bindResponse(RequestID id, std::function<void(value &)>func) {
        m_requests.emplace_back(id, std::move(func));
    }
    void onNotify(string_ref method, value &params) override {
        std::string str = method.str();
        if (m_notify.count(str)) {
            m_notify[str](params);
        }
	    if (VERBOSE)
          	fprintf(stderr, "JsonTransport:onNotify -> %s\n", method.str().c_str());
    }
    void onResponse(value &ID, value &result) override {
        for (std::vector<std::pair<RequestID, std::function<void(value &)>>>::size_type i = 0; i < m_requests.size(); ++i) {
            if (ID == m_requests[i].first) {
                m_requests[i].second(result);
                m_requests.erase(m_requests.begin() + i);
                return;
            }
        }
	    if (VERBOSE)
          	fprintf(stderr, "JsonTransport:onRequest -> %s\n", result.dump().c_str());
    }
    void onError(value &ID, value &error) override {
	    // if (VERBOSE)
           	fprintf(stderr, "JsonTransport:onError -> %s\n", error.dump().c_str());
    }
    
    void onRequest(string_ref method, value &params, value &ID) override {
        std::string string = method.str();
        if (m_calls.count(string)) {
            m_calls[string](params, ID);
        }
	    if (VERBOSE)
          	fprintf(stderr, "JsonTransport:onRequest -> %s\n", method.str().c_str());
    }
};

class Transport {
public:
    virtual void notify(string_ref method, value &params) = 0;
    virtual void request(string_ref method, value &params, RequestID &id) = 0;
    virtual int loop(MessageHandler &) = 0;
};

class JsonTransport : public Transport {
public:
    const char *jsonrpc = "2.0";
    int loop(MessageHandler &handler) override {
        while (true) {
            try {
                value value;
                if (readJson(value)) {
                    if (value.count("id")) {
                        if (value.contains("method")) {
                            handler.onRequest(value["method"].get<std::string>(), value["params"], value["id"]);
                        } else if (value.contains("result")) {
                            handler.onResponse(value["id"], value["result"]);
                        } else if (value.contains("error")) {
                            handler.onError(value["id"], value["error"]);
                        }
                    } else if (value.contains("method")) {
                        if (value.contains("params")) {
                            handler.onNotify(value["method"].get<std::string>(), value["params"]);
                        }
                    }
                } else {
					return -1;
	          }
            } catch (std::exception &e) {
				//if (VERBOSE)
                	fprintf(stderr, "JsonTransport:loop exception -> %s\n", e.what());
            }
        }
        return 0;
    }
    void notify(string_ref method, value &params) override {
        json value = {{"jsonrpc", jsonrpc},
                      {"method",  method},
                      {"params",  params}};
        writeJson(value);
    }
    void request(string_ref method, value &params, RequestID &id) override {
        json rpc = {{"jsonrpc", jsonrpc},
                    {"id",      id},
                    {"method",  method},
                    {"params",  params}};
        writeJson(rpc);
    }
    virtual bool readJson(value &) = 0;
    virtual bool writeJson(value &) = 0;
};

#endif //LSP_TRANSPORT_H
