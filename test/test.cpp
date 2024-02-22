#include <format>
#include <print>
#include <string>
#include <type_traits>
#include <variant>
#include <yaml/yaml.hpp>

#include <catch2/catch_test_macros.hpp>

template<class... Ts> struct overload : Ts...
{
  using Ts::operator()...;
};

TEST_CASE("Invoice")
{
  auto doc = std::string{
    "--- !<tag:clarkevans.com,2002:invoice>\n"
    "invoice: 34843\n"
    "date   : 2001-01-23\n"
    "bill-to: &id001\n"
    "  given  : Chris\n"
    "  family : Dumars\n"
    "  address:\n"
    "    lines: |\n"
    "      458 Walkman Dr.\n"
    "      Suite #292\n"
    "    city    : Royal Oak\n"
    "    state   : MI\n"
    "    postal  : 48046\n"
    "ship-to: *id001\n"
    "product:\n"
    "- sku         : BL394D\n"
    "  quantity    : 4\n"
    "  description : Basketball\n"
    "  price       : 450.00\n"
    "- sku         : BL4438H\n"
    "  quantity    : 1\n"
    "  description : Super Hoop\n"
    "  price       : 2392.00\n"
    "tax  : 251.42\n"
    "total: 4443.52\n"
    "comments:\n"
    "  Late afternoon is best.\n"
    "  Backup contact is Nancy\n"
    "  Billsmer @ 338-4338.\n"
  };

  std::println("{}", doc);

  auto res = yaml::load("[1, 2, 3]");

  std::println("{}",
    std::visit(overload{ [](const yaml::failsafe::scalar &_val) { return std::format("{}", _val); },
                 [](this auto const &self, const yaml::failsafe::sequence &_seq) {
                   auto content = std::string{};
                   for (auto &element : _seq) {
                     auto value = std::visit(self, std::static_pointer_cast<yaml::failsafe::node>(element)->data);
                     if (content.empty())
                       content += value;
                     else
                       content += std::format(", {}", value);
                   }
                   return std::format("[ {} ]", content);
                 },
                 [](this auto const &self, const yaml::failsafe::mapping &_map) {
                   auto content = std::string{};
                   for (auto &[k, v] : _map) {
                     auto key = std::visit(self, std::static_pointer_cast<yaml::failsafe::node>(k)->data);
                     auto value = std::visit(self, std::static_pointer_cast<yaml::failsafe::node>(v)->data);
                     auto formatted = std::format("{} : {}", key, value);
                     if (content.empty())
                       content += formatted;
                     else
                       content += std::format(", {}", formatted);
                   }
                   return std::format("{{ {} }}", content);
                 } },
      res.data));
}