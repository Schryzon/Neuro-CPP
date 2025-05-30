#include <iostream>
#include <vector>
#include <sstream>
#include "dotenv.h"
#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"
#include "curlpp/Option.hpp"
#include "curlpp/Types.hpp"
#include "nlohmann/json.hpp"
#include "script-mahesa-history.cpp"

/*
Roxas
Ventus
Hatsuki
Reina
Neuro-CPP
*/

/*
pesan

content
creator
model


chat -> history
judul
creator
struct pesan array

history chat -> array of struct dari pesan
for loopin
pesan[i].content 
pesan[i].creator
*/

nlohmann::json get_answer(curlpp::Easy &request, std::string &persona, std::string &input){
    std::getline(std::cin, persona);
    std::getline(std::cin, input);
    std::cout<<std::endl;
    // 3) Prepare JSON payload
    nlohmann::json payload = {
        {"system_instruction", {
            {"parts", {
                {{"text", persona}}
            }}
        }},
        {"contents", {
            {
                {"parts", {
                    {{"text", input}}
                }}
            }
        }}
    };
    std::string body = payload.dump();

    // 4) Set headers
    std::list<std::string> headers = {
        "Content-Type: application/json"
    };
    request.setOpt(new curlpp::options::HttpHeader(headers));

    // 5) Tell curlpp to POST our JSON
    request.setOpt(new curlpp::options::PostFields(body));
    request.setOpt(new curlpp::options::PostFieldSize(body.size()));

    // 6) Capture response
    std::ostringstream response_stream;
    request.setOpt(new curlpp::options::WriteStream(&response_stream));

    // 7) Perform the POST
    request.perform();

    // 8) Parse and extract AI answer
    const std::string resp_str = response_stream.str();
    auto json = nlohmann::json::parse(resp_str);
    return json;
}

int main(){
    try{
        // 1) Load API key
        std::string API_KEY = dotenv::getenv("GEMINI_API_KEY");
        curlpp::Cleanup cleanup;
        curlpp::Easy request;

        // 2) Build URL
        const std::string url =
            "https://generativelanguage.googleapis.com/v1beta/models/"
            "gemini-2.0-flash:generateContent?key=AIzaSyBFlEiiVLsHOTiLYCAnnQff2ql20DiW-rU";
        request.setOpt<curlpp::options::Url>(url);
        std::string personality;
        std::string input;
        do{
            auto json = get_answer(request, personality, input);
            // Gemini returns an array of candidates; take the first one's content
            std::string ai_answer = json["candidates"][0]["content"]["parts"][0]["text"].get<std::string>();

            // 9) Print it out
            std::cout << "Gemini says: " << ai_answer << "\n";
        }while(input != "exit");
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