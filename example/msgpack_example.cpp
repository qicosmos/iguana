#include <iguana/msgpack.hpp>

namespace client
{
	struct game
	{
		std::string	name;
		double		price;
	};
	REFLECTION(game, name, price);
}

int main(void)
{
	client::game game = { "overwatch", 258.0 };

	iguana::memory_buffer buffer;
	iguana::msgpack::to_msgpack(buffer, game);

	auto buf = buffer.release();
	client::game g2;
	iguana::msgpack::from_msgpack(g2, buf.data(), buf.size());

	std::cout << g2.name << " - " << g2.price << std::endl;

	return 0;
}