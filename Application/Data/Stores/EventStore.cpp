#include "Application/Data/data.hpp"

using namespace prc;

Event::Event() {}

EventStore::EventStore() {}

bool EventStore::get_no_cable_continuity () const {
    return data_.no_cable_continuity;
}
void EventStore::set_no_cable_continuity (bool value) {
    data_.no_cable_continuity = value;
}
