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

  template <typename State, typename Event, typename Action>
  struct Transition
  {
    State initial;
    Event event;
    State final;
    Action action;
  };

  template<typename Action, Action action>
  using Tag = std::integral_constant<Action, action>;

  template<typename... Handlers>
  static constexpr decltype(auto) make_handler(Handlers&&... handlers)
  {
    return detail::HandlerImpl<std::decay_t<Handlers>...>(
      std::forward<Handlers>(handlers)...);
  }

  template<typename State, typename Event, typename Action>
  using Model = std::vector<Transition<State, Event, Action>>;

  template <typename State, typename Event, typename Action, typename Handler>
  struct Manager
  {
    State state;

    template<Action action>
    using Tag = Tag<Action, action>;

    template<typename Model_, typename... Handlers>
    Manager(State initial_state, Model_&& model, Handlers&&... handlers)
      : state{initial_state}
      , handler{std::forward<Handlers>(handlers)...}
      , model{std::forward<Model_>(model)}
    {
    }

    template<typename Model_, typename Handler_>
    Manager(State initial_state, Model_&& model, Handler_&& handler_)
      : state{initial_state}
      , handler{std::forward<Handler_>(handler_)}
      , model{std::forward<Model_>(model)}
    {
    }

    template<typename... Args>
    void on(Event event, Args&&... args)
    {
      for (const auto& transition : model) {
        if (transition.initial == state && transition.event == event) {
          state = transition.final;
          dispatch_action((UAction)transition.action,
                          std::make_integer_sequence<UAction, (UAction)Action::Count>{},
                          std::forward<Args>(args)...);
          return;
        }
      }
    }

  private:
    Handler handler;
    Model<State, Event, Action> model;

    using UAction = std::underlying_type_t<Action>;

    template<typename... Args, UAction... Is>
    void dispatch_action(UAction action,
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

  template<typename State,
           typename Event,
           typename Action,
           typename... Handlers>
  Manager(State initial_state,
          Model<State, Event, Action> model,
          Handlers&&... handlers)
    -> Manager<State, Event, Action, detail::HandlerImpl<Handlers...>>;

}

#endif // STATEMENT_STATEMODEL_H
