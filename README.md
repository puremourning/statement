# statement: State models the way it was meant to be

I'm a big fan of "robust" state machines for event-driven servers. Think
something that has to wait for a start signal, handle user stop/pause/modify/etc
requests and maintain some external connection, say to a websocket, FIX link,
etc, and handle varioius events. This is trivial to write in about 20 LOCs using
a few enums and a (Start State, Event, Final State, Action) tuple.

But I've always wanted a syntactially convenient purely generic implementation.
So I tried to write one. It's far from perfect yet and TBH it's a bit hairy, but
it seems to work in some trivial tests and I'm quite pleased with the API.

Partially serious, partially a bit of fun.

Example usage:

```c++
// Somewhere
  statement::Manager manager{
    StateModel::State::Disconnected,
    StateModel::Model{
      { StateModel::State::Disconnected,    StateModel::Event::StartRequest,
        StateModel::State::Connecting,      StateModel::Action::StartConnecting },

      { StateModel::State::Connecting,      StateModel::Event::Connected,
        StateModel::State::Connected,       StateModel::Action::None },
      { StateModel::State::Connecting,      StateModel::Event::StopRequest,
        StateModel::State::Disconnecting,   StateModel::Action::StartDisconnecting },

      { StateModel::State::Connected,       StateModel::Event::StopRequest,
        StateModel::State::Disconnecting,   StateModel::Action::StartDisconnecting },
      { StateModel::State::Connected,       StateModel::Event::Disconnected,
        StateModel::State::Disconnected,    StateModel::Action::StartReconnectionTimer },

      { StateModel::State::Disconnecting,   StateModel::Event::Disconnected,
        StateModel::State::Disconnected,    StateModel::Action::None },
    },
    [&](StateModel::Tag<StateModel::Action::StartConnecting>, int retries) {
      my_connector.Start(retries);
    },
    [&](StateModel::Tag<StateModel::Action::StartDisconnecting>) {
      my_connector.Stop();
    }
    [&](StateModel::Tag<StateModel::Action::StartReconnectionTimer>, const std::string& reason) {
      std::cout << "Reconnecting because " << reason << std::endl;
      my_connector.Start();
    }
  };

// Elsewehere
  manager.on(StateModel::Event::StartRequest, num_retries);
  manager.on(StateModel::Event::Connected);
  manager.on(StateModel::Event::Disconnected, "Unexpected loss of service"s);

// use manager.state if you really want to
```

Alternatively, you can forego `statement::Manager` entirely. The entire library
is just one function `handle_event`:

```c++
    SimpleStateModel::Model model{
      { SimpleStateModel::State::Disconnected,    SimpleStateModel::Event::StartRequest,
        SimpleStateModel::State::Connected,       SimpleStateModel::Action::StartConnecting },
    };
    SimpleStateModel::State state = SimpleStateModel::State::Disconnected;
    auto h = statement::make_handler<SimpleStateModel::Action>(
      [](SimpleStateModel::Tag<SimpleStateModel::Action::StartConnecting>) {
      }
    );
    statement::handle_event(state,
                            model,
                            h,
                            SimpleStateModel::Event::StartRequest);

    assert(state == SimpleStateModel::State::Connected);
```

## Features

A state model is an initial state and list of transitions, each of which is a
tuple of (Start State, Event, End State, Action). Users define the states,
events and and provide handlers for the actions. 

As it is the only correct way to do it (and I will fight anyone who disagrees),
the new state is entered _before_ triggering the action handler. This allows
synchronous state transitions within action handlers.

As events can have arguments, action handlers may be passed those arguments, so
can be implemented generically, or explicitly for each event triggering the
action. Typically this is used sparingly, but can be useful for passing a
"reason" or something similar from event to action handler.

- [x] Generic state machine
- [x] Generic actions
- [x] Generic events, with arbitrary arguments
- [x] Generic states

## States

These are the states of the state machine. They are defined by an enum.

```c++
enum class State {
  Disconnected,
  Connecting,
  Connected,
  Disconnecting
};
```

## Events

These are the events that can trigger a state transition. They are defined by an
enum. The arguments are not specified here, but rather implicitly when
triggering the event.

```c++
enum class Event {
  StartRequest,
  StopRequest,
  Connected,
  Disconnected
};
```

## Actions

These are the actions that can be triggered by a state transition. They are 
defined by an enum.

There are 2 mandatory actions, which must be provided:

- `None` - does nothing
- `Count` - the number of actions, used for constructing the sequence of actions

The values of the enum must be strictly increasing from 0 to `Count`.

```c++
enum class Action {
  None,
  StartConnecting,
  StartDisconnecting,
  StartReconnectionTimer,
  Count
};
```

## Action handlers

These are implementations of the `operator()` with the first argument's type
matching `statement::Manager<...>::Tag<Action>`. The remaining arguments are the
event's arguments, if any. Thus the c++ overload mechanism is used to select the
handler via tag dispatch. A default implementation is provided as a catch-all
which throws an exception.

```c++
// Handle the StartConnecting action
void operator()(statement::Manager<State, Event, Action>::Tag<Action::StartConnecting>, int retries) {
  my_connector.Start(retries);
}
```

There are 3 ways to supply action handlers:

1. As a list of lambas passed as arguments to the manager constructor
2. via the `make_handler` function
2. As an instance of a type with `operator()` defined for all possible actions
   and event arguments

See `tests/test_statement.cpp` for examples of all 3.

When the state mamanger is a member of a class, you can pass the handler
'object' as argument to the `on` method.

```c++
  struct Thing
  {
    statement::Manager<SimpleStateModel::State, SimpleStateModel::Event, SimpleStateModel::Action> manager{
      SimpleStateModel::State::Disconnected,
      SimpleStateModel::Model{
        { SimpleStateModel::State::Disconnected,    SimpleStateModel::Event::StartRequest,
          SimpleStateModel::State::Connected,       SimpleStateModel::Action::StartConnecting },
      }
    };

    void start() {
      manager.on(*this, SimpleStateModel::Event::StartRequest);
    }

    void operator()(SimpleStateModel::Tag<SimpleStateModel::Action::StartConnecting>) {
    }
    void operator()(SimpleStateModel::Tag<SimpleStateModel::Action::None>) {
    }
  };
```
