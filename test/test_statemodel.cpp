#include <statement/StateModel.h>
#include <cassert>
#include <iostream>
#include <stdexcept>

namespace
{
  struct SimpleStateModel
  {
    enum class State {
      Disconnected,
      Connected,
    };
    enum class Event {
      StartRequest,
    };
    enum class Action {
      None,
      StartConnecting,

      Count,
    };
    using Model = statement::Model<State, Event, Action>;
    template <Action action>
    using Tag = statement::Tag<Action, action>;
  };

  struct StateModel
  {
    enum class State {
      Disconnected,
      Connecting,
      Connected,
      Disconnecting,
    };
    enum class Event {
      StartRequest,
      StopRequest,
      Connected,
      Disconnected,
    };
    enum class Action {
      None,
      StartConnecting,
      StartDisconnecting,
      Three,
      Four,
      Five,
      Six,
      Seven,
      Eight,
      Nine,
      Ten,
      Eleven,
      Twelve,


      Count,
    };

    using Model = statement::Model<State, Event, Action>;
    template <Action action>
    using Tag = statement::Tag<Action, action>;
  };
}

namespace
{
  int test_simple_pack(int argc, char **argv)
  {
    bool call_1_called = false;
    bool call_2_called = false;
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

        { StateModel::State::Disconnecting,   StateModel::Event::Disconnected,
          StateModel::State::Disconnected,    StateModel::Action::None },
      },
      // TODO/FIXME: For some reason whenever any overload is non-const, then we
      // get compilation error substituting std::invoke in dispatch_action
      // There's probably some std::remove_cvref stuff that we need but it's
      // already kind of hairy here.
      // That is, unless we remove 'const' from the NoAction operator() const.
      // In which case, it compiles but always throws that exception.
      // Tricky!
      [&](StateModel::Tag<StateModel::Action::StartConnecting>, int i) {
        call_1_called = true;
        std::cout << "StartConnecting " << i << std::endl;
      },
      [&](StateModel::Tag<StateModel::Action::StartDisconnecting>, char y) {
        call_2_called = true;
        std::cout << "StartDisconnecting " << y << std::endl;
      }
    };

    manager.on(StateModel::Event::StartRequest, argc);
    assert(call_1_called);
    assert(manager.state == StateModel::State::Connecting);
    manager.on(StateModel::Event::StopRequest, (char)('a' + argc));
    assert(call_2_called);
    assert(manager.state == StateModel::State::Disconnecting);
    manager.on(StateModel::Event::Disconnected);
    assert(manager.state == StateModel::State::Disconnected);
    return 0;
  }

  int test_simple_type(int argc, char **argv)
  {
    // FIXME: This API is bad. Need a way to deduce this better or at least make it
    // simpler to do this way.
    auto h = statement::make_handler<StateModel::Action>(
        [](StateModel::Tag<StateModel::Action::StartConnecting>, int i) {
          std::cout << "StartConnecting " << i << std::endl;
        },
        [](StateModel::Tag<StateModel::Action::StartDisconnecting>, char y) {
          std::cout << "StartDisconnecting " << y << std::endl;
        }
      );
    statement::Manager<StateModel::State, StateModel::Event, StateModel::Action, decltype(h)> manager{
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

        { StateModel::State::Disconnecting,   StateModel::Event::Disconnected,
          StateModel::State::Disconnected,    StateModel::Action::None },
      },
      h
    };

    manager.on(StateModel::Event::StartRequest, argc);
    manager.on(StateModel::Event::StopRequest, (char)('a' + argc));
    return 0;
  }

  int test_invalid_action(int argc, char** argv)
  {
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

        { StateModel::State::Disconnecting,   StateModel::Event::Disconnected,
          StateModel::State::Disconnected,    StateModel::Action::None },
      },
      [](StateModel::Tag<StateModel::Action::StartConnecting>, int i) {
        std::cout << "StartConnecting " << i << std::endl;
      }
    };

    manager.on(StateModel::Event::StartRequest, argc);
    try {
      manager.on(StateModel::Event::StopRequest, (char)('a' + argc));
    }
    catch (const std::exception& e) {
     return 0;
    }
    throw std::runtime_error("Expected an exception");
  }
  struct H
  {
    void operator()(StateModel::Tag<StateModel::Action::StartConnecting>, int i) {
      std::cout << "StartConnecting " << i << std::endl;
    }

    void operator()(StateModel::Tag<StateModel::Action::StartDisconnecting>, char y) {
      std::cout << "StartDisconnecting " << y << std::endl;
    }

    template<typename... Args>
    void operator()(StateModel::Tag<StateModel::Action::None>, Args&&...) {
    }

    template<typename...Args>
    void operator()( Args&&...) {
      throw std::runtime_error("No handler for action");
    }
  };
  int test_clever_type(int argc, char **argv)
  {
    // FIXME: This API is bad. Need a way to deduce this better or at least make it
    // simpler to do this way with CTAD.
    statement::Manager<StateModel::State, StateModel::Event, StateModel::Action, H> manager{
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

        { StateModel::State::Disconnecting,   StateModel::Event::Disconnected,
          StateModel::State::Disconnected,    StateModel::Action::None },
      },
      H(),
    };

    manager.on(StateModel::Event::StartRequest, argc);
    manager.on(StateModel::Event::StopRequest, (char)('a' + argc));
    return 0;
  }
  int test_mutable()
  {
    using StateModel = SimpleStateModel;
    statement::Manager manager{
      StateModel::State::Disconnected,
      StateModel::Model{
        { StateModel::State::Disconnected,    StateModel::Event::StartRequest,
          StateModel::State::Connected,       StateModel::Action::StartConnecting },
      },
      [](StateModel::Tag<StateModel::Action::StartConnecting>) {
      },
      [](StateModel::Tag<StateModel::Action::Count>) mutable {
        abort();
      }
    };

    manager.on(StateModel::Event::StartRequest);
    return 0;
  }

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
  int test_member()
  {
    Thing t;
    t.start();
    return (t.manager.state == SimpleStateModel::State::Connected) ? 0 : 1;
  }
}

int main(int argc, char **argv)
{

  return
    test_simple_pack(argc, argv)
      + test_simple_type(argc, argv)
      + test_invalid_action(argc, argv)
      + test_clever_type(argc, argv)
      + test_mutable()
      ;
}
