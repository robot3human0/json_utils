#ifndef OSON_RMP_SERVICE_UTILS_JSONCPP_UTILS_H_
#define OSON_RMP_SERVICE_UTILS_JSONCPP_UTILS_H_

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <json/json.h>

namespace jsoncpp_utils {

inline std::string toStr(double t, int precision = 8, bool trimLeadingZero = true) {
    char buf[64] = {0};
    size_t z = snprintf(buf, 64, "%.*f", precision, t);

    if (trimLeadingZero)
    {
        if (const char *dot = ::std::char_traits<char>::find(buf, z, '.'))
        {
            size_t dot_pos = dot - buf;

            while (z > dot_pos && buf[z - 1] == '0')
            {
                buf[--z] = '\0';
            }

            if (z > 0 && buf[z - 1] == '.') buf[--z] = '\0';
        }
    }

    return { static_cast<const char *>(buf), z };
}

inline std::string toStr(const Json::Value &value, bool pretty = false) {
    Json::StreamWriterBuilder writer;

    if (!pretty)
        writer["indentation"] = "";
    else
        writer["indentation"] = "    ";

    return Json::writeString(writer, value);
}

} // namespace jsoncpp_utils

template <typename T>
using JsonRef = std::pair<const char *, T &>;

template <typename T>
using JsonConstRef = std::pair<const char *, const T &>;

template <typename T>
JsonRef<T> mjRef(const char *name, T &t) {
    return JsonRef<T>(name, t);
}

template <typename T>
JsonConstRef<T> mjRef(const char *name, const T &t) {
    return JsonConstRef<T>(name, t);
}

template <typename T>
struct JsonDefaultInit {
  void operator()(T &ref) const { ref = T(); }

  void operator()(T &ref, const Json::Value &) const { (*this)(ref); }
};

template <typename T, typename void_t = void>
struct JsonInit : public JsonDefaultInit<T> {};

template <>
struct JsonInit<std::string> {
  void operator()(std::string &ref, const Json::Value &json) const {
      if (json.isNull()) {
          ref.clear();
      } else if (json.isString()) {
          ref = json.asString();
      } else if (json.isNumeric()) {
          if (json.isInt64()) {
              ref = std::to_string(json.asInt64());
          } else if (json.isDouble()) {
              ref = jsoncpp_utils::toStr(json.asDouble());
          }
      }
  }
};

template <typename T>
struct JsonInit<T,
                typename std::enable_if<std::is_arithmetic<T>::value>::type> {
  void operator()(T &ref, const Json::Value &json) const {
      if (json.isNumeric()) {
          if (json.isInt64()) {
              ref = static_cast<T>(json.asInt64());
          } else if (json.isDouble()) {
              ref = static_cast<T>(json.asDouble());
          }
      } else if (json.isString()) {
          try {
              ref = static_cast<T>(std::stod(json.asString()));
          } catch (...) {
              ref = T();
          }
      } else {
          ref = T();
      }
  }
};

template <>
struct JsonInit<bool> {
  void operator()(bool &ref, const Json::Value &json) const {
      if (json.isBool()) {
          ref = json.asBool();
      } else if (json.isString()) {
          std::string str = json.asString();
          ref = (str == "true" || str == "1");
      } else if (json.isNumeric()) {
          ref = json.asInt() != 0;
      } else {
          ref = false;
      }
  }
};

template <typename T>
const Json::Value &operator>>(const Json::Value &json, const JsonRef<T> &ref) {
    const char *name = ref.first;
    if (!json.isMember(name)) {
        JsonDefaultInit<T>()(ref.second);
        return json;
    }

    const Json::Value &value = json[name];
    JsonInit<T>()(ref.second, value);
    return json;
}

template <typename T>
Json::Value &operator<<(Json::Value &json, const JsonRef<T> &ref) {
    const char *name = ref.first;
    if (std::is_arithmetic<T>::value) {
        json[name] = ref.second;
    } else {
        json[name] = Json::Value(ref.second);
    }
    return json;
}

inline Json::Value vectorToJson(const std::vector<int> &array) {
    Json::Value jsonArray(Json::arrayValue);
    for (const auto &i : array) {
        jsonArray.append(i);
    }
    return jsonArray;
}

template<typename T>
const std::shared_ptr<Json::Value>& operator>>(const std::shared_ptr<Json::Value>& json, const JsonRef<T>& ref) {
    if (json) {
        *json >> ref;
    } else {
        JsonDefaultInit<T>()(ref.second);
    }
    return json;
}

template<typename T>
std::shared_ptr<Json::Value>& operator<<(std::shared_ptr<Json::Value>& json, const JsonRef<T>& ref) {
    if (!json) {
        json = std::make_shared<Json::Value>();
    }
    *json << ref;
    return json;
}

#endif //OSON_RMP_SERVICE_UTILS_JSONCPP_UTILS_H_
