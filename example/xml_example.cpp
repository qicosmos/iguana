#include <iguana/xml.hpp>

namespace client
{
	struct madoka
	{
		std::string	onegayi;
		double		power;
	};

	REFLECTION(madoka, onegayi, power);
}

int main(void)
{
	client::madoka m = { "Majyosine", 99999.999 };

	iguana::string_stream ss;
	iguana::xml::to_xml(ss, m);

	auto xml_str = ss.str();
	std::cout << xml_str << std::endl;

	client::madoka m2;
	iguana::xml::from_xml(m2, xml_str.data(), xml_str.length());

	std::cout << m2.onegayi << " - " << m2.power << std::endl;
	return 0;
}