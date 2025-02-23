#ifndef STATEMENT_STATEMODEL_H
#define STATEMENT_STATEMODEL_H

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

    static_assert((size_t)Action::Count <= 10, "Too many actions");

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
          switch((size_t)transition.action) {
#define STATEMENT_STATEMODEL_ACTION_CASE(action) \
          case action: \
            std::invoke(handler, \
                        Tag<(Action)action>{}, \
                        std::forward<Args>(args)...);\
            break

            STATEMENT_STATEMODEL_ACTION_CASE(0);
            STATEMENT_STATEMODEL_ACTION_CASE(1);
            STATEMENT_STATEMODEL_ACTION_CASE(2);
            STATEMENT_STATEMODEL_ACTION_CASE(3);
            STATEMENT_STATEMODEL_ACTION_CASE(4);
            STATEMENT_STATEMODEL_ACTION_CASE(5);
            STATEMENT_STATEMODEL_ACTION_CASE(6);
            STATEMENT_STATEMODEL_ACTION_CASE(7);
            STATEMENT_STATEMODEL_ACTION_CASE(8);
            STATEMENT_STATEMODEL_ACTION_CASE(9);
            STATEMENT_STATEMODEL_ACTION_CASE(10);

#undef STATEMENT_STATEMODEL_ACTION_CASE
          }
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
  };
}

#endif // STATEMENT_STATEMODEL_H
