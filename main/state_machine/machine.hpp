#pragma once

#include <utility>
#include <cstddef>
#include <variant>
#include <concepts>
#include <type_traits>

template<typename T, typename TMachine>
concept state_type = requires (T t, TMachine& m) {
    { t.init(m)   }  -> std::same_as<void>; // maybe have it so states must be machine aware??
    { t.deinit(m) }  -> std::same_as<void>; 
};

template<typename Ctx, typename... Ts>
class machine {
    static_assert(sizeof...(Ts) > 0, "need atleast one state");
    static_assert((state_type<Ts, machine> && ...), "state requires init(machine&) and deinit(machine&)");

public:
    explicit machine(Ctx& ctx) : m_ctx(ctx) {}

    ~machine() {
        if (started()) visit_current([](auto& s) { s.deinit(); });
    }

public:
    Ctx& ctx() { 
        return m_ctx; 
    }

    bool started() const { 
        return m_states.index() != 0; 
    }

    template<typename T, typename... Args>
    void set_state(Args&&... args) {
        static_assert((std::is_same_v<T, Ts> || ...), "T is not in this machine's state list");

        // deinit state if state has already started
        if (started()) visit_current([](auto& s) { s.deinit(); }); 

        // destroy old state and construct new state 
        // into varient and start state
        if constexpr (std::is_constructible_v<T, Ctx&, Args...>)
            m_states.template emplace<T>(m_ctx, std::forward<Args>(args)...);
        else
            m_states.template emplace<T>(std::forward<Args>(args)...);                              
        visit_current([](auto& s) { s.init(); });
    }

    template<typename T>
    bool is() const {
        static_assert((std::is_same_v<T, Ts> || ...), "T is not in this machine's state list");

        return std::holds_alternative<T>(m_states);
    }

private:
    template<typename Fn>
    constexpr void visit_current(Fn&& fn) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((m_states.index() == Is + 1 ? (fn(*std::get_if<Is + 1>(&m_states)), true) : false) || ...);
        }(std::index_sequence_for<Ts...>{});
    }

private:
    std::variant<std::monostate, Ts...> m_states;
    Ctx& m_ctx;
};