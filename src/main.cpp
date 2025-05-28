#include <iostream>
#include <vector>
#include <sstream>
#include "dotenv.h"
#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"
#include "nlohmann/json.hpp"

int main(){
    try{
        // 1) Initialize curlpp (RAII)
        curlpp::Cleanup cleanup;
        curlpp::Easy request;

        // 2) Set the URL
        const std::string url = "https://jsonplaceholder.typicode.com/posts/1";
        request.setOpt<curlpp::options::Url>(url);

        // 3) Capture the response in a stringstream
        std::ostringstream response_stream;
        request.setOpt(new curlpp::options::WriteStream(&response_stream));

        // 4) Perform the GET
        request.perform();

        // 5) Parse the JSON
        const std::string resp_str = response_stream.str();
        auto json = nlohmann::json::parse(resp_str);

        // 6) Extract fields
        int userId = json.at("userId").get<int>();
        int id = json.at("id").get<int>();
        std::string title = json.at("title").get<std::string>();
        std::string body  = json.at("body").get<std::string>();

        // 7) Print it out
        std::cout << "Post #" << id << " by user " << userId << ":\n";
        std::cout << "Title: " << title << "\n\n";
        std::cout << body << "\n";
    }
    catch (const curlpp::RuntimeError & e) {
        std::cerr << "Runtime error: " << e.what() << "\n";
        return 1;
    }
    catch (const curlpp::LogicError & e) {
        std::cerr << "Logic error: " << e.what() << "\n";
        return 1;
    }
    catch (const nlohmann::json::exception & e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return 1;
    }
    catch (const std::exception & e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}