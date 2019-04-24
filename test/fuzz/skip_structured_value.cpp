
#include <vtzero/detail/attributes.hpp>

#include <iostream>
#include <vector>

int main() {
    const std::string input{std::istreambuf_iterator<char>(std::cin.rdbuf()),
                            std::istreambuf_iterator<char>()};

    std::cout << "Read " << input.size() << " bytes of data\n";

    std::vector<uint64_t> data;
    data.resize(input.size() / sizeof(uint64_t));

    std::cout << "This is " << data.size() << " 64bit values\n";

    std::memcpy(data.data(), input.data(), data.size() * sizeof(uint64_t));

    std::cout << "Values are:";
    for (const auto i : data) {
        std::cout << " " << i;
    }
    std::cout << '\n';

    auto it = data.cbegin();
    try {
        while (it != data.cend()) {
            vtzero::detail::skip_structured_value(nullptr, 0, it, data.cend());
        }
    } catch (const vtzero::format_exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

