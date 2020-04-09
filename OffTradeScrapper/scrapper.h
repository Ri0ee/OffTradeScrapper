#pragma once

#include <map>
#include <string>
#include <optional>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

#include "json.hpp" 

class TradeScrapper {
public:
	struct TradeData {
		struct Item {
			struct Price {
				std::string currency;
				double value;
			} price;

			struct Gem {
				std::string level;
				std::string experience;
			} gem;

			std::string note;
			std::string name;
			std::string type;
			std::string quality;
			std::string corrupted;

			std::string seller_char_name;
			std::string seller_acc_name;

			std::string indexed;
			long long int listing_time;
		};

		std::map<std::string, std::vector<Item>> information;
	};

	struct TradeRequest {
		struct Filter {
			struct Gems {
				int minimal_level;
				int maximal_level;
				std::string experience;
			} gem;

			struct Quality {
				std::string minimal;
				std::string maximal;
			} quality;

			std::string name;
			std::string base;
			std::string type;
			std::string corrupted;
		};

		std::map<std::string, Filter> filters;
	};

	TradeScrapper() {}

	std::optional<TradeData> GetTradeData(const TradeRequest& request_);
	std::optional<double> GetExPrice();

private:

	std::optional<std::vector<TradeData::Item>> ParseRawData(const std::vector<std::string>& page_);
	std::optional<std::vector<std::string>> GetTradeRawPage(TradeRequest::Filter filter_);
};