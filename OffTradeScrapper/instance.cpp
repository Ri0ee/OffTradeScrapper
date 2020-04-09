#include "instance.h"

std::string CurrentTimeAndDate()
{
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);

	std::stringstream ss;
	ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d");
	return ss.str();
}

std::string TimeToHours(long long int time_) {
	return std::to_string(time_ / 3600);
}

int Instance::Run() {
	TradeScrapper scrapper;

	auto ex_result = scrapper.GetExPrice();
	const double EX_PRICE = (ex_result.has_value())?ex_result.value():150;

	std::cout << "Using Exalted Orb price: " << EX_PRICE << " chaos\n";

	std::vector<std::string> gem_types = {
		"Awakened Curse On Hit Support", "Awakened Added Cold Damage Support", "Awakened Arrow Nova Support",
		"Awakened Cast On Critical Strike Support", "Awakened Chain Support", "Awakened Cold Penetration Support",
		"Awakened Deadly Ailments Support", "Awakened Fork Support", "Awakened Greater Multiple Projectiles Support",
		"Awakened Swift Affliction Support", "Awakened Vicious Projectiles Support", "Awakened Void Manipulation Support",
		"Awakened Added Fire Damage Support", "Awakened Ancestral Call Support", "Awakened Brutality Support",
		"Awakened Burning Damage Support", "Awakened Elemental Damage with Attacks Support",
		"Awakened Fire Penetration Support", "Awakened Generosity Support", "Awakened Melee Physical Damage Support",
		"Awakened Melee Splash Support", "Awakened Multistrike Support", "Awakened Added Chaos Damage Support",
		"Awakened Added Lightning Damage Support", "Awakened Blasphemy Support", "Awakened Cast While Channelling Support",
		"Awakened Controlled Destruction Support", "Awakened Curse On Hit Support", "Awakened Elemental Focus Support",
		"Awakened Increased Area of Effect Support", "Awakened Lightning Penetration Support", "Awakened Minion Damage Support",
		"Awakened Spell Cascade Support", "Awakened Spell Echo Support", "Awakened Unbound Ailments Support",
		"Awakened Unleash Support"
	};

	TradeScrapper::TradeRequest request;
	for (auto& gem_type : gem_types) {
		request.filters[gem_type].type = gem_type;
		request.filters[gem_type].gem.minimal_level = 1;
		request.filters[gem_type].gem.maximal_level = 1;
		request.filters[gem_type].corrupted = "false";
	}

	auto data = scrapper.GetTradeData(request);
	if (!data.has_value())
		return 0;

	std::map<std::string, double> average_buy_prices;
	std::map<std::string, double> average_sell_prices;
	std::map<std::string, double> average_indexing_time;
	int avg_counter = 0;

	//std::string log_file_name = "log_" + CurrentTimeAndDate() + ".txt";
	//std::fstream log_output(log_file_name, std::ios::out | std::ios::trunc);

	for (auto& req : data.value().information) {
		avg_counter = 0;

		if (req.second.size() == 0)
			continue;

		double minimal_price = 0;
		if (req.second.front().price.currency == "exa") {
			minimal_price = req.second.front().price.value * EX_PRICE;
		} 
		else if (req.second.front().price.currency == "chaos") {
			minimal_price = req.second.front().price.value;
		}

		for (auto& item : req.second) {
			//log_output << req.first << ": " << "level:" << item.gem.level << "\t\tquality:" << item.quality << "\t\tcorrupted:" << item.corrupted << "\t\t" << item.note << "\n";
			if (item.price.currency == "exa") {
				if (item.price.value * EX_PRICE > minimal_price + EX_PRICE * 10)
					continue;

				average_buy_prices[req.first] += item.price.value * EX_PRICE;
				avg_counter++;
			}
			else if (item.price.currency == "chaos") {
				if (item.price.value > minimal_price + EX_PRICE * 10)
					continue;

				average_buy_prices[req.first] += item.price.value;
				avg_counter++;
			}
		}

		average_buy_prices[req.first] = average_buy_prices[req.first] / (double)avg_counter;
	}

	for (auto& gem_type : gem_types) {
		request.filters[gem_type].gem.minimal_level = 5;
		request.filters[gem_type].gem.maximal_level = 5;
		request.filters[gem_type].corrupted = "false";
	}

	data = scrapper.GetTradeData(request);
	if (!data.has_value())
		return 0;
	
	for (auto& req : data.value().information) {
		avg_counter = 0;

		if (req.second.size() == 0)
			continue;

		double minimal_price = 0;
		if (req.second.front().price.currency == "exa") {
			minimal_price = req.second.front().price.value * EX_PRICE;
		}
		else if (req.second.front().price.currency == "chaos") {
			minimal_price = req.second.front().price.value;
		}

		for (auto& item : req.second) {
			average_indexing_time[req.first] = average_indexing_time[req.first] + (item.listing_time / req.second.size());

			//log_output << req.first << ": " << "level:" << item.gem.level << "\t\tquality:" << item.quality << "\t\tcorrupted:" << item.corrupted << "\t\t" << item.note << "\n";
			if (item.price.currency == "exa") {
				if (item.price.value * EX_PRICE > minimal_price + EX_PRICE * 10)
					continue;

				average_sell_prices[req.first] += item.price.value * EX_PRICE;
				avg_counter++;
			} 
			else if (item.price.currency == "chaos") {
				if (item.price.value > minimal_price + EX_PRICE * 10)
					continue;

				average_sell_prices[req.first] += item.price.value;
				avg_counter++;
			}
		}

		average_sell_prices[req.first] = average_sell_prices[req.first] / (double)avg_counter;
	}	
	
	std::string gem_info_file_name = "gem_info_" + CurrentTimeAndDate() + ".csv"; 
	std::fstream output(gem_info_file_name, std::ios::out | std::ios::trunc);
	output << "Gem type;Average buy (chaos);Average sell (chaos);Average profit(chaos);Average buy (EX);Average sell(EX);Average profit(EX);Average indexing time\n";

	for (auto& average_buy_price : average_buy_prices) {
		output << average_buy_price.first << ";";
		
		output.precision(3);

		// Chaos values
		output << std::fixed << average_buy_price.second << ";";
		output << std::fixed << average_sell_prices[average_buy_price.first] << ";";
		output << std::fixed << average_sell_prices[average_buy_price.first] - average_buy_price.second << ";";

		// Exalt values
		output << std::fixed << average_buy_price.second / EX_PRICE << ";";
		output << std::fixed << average_sell_prices[average_buy_price.first] / EX_PRICE << ";";
		output << std::fixed << (average_sell_prices[average_buy_price.first] - average_buy_price.second) / EX_PRICE << ";";
		output << TimeToHours(average_indexing_time[average_buy_price.first]) << "h\n";
	}
}