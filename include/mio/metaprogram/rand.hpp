#pragma once

#include <mio/metaprogram/hash.hpp>

#define MIO_RAND() (static_cast<int64_t>(mio::metaprogram::hash{__COUNTER__ + mio::metaprogram::hash{}(__TIME__)}(__PRETTY_FUNCTION__)))