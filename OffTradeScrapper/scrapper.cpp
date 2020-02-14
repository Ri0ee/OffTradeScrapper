#include "scrapper.h"

long long int GetTimePassed(std::string date_time_) {
	auto now = std::chrono::system_clock::now();
	auto in_time_t = std::chrono::system_clock::to_time_t(now);

	auto tnow = std::localtime(&in_time_t);
	time_t tnow_since_epoch = mktime(tnow);

	time_t tthen_since_epoch;
	std::tm t = {};
	std::istringstream ss(date_time_);

	if (ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S"))
		tthen_since_epoch = std::mktime(&t);

	return tnow_since_epoch - tthen_since_epoch;
}

std::optional<TradeScrapper::TradeData> TradeScrapper::GetTradeData(const TradeRequest& request_) {
	TradeData data;

	for (auto& filter : request_.filters) {
		auto page = GetTradeRawPage(filter.second);
		if (!page.has_value())
			continue;

		auto result = ParseRawData(page.value()); // Gets info about every item queried
		if (!result.has_value())
			continue;
		
		data.information.try_emplace(filter.first, result.value());
	}

	if (data.information.empty())
		return std::nullopt;

	return data;
}

std::optional<std::vector<TradeScrapper::TradeData::Item>> TradeScrapper::ParseRawData(const std::vector<std::string>& page_) {
	std::vector<TradeData::Item> items;

	for (auto& page : page_) {
		using nlohmann::json;
		json page_json = json::parse(page);

		//std::cout << page_json.dump(1, '\t') << "\n";

		page_json = page_json["result"];

		for (auto& item : page_json) {
			TradeData::Item thing;

			if(!item["item"]["note"].is_null())
				thing.note = item["item"]["note"].get<std::string>();

			if (!item["listing"]["price"].is_null()) {
				thing.price.currency = item["listing"]["price"]["currency"].get<std::string>();
				thing.price.value = item["listing"]["price"]["amount"].get<double>();
			}

			if (!item["listing"]["indexed"].is_null()) {
				thing.indexed = item["listing"]["indexed"].get<std::string>();
				thing.listing_time = GetTimePassed(thing.indexed);
			}
			
			if(!item["item"]["name"].is_null())
				thing.name = item["item"]["name"].get<std::string>();

			if(!item["item"]["typeLine"].is_null())
				thing.type = item["item"]["typeLine"].get<std::string>();

			if (!item["item"]["corrupted"].is_null())
				thing.corrupted = (item["item"]["corrupted"].get<bool>() == true) ? "true" : "false";
			else
				thing.corrupted = "false";

			json prop_json = item["item"]["properties"];
			for (auto& prop : prop_json) {
				if (prop["name"].get<std::string>() == "Quality") {
					json value = prop["values"];
					thing.quality = value[0][0].get<std::string>();
				}

				if (prop["name"].get<std::string>() == "Level") {
					json value = prop["values"];
					thing.gem.level = value[0][0].get<std::string>();

					if (thing.gem.level.find("(Max)") != std::string::npos) {
						thing.gem.level.erase(thing.gem.level.find("(Max)"), 5);
					}
				}
			}

			thing.seller_acc_name = item["listing"]["account"]["name"].get<std::string>();
			thing.seller_char_name = item["listing"]["account"]["lastCharacterName"].get<std::string>();

			items.push_back(thing);
		}
	}

	return items;
}

std::optional<std::vector<std::string>> TradeScrapper::GetTradeRawPage(TradeScrapper::TradeRequest::Filter filter_) {
	using namespace httplib;
	SSLClient client("www.pathofexile.com");

	std::cout << filter_.type;

	using nlohmann::json;
	json query = {
		{"query", {
			{"status", {
				{"option", "online"}
			}},
			{"type", filter_.type},
			{"stats", 
				json::array({
				{
					{"type", "and"}, 
					{"filters", json::array()}}
				})
			}
		}},
		{"sort", {
			{"price", "asc"}
		}}
	};

	if(filter_.gem.minimal_level > 0)
		query["query"]["filters"]["misc_filters"]["filters"]["gem_level"]["min"] = filter_.gem.minimal_level;

	if(filter_.gem.maximal_level > 0)
		query["query"]["filters"]["misc_filters"]["filters"]["gem_level"]["max"] = filter_.gem.maximal_level;

	if(!filter_.corrupted.empty())
		query["query"]["filters"]["misc_filters"]["filters"]["corrupted"]["option"] = filter_.corrupted;

	std::string query_body = query.dump();
	auto result = client.Post("/api/trade/search/Metamorph", query_body, "application/json");

	json response_json = json::parse(result->body);
	json result_line_json = response_json["result"];
	std::string id = response_json["id"];

	std::vector<std::string> hash_lines;
	for (auto& res_line : result_line_json)
		hash_lines.push_back(res_line);

	std::vector<std::string> page;
	std::string url = ("/api/trade/fetch/");
	bool first = true;
	for (int i = 1; i <= hash_lines.size(); i++) {
		if (first)
			first = false;
		else
			url.append(",");

		url.append(hash_lines[i - 1]);

		if (i % 10 == 0 || i == hash_lines.size()) {
			url.append("?query=");
			url.append(id);

			result = client.Get(url.c_str());
			if(result != nullptr)
				page.push_back(result->body);

			url = "/api/trade/fetch/";
			first = true;

			std::cout << ".";
		}
	}

	std::cout << "\n";

	return page;
}

std::optional<double> TradeScrapper::GetExPrice() {
	using namespace httplib;
	SSLClient client("poe.ninja");

	auto response = client.Get("/api/data/currencyoverview?league=Metamorph&type=Currency&language=en");

	using nlohmann::json;
	json response_json = json::parse(response->body);

	json lines = response_json["lines"];
	for (auto& line : lines)
		if (line["currencyTypeName"].get<std::string>() == "Exalted Orb")
			return line["chaosEquivalent"].get<double>();

	return std::nullopt;
}