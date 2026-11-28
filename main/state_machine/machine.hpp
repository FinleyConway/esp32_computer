#pragma once

#include <utility>
#include <cstddef>
#include <variant>
#include <concepts>
#include <type_traits>

template<typename T>
concept state_type = requires (T t) {
    { t.init()   }  -> std::same_as<void>; // maybe have it so states must be machine aware??
    { t.deinit() }  -> std::same_as<void>; 
};

template<state_type... Ts>
class machine {
    static_assert(sizeof...(Ts) > 0, "need atleast one state");

public:
    ~machine() {
        if (started()) visit_current([](auto& s) { s.deinit(); });
    }

public:
    bool started() const { 
        return m_states.index() != 0; 
    }

    template<state_type T, typename... Args>
    void set_state(Args&&... args) {
        static_assert((std::is_same_v<T, Ts> || ...), "T is not in this machine's state list");

        // deinit state if state has already started
        if (started()) visit_current([](auto& s) { s.deinit(); }); 

        // destroy old state and construct new state 
        // into varient and start state
        m_states.template emplace<T>(std::forward<Args>(args)...);                                 
        visit_current([](auto& s) { s.init(); });
    }

    template<state_type T>
    bool is() const {
        static_assert((std::is_same_v<T, Ts> || ...), "T is not in this machine's state list");

        return std::holds_alternative<T>(m_states);
    }

private:
    template<class F>
    constexpr void visit_current(F&& f) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((m_states.index() == Is + 1 ? (f(*std::get_if<Is + 1>(&m_states)), true) : false) || ...);
        }(std::index_sequence_for<Ts...>{});
    }

private:
    std::variant<std::monostate, Ts...> m_states;
};