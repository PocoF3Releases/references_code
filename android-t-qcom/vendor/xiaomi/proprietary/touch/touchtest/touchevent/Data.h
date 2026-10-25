#ifndef DATA_H_
#define DATA_H_

#include <stdint.h>
#include <string>
#include <json/json.h>

using namespace std;

#define VAR_TO_STR(var) #var
#define VAR_TO_JSON(var, json) json[#var] = var;

struct data_interface {
    data_interface(const char *name) : name(name) {}
    virtual ~data_interface() = default;
    virtual bool toJson(Json::Value &json) { return false; }
    virtual void toStr(string &str) {}
    virtual void clear() {}
    const char *name;
};

struct count_type : public data_interface {
    count_type(const char *name) : data_interface(name), cnt(0) {}

    virtual bool toJson(Json::Value &json) override {
        if (cnt > 0) {
            json[name] = (Json::Int64)cnt;
            return true;
        }
        return false;
    }
    virtual void toStr(string &str) override {}
    virtual void clear() override { cnt = 0; }

    uint64_t cnt;
};

struct game_mode_type : public data_interface {
    game_mode_type(const char *name) : data_interface(name),
                                       tolerence(-1), up_threshold(-1), aim_sensitivity(-1),
                                       tap_stability(-1), edge_filter(-1){}

    int tolerence;
    int up_threshold;
    int aim_sensitivity;
    int tap_stability;
    int edge_filter;

    bool toJson(Json::Value &json) {
        if (tolerence != -1 && up_threshold != -1 &&
            aim_sensitivity != -1 && tap_stability != -1 && edge_filter != -1) {
            VAR_TO_JSON(tolerence, json);
            VAR_TO_JSON(up_threshold, json);
            VAR_TO_JSON(aim_sensitivity, json);
            VAR_TO_JSON(tap_stability, json);
            VAR_TO_JSON(edge_filter, json);
            return true;
        }
        return false;
    }

    void toStr(string &str) {
        str.append(string(VAR_TO_STR(tolerence)));
        str.append(" ");
        str.append(VAR_TO_STR(up_threshold));
        str.append(" ");
        str.append(VAR_TO_STR(aim_sensitivity));
        str.append(" ");
        str.append(VAR_TO_STR(tap_stability));
        str.append(" ");
        str.append(VAR_TO_STR(edge_filter));
        str.append(" ");
    }
    virtual void clear() override {
        tolerence = -1;
        up_threshold = -1;
        aim_sensitivity = -1;
        tap_stability = -1;
        edge_filter = -1;
    }
};

struct number_type : public data_interface {
    number_type(const char *name) : data_interface(name) {}
    virtual bool toJson(Json::Value &json) override {
        json[name] = (Json::Int64)number;
        return true;
    }
    virtual void toStr(string &str) override {}
    virtual void clear() override { number = -1; }
    int64_t number;
};

struct string_type : public data_interface {
    string_type(const char *name) : data_interface(name) {}

    virtual bool toJson(Json::Value &json) override {
        return false;
    }
    virtual void toStr(string &str) override {

    }
    virtual void clear() override { str.clear(); }
    string str;
};

#endif // DATA_H_
