#include <statement/StateModel.h>
#include <iostream>
#include <array>

namespace
{
  struct StateModel
  {
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
      Eleven,
      Twelve,

      Count,
    };

    using Model = statement::Model<State, Event, Action>;
    template <Action action>
    using Tag = statement::Tag<Action, action>;
  };
}

int main(int argc, char **argv)
{
  statement::Manager manager{
    StateModel::State::Disconnected,
    StateModel::Model{
      { StateModel::State::Disconnected,    StateModel::Event::StartRequest,
        StateModel::State::Connecting,      StateModel::Action::StartConnecting },

      { StateModel::State::Connecting,      StateModel::Event::Connected,
        StateModel::State::Connected,       StateModel::Action::StartDisconnecting },
      { StateModel::State::Connecting,      StateModel::Event::StopRequest,
        StateModel::State::Disconnecting,   StateModel::Action::StartDisconnecting },

      { StateModel::State::Connected,       StateModel::Event::StopRequest,
        StateModel::State::Disconnecting,   StateModel::Action::StartDisconnecting },

      { StateModel::State::Disconnecting,   StateModel::Event::Disconnected,
        StateModel::State::Disconnected,    StateModel::Action::StartConnecting },
    },
    [](StateModel::Tag<StateModel::Action::StartConnecting>, int i) {
      std::cout << "StartConnecting " << i << std::endl;
    },
    [](StateModel::Tag<StateModel::Action::StartDisconnecting>, char y) {
      std::cout << "StartDisconnecting " << y << std::endl;
    }
  };

  manager.on(StateModel::Event::StartRequest, argc);
  manager.on(StateModel::Event::StopRequest, (char)('a' + argc));
}
