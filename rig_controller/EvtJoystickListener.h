#include <limits.h>
#include <Arduino.h>


class EvtJoystickListener : public EvtListener {
  public:
  EvtJoystickListener();
  /*EvtJoystickListener(int pin, EvtAction trigger);
  EvtJoystickListener(int pin, int debounce, EvtAction action);
  int pin = 0;
  int debounce = 40;  
  bool targetValue = HIGH;
  bool mustStartOpposite = true;
  bool startState;
  unsigned long firstNoticed = 0;

  void setupListener();
  bool isEventTriggered();*/
};
