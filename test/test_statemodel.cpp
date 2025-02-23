#include <statement/StateModel.h>
#include <iostream>
#include <array>

namespace
{
  struct ModelTraits
  {
    using Manager = statement::Manager<ModelTraits>;
    enum class State {
      Disconnected,
      Connecting,
      Connected,
      Disconnecting,

      Count,
    };
    enum class Event {
      StartRequest,
      StopRequest,
      Connected,
      Disconnected,

      Count,
    };
    enum class Action {
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

      Count,
    };

    static constexpr auto initial_state = State::Disconnected;

    inline static constexpr Manager::Transition model[] = {
      { State::Disconnected,    Event::StartRequest,
        State::Connecting,      Action::StartConnecting },

      { State::Connecting,      Event::Connected,
        State::Connected,       Action::StartDisconnecting },
      { State::Connecting,      Event::StopRequest,
        State::Disconnecting,   Action::StartDisconnecting },

      { State::Connected,       Event::StopRequest,
        State::Disconnecting,   Action::StartDisconnecting },

      { State::Disconnecting,   Event::Disconnected,
        State::Disconnected,    Action::StartConnecting },
    };
  };
}

int main(int argc, char **argv)
{
  using Manager = ModelTraits::Manager;
  Manager manager;
  auto h = Manager::make_handler(
    [](Manager::Tag<ModelTraits::Action::StartConnecting>, int i) {
      std::cout << "StartConnecting " << i << std::endl;
    },
    [](Manager::Tag<ModelTraits::Action::StartDisconnecting>, char y) {
      std::cout << "StartDisconnecting " << y << std::endl;
    }
  );

  manager.on(h, ModelTraits::Event::StartRequest, argc);
  manager.on(h, ModelTraits::Event::StopRequest, 'a');
}
