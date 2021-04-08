#pragma once

#include <mio/metaprogram/hash.hpp>

#define MIO_RAND() mio::metaprogram::hash{__COUNTER__}(__TIME__)