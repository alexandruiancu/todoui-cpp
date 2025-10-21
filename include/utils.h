#include <algorithm>
#include <string>

#include "opentelemetry/nostd/variant.h"

using ConfigValT = opentelemetry::nostd::variant<std::string, uint16_t>;
using ConfigMapT = std::unordered_map<std::string, ConfigValT>;

int init_app_config(ConfigMapT &config);
int init_attributes(const ConfigMapT &config, opentelemetry::sdk::resource::ResourceAttributes &ra);
