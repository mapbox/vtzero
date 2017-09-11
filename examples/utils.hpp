
#include <vtzero/reader.hpp>

#include <string>

std::string read_file(const std::string& filename);

vtzero::layer get_layer(vtzero::vector_tile& tile, const std::string& layer_name_or_num);

