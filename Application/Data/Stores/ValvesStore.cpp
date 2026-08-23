#include "Application/Data/data.hpp"

using namespace prc;

Valves::Valves()
:  valve_safety_lox(false),
   valve_safety_fuel(false),
   valve_vent_lox(false),
   valve_vent_fuel(false),
   valve_main_lox(false),
   valve_main_fuel(false),
   ball_valve_lox(0.),
   ball_valve_fuel(0.)
{}

ValvesStore::ValvesStore() {}

bool ValvesStore::get_valve_safety_lox () const {
    return data_.valve_safety_lox;
}
void ValvesStore::set_valve_safety_lox(bool value) {
    data_.valve_safety_lox = value;
}

bool ValvesStore::get_valve_safety_fuel () const {
    return data_.valve_safety_fuel;
}
void ValvesStore::set_valve_safety_fuel(bool value) {
    data_.valve_safety_fuel = value;
}

bool ValvesStore::get_valve_vent_lox () const {
    return data_.valve_vent_lox;
}
void ValvesStore::set_valve_vent_lox(bool value) {
    data_.valve_vent_lox = value;
}

bool ValvesStore::get_valve_vent_fuel () const {
    return data_.valve_vent_fuel;
}
void ValvesStore::set_valve_vent_fuel(bool value) {
    data_.valve_vent_fuel = value;
}

bool ValvesStore::get_valve_main_lox () const {
    return data_.valve_main_lox;
}
void ValvesStore::set_valve_main_lox(bool value) {
    data_.valve_main_lox = value;
}

bool ValvesStore::get_valve_main_fuel () const {
    return data_.valve_main_fuel;
}
void ValvesStore::set_valve_main_fuel(bool value) {
    data_.valve_main_fuel = value;
}

float ValvesStore::get_ball_valve_lox () const {
    return data_.ball_valve_lox;
}
void ValvesStore::set_ball_valve_lox(float value) {
    data_.ball_valve_lox = value;
}

float ValvesStore::get_ball_valve_fuel () const {
    return data_.ball_valve_fuel;
}
void ValvesStore::set_ball_valve_fuel(float value) {
    data_.ball_valve_fuel = value;
}
