#pragma once

#include <typeinfo>
#include <functional>

template<typename R, typename T>
R fn_map (const T* obj) {
    (void) obj;
    return R();
}

template<typename R, typename T, typename H, typename... HS>
R fn_map (const T* obj, std::function<R (const H*)> handler, HS... handlers) {
    if (typeid(*obj) == typeid(H))
        return handler(dynamic_cast<const H*>(obj));

    else
        return fn_map<R>(obj, handlers...);
}

template<typename R, typename T, typename H, typename... HS>
R fn_map (const T* obj, R handler(const H*), HS... handlers) {
    if (typeid(*obj) == typeid(H))
        return handler(dynamic_cast<const H*>(obj));

    else
        return fn_map<R>(obj, handlers...);
}
