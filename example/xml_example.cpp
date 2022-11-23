#include <iguana/xml.hpp>

namespace client {
struct madoka {
  std::string onegayi;
  double power;
};

REFLECTION(madoka, onegayi, power);
} // namespace client

int main(void) {
  client::madoka m = {"Majyosine", 99999.999};

  iguana::string_stream ss;
  iguana::xml::to_xml(ss, m);

  std::cout << ss << std::endl;

  client::madoka m2;
  iguana::xml::from_xml(m2, ss.data(), ss.length());

  std::cout << m2.onegayi << " - " << m2.power << std::endl;
  return 0;
}