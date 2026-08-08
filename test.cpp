#include <string_view>
#include <format>

int main () 
{
    std::string_view s = "";
    auto a = std::format("_{}_", s);

    printf ( "%s", s);

    return 0;
}
