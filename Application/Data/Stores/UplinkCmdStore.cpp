#include "Application/Data/data.hpp"

using namespace prc;

UplinkCmd::UplinkCmd()
:   id(0),
    value(0)
{}

UplinkCmdStore::UplinkCmdStore() {}

uint8_t UplinkCmdStore::get_id() const { return data_.id; }
void UplinkCmdStore::set_id(uint8_t value) { data_.id = value; }

uint8_t UplinkCmdStore::get_value() const { return data_.value; }
void UplinkCmdStore::set_value(uint8_t value) { data_.value = value; }
