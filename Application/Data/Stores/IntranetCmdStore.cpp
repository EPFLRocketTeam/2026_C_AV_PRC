#include "Application/Data/data.hpp"

using namespace prc;

IntranetCmd::IntranetCmd()
:   id(0),
    value(0)
{}

IntranetCmdStore::IntranetCmdStore() {}

uint16_t IntranetCmdStore::get_id() const { return data_.id; }
void IntranetCmdStore::set_id(uint16_t value) { data_.id = value; }

uint8_t IntranetCmdStore::get_value() const { return data_.value; }
void IntranetCmdStore::set_value(uint8_t value) { data_.value = value; }
