#ifndef STATEMENT_STATEMODEL_H
#define STATEMENT_STATEMODEL_H

#include <stdexcept>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace statement {

  namespace detail {
    struct NoAction {
      template<typename... Args>
      void operator()(Args&&...) const
      {
        throw std::runtime_error("No handler for action");
      }
    };

    template<typename... Handlers>
    struct HandlerImpl : detail::NoAction, Handlers...
    {
      HandlerImpl(Handlers&&... handlers)
        : Handlers(std::forward<Handlers>(handlers))...
      {
      }

      using Handlers::operator()...;
      using NoAction::operator();
    };
    template<typename... Handlers>
    HandlerImpl(Handlers&&...) -> HandlerImpl<Handlers...>;

  }

  template <typename Traits>
  struct Manager
  {
    using Event = typename Traits::Event;
    using Action = typename Traits::Action;
    using State = typename Traits::State;

    template<Action action>
    using Tag = std::integral_constant<Action, action>;

    template<typename... Handlers>
    static constexpr decltype(auto) make_handler(Handlers&&... handlers)
    {
      return detail::HandlerImpl<std::decay_t<Handlers>...>(
        std::forward<Handlers>(handlers)...);
    }

    State state = Traits::initial_state;

    template<typename Handler, typename... Args>
    void on(Handler& handler, Event event, Args&&... args)
    {
      for (auto transition : Traits::model) {
        if (transition.initial == state && transition.event == event) {
          state = transition.final;
          dispatch_action(handler,
                          (UAction)transition.action,
                          std::make_integer_sequence<UAction, (UAction)Action::Count>{},
                          std::forward<Args>(args)...);
          return;
        }
      }
    }

    struct Transition
    {
      State initial;
      Event event;
      State final;
      Action action;
    };
    using Model = std::vector<Transition>;

    using UAction = std::underlying_type_t<Action>;

    template<typename Handler, typename... Args, UAction... Is>
    void dispatch_action(Handler& handler,
                         UAction action,
                         std::integer_sequence<UAction, Is...> action_seq,
                         Args&&... args)
    {
      auto do_action = [&](auto candidate) {
        if ((UAction)candidate.value == action) {
          std::invoke(handler,
                      candidate,
                      std::forward<Args>(args)...);
        }
      };
      (do_action(Tag<(Action)Is>{}), ...);
    }

  };
}

#endif // STATEMENT_STATEMODEL_H
