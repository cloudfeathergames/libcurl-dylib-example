#include "restclient.h"
#include <iostream>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

int main() {
	RestClient rc;
    rc.init();
	RestClient::headermap hmap = {};
	const char* requestBody = "{\"title\":\"Test\"}";
	RestClient::response postResult = rc.post("https://dummyjson.com/products/add", "application/json", requestBody, hmap, 5);
	rapidjson::Document postResponse;
    postResponse.Parse(&postResult.body[0]);
	int id = postResponse["id"].GetInt();
	std::cout << "product id is " << id << '\n';
	RestClient::response getResult = rc.get("https://dummyjson.com/products/1", hmap, 5);
	rapidjson::Document getResponse;
    getResponse.Parse(&getResult.body[0]);
	std::string title = getResponse["title"].GetString();
	std::cout << "product 1 title is " << title << '\n';
	return 0;
}