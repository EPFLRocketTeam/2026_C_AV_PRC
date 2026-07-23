#include "Application/Data/data.hpp"

using namespace prc;

BoardIdentity::BoardIdentity()
:   role(BoardRole::Unknown)
{}

BoardIdentityStore::BoardIdentityStore() {}

BoardRole BoardIdentityStore::get_role() const { return data_.role; }
