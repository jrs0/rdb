#include "string_lookup.h"

std::shared_ptr<StringLookup> new_string_lookup() {
    return std::make_shared<StringLookup>();
}
