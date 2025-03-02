#ifndef STATEMENT_STATEMODEL_H
#define STATEMENT_STATEMODEL_H

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace statement {

  template<typename Action, Action action>
  using Tag = std::integral_constant<Action, action>;

  template <typename State, typename Event, typename Action>
  struct Transition
  {
    State initial;
    Event event;
    State final;
    Action action;
  };

  template<typename State, typename Event, typename Action>
  using Model = std::vector<Transition<State, Event, Action>>;

  namespace detail {
    template<typename Action, Action ActionNone>
    struct NoAction {
      template<typename... Args>
      void operator()(Args&&...) const
      {
        throw std::runtime_error("No handler for action");
      }

      template<typename... Args>
      void operator()(Tag<Action, ActionNone>, Args&&...) const
      {
      }
    };

    template<typename Action, Action ActionNone, typename... Handlers>
    struct HandlerImpl : detail::NoAction<Action, ActionNone>, Handlers...
    {
      HandlerImpl(Handlers&&... handlers)
        : Handlers(std::forward<Handlers>(handlers))...
      {
      }

      using Handlers::operator()...;
      using NoAction<Action, ActionNone>::operator();
    };

    template<
      typename Action,
      typename Handler,
      typename... Args,
      typename UAction = std::underlying_type_t<Action>, UAction... Is>
    constexpr void dispatch_action(
      Handler&& handler,
      UAction action,
      std::integer_sequence<UAction, Is...> action_seq,
      Args&&... args)
    {
      // TODO: @Speed.
      // This is doing a check on every possible action at runtime. No early
      // exit even. A simple optimisation would be to say generate a swtich
      // statement (or at least an if/else chain) at compile time for Count < 10
      // or something.
      auto do_action = [&](auto candidate) {
        if ((UAction)candidate.value == action) {
          handler(candidate, std::forward<Args>(args)...);
        }
      };
      (do_action(Tag<Action, (Action)Is>{}), ...);
    }

  }

  template <typename State,
            typename Event,
            typename Action,
            typename Handler,
            typename... Args>
  void handle_event(State& state,
                    const Model<State, Event, Action>& model,
                    Handler&& handler,
                    Event event,
                    Args&&... args)
  {
    using UAction = std::underlying_type_t<Action>;
    // TODO: @Speed
    // This is a linear search. We could do a binary search if the model is
    // sorted by initial state and event.
    for (const auto& transition : model) {
      if (transition.initial == state && transition.event == event) {
        state = transition.final;
        detail::dispatch_action<Action>(
          handler,
          (UAction)transition.action,
          std::make_integer_sequence<UAction, (UAction)Action::Count>{},
          std::forward<Args>(args)...);
        return;
      }
    }
  }

  template<
    typename Action,
    Action ActionNone=Action::None,
    typename... Handlers>
  static constexpr decltype(auto) make_handler(Handlers&&... handlers)
  {
    return detail::HandlerImpl<Action, ActionNone, Handlers...>(
      std::forward<Handlers>(handlers)...);
  }

  template<
    typename State,
    typename Event,
    typename Action,
    typename Handler=void>
  struct Manager
  {
    State state;

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
      handle_event(state, model, handler, event, std::forward<Args>(args)...);
    }

  private:
    Handler handler;
    const Model<State, Event, Action> model;
  };

  template <typename State, typename Event, typename Action>
  struct Manager<State, Event, Action, void>
  {
    State state;

    template<typename Model_, typename... Handlers>
    Manager(State initial_state, Model_&& model)
      : state{initial_state}
      , model{std::forward<Model_>(model)}
    {
    }

    template<typename Handler, typename... Args>
    void on(Handler&& handler, Event event, Args&&... args)
    {
      handle_event(state, model, handler, event, std::forward<Args>(args)...);
    }

  private:
    const Model<State, Event, Action> model;
  };

  template<
    typename State,
    typename Event,
    typename Action,
    Action ActionNone=Action::None,
    typename... Handlers>
  Manager(State initial_state,
          Model<State, Event, Action> model,
          Handlers&&... handlers)
    -> Manager<State,
               Event,
               Action,
               detail::HandlerImpl<Action, ActionNone, Handlers...>>;

  template<
    typename State,
    typename Event,
    typename Action,
    Action ActionNone=Action::None>
  Manager(State initial_state,
          Model<State, Event, Action> model)
    -> Manager<State, Event, Action, void>;
}

#endif // STATEMENT_STATEMODEL_H
