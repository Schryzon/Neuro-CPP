#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <conio.h>
#include "dotenv.h"
#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"
#include "curlpp/Option.hpp"
#include "nlohmann/json.hpp"
const int UP = 72, DOWN = 80;
const int RIGHT = 77, LEFT = 75, ENTER = 13;
const int BACKSPACE = 8;
#define cursor_up std::cout<<"\x1b[A\x1b[2K"
#define flsh std::cin.ignore(9999, '\n')
#define failed std::cin.fail()
void banner(const std::string& color);
void chat_limiter(std::string&);

std::string input(const std::string &prompt){
    std::string in; std::cout<<prompt;
    std::getline(std::cin, in);
    return in;
}

int input(const std::string &prompt, int min, int max){
    int x; while(true){
        std::cout<<prompt;
        std::cin>>x;
        if(failed || x < min || x > max){
            std::cin.clear(); flsh;
            cursor_up;
            std::cout<<"\033[31mInvalid\033[0m: Please input between "<<min<<" and "<<max<<"!"<<std::endl;
        }else{
            flsh; return x;
        }
    }
}

struct Message{
    std::string role; // Either "user" or "model" TOLONG JANGAN GONTA GANTI
	std::string content;
};

struct Chat{
    std::string ai_name = "Neuro";
    std::string personality = "";
	std::string title;
	std::vector<Message> messages = {}; // DO NOT INIT Message{}
};

struct User{
    std::string username;
    std::string password;
    std::vector<Chat> chats = {Chat{}};
    int total_chat;
    int user_id;
};

struct Neuro{
	std::vector<User> users;
	int id = -1;
	int total_user = 0;

    Chat &get_current_chat(){
        return users[id].chats[users[id].total_chat];
    }

    Chat &get_current_chat(int idx){ // Overload for indexing
        return users[id].chats[idx];
    }

    User &get_current_user(){
        return users[id];
    }
};

void line(int length, char c){
	for(int i = 0; i < length; i++) std::cout << c;
	std::cout << std::endl;
} 

nlohmann::json prep_payload(const Chat *chat){
    nlohmann::json contents = nlohmann::json::array(); // For Neuro's memory
    for(auto message : chat->messages){
        nlohmann::json current_turn = {
            {"role", message.role},
            {"parts", {
                {{"text", message.content}}
            }}
        };
        contents.push_back(current_turn);
    }
    return contents;
}

nlohmann::json send_request_unlogged(std::string &input, std::string &persona, std::string &key){
    // Prepare JSON payload
    curlpp::Easy request;
    const std::string url = dotenv::getenv("AI_BASE_URL") + key;
    request.setOpt<curlpp::options::Url>(url);
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
    // Set headers
    std::list<std::string> headers = {
        "Content-Type: application/json"
    };
    request.setOpt(new curlpp::options::HttpHeader(headers));

    // Tell curlpp to POST our JSON
    request.setOpt(new curlpp::options::PostFields(body));
    request.setOpt(new curlpp::options::PostFieldSize(body.size()));

    // Capture response
    std::ostringstream response_stream;
    request.setOpt(new curlpp::options::WriteStream(&response_stream));

    // Perform the POST
    request.perform();

    // Parse and extract AI answer
    const std::string resp_str = response_stream.str();
    auto json = nlohmann::json::parse(resp_str);
    return json;
}

nlohmann::json send_request(Neuro *neuro, const Chat *chat, std::string &persona, std::string &key){
    // Prepare JSON payload
    curlpp::Easy request;
    const std::string url = dotenv::getenv("AI_BASE_URL") + key;
    request.setOpt<curlpp::options::Url>(url);
    nlohmann::json contents = prep_payload(chat);
    std::string add_sys_prompt = "You are talking to '" + neuro->get_current_user().username+"'";
    nlohmann::json payload = {
        {"system_instruction", {
            {"parts", {
                {{"text", (persona + add_sys_prompt)}}
            }}
        }},
        {"contents", contents}
    };
    std::string body = payload.dump();
    // Set headers
    std::list<std::string> headers = {
        "Content-Type: application/json"
    };
    request.setOpt(new curlpp::options::HttpHeader(headers));

    // Tell curlpp to POST our JSON
    request.setOpt(new curlpp::options::PostFields(body));
    request.setOpt(new curlpp::options::PostFieldSize(body.size()));

    // Capture response
    std::ostringstream response_stream;
    request.setOpt(new curlpp::options::WriteStream(&response_stream));

    // Perform the POST
    request.perform();

    // Parse and extract AI answer
    const std::string resp_str = response_stream.str();
    auto json = nlohmann::json::parse(resp_str);
    return json;
}

std::string get_answer_unlogged(std::string &input, std::string &persona, std::string &key){
    nlohmann::json json = send_request_unlogged(input, persona, key);
    std::string ai_answer = json["candidates"][0]["content"]["parts"][0]["text"].get<std::string>();
    return ai_answer;
}

std::string get_answer(Neuro *neuro, const Chat *chat, std::string &persona, std::string &key){
    nlohmann::json json = send_request(neuro, chat, persona, key);
    std::string ai_answer = json["candidates"][0]["content"]["parts"][0]["text"].get<std::string>();
    return ai_answer;
}

// For memory
void append_message(Neuro *neuro, std::string role, std::string content){
    auto& current_user = neuro->users[neuro->id];
	current_user.chats[current_user.total_chat].messages.push_back({role, content});
}

void continue_message(Neuro *neuro, int &pil, std::string role, std::string content){
    auto &current_user = neuro->users[neuro->id];
    current_user.chats[pil].messages.push_back({role, content});
}

// continue chat
void continue_chat(Neuro* neuro, int &pil){
    std::string name, ai_name, persona;
    auto GEMINI_API_KEY = dotenv::getenv("GEMINI_API_KEY");
    auto &user = neuro->get_current_user();
    auto &current_chat = neuro->get_current_chat(pil);
    name = user.username;
    ai_name = current_chat.ai_name;
    persona = current_chat.personality;
	std::string prompt, ai_answer;
    for(auto message : current_chat.messages){
        if(message.role == "user")
            std::cout << "[" << user.username << "]: " << message.content << std::endl;
        else
            std::cout<<"["<<current_chat.ai_name<<"]: "<<message.content<<std::endl;
    }
	do{
        line(73, '-');
        std::string display_name = "["+name+"]";
        std::string display_prompt = display_name +" (type 'exit' to go back): ";
		prompt = input(display_prompt);
        if (prompt == "exit" || prompt == "Exit") break;
        cursor_up; std::cout<<display_name<<": "<<prompt;
		std::cout<<std::endl<<std::endl;
        std::cout<<ai_name<<" is thinking...\n";
        continue_message(neuro, pil, "user", prompt);
        ai_answer = get_answer(neuro, &current_chat, persona, GEMINI_API_KEY);
		cursor_up;
        std::string display_ai_name = "["+ai_name+"]: ";
        std::cout <<display_ai_name;
        chat_limiter(ai_answer);
        continue_message(neuro, pil, "model", ai_answer);
	}while(prompt != "exit" && prompt != "Exit");
    std::cout<<"Returning to the previous menu..."<<std::endl;
    system("pause");
    return;
}
void history(Neuro* neuro){
    int pil, ans;
    auto& current_user = neuro->users[neuro->id];
    if (current_user.total_chat == 0) return;
    do {
        system("cls");
        banner("\033[1;32m");
        for (int i=0; i<current_user.total_chat; i++){
            std::cout<< i+1 << ". " << current_user.chats[i].title << std::endl;
        }
        ans = input("1. Continue Chat\n2. Delete Chat\n3. Back\n>> ", 1, 3);
        if(ans==1 || ans==2){
            pil = input("Select title: ", 1, current_user.total_chat);
            pil--;
        }
        system("cls");
        banner("\033[1;32m");
        switch(ans){
            case 1 : {
                continue_chat(neuro, pil);
                break;
            } 
            case 2 : {
                current_user.chats.erase(current_user.chats.begin() + pil);
                current_user.total_chat--;
            }
            default : return;
        }
    } while (ans == 1 || ans == 2);
}

void init_ai(Neuro* neuro, Chat &current_chat){
    banner("\033[35m");
    std::cout<<"Before starting your new chat,\n"
    <<"We would like you to customize your chatbot!\n";
    current_chat.ai_name = input("AI name (defaults to Neuro): ");
    current_chat.personality = input("AI personality (defaults to system prompt): ");
    if(current_chat.ai_name == "" || current_chat.ai_name.empty()){
        current_chat.ai_name = "Neuro";
    }if(current_chat.personality == "" || current_chat.personality.empty()){
        current_chat.personality = dotenv::getenv("DEFAULT_PERSONALITY");
    }
    std::cout<<"Initialization complete, you may now begin your chat with "<<current_chat.ai_name<<".\n";
    system("pause");
    system("cls");
}

void chat_limiter(std::string& text){
    int count = 0;
    for(size_t i = 0; i < text.size(); i++){
        std::cout << text[i];
        count++;
        if(count >= 73 && text[i] == ' '){
            std::cout << std::endl;
            count = 0;
        }
        Sleep(30);
    }
    std::cout << std::endl;
}

// First chat
void new_chat(Neuro* neuro){
    std::string name, ai_name, persona;
    auto GEMINI_API_KEY = dotenv::getenv("GEMINI_API_KEY");
    auto TITLE_KEY = dotenv::getenv("TITLE_KEY");
    auto TITLE_PERSONALITY = dotenv::getenv("TITLE_PERSONALITY");
    if(neuro->id != -1){
        auto &user = neuro->get_current_user();
        auto &current_chat = neuro->get_current_chat();
        init_ai(neuro, current_chat);
        name = user.username;
        ai_name = current_chat.ai_name;
        persona = current_chat.personality;
    }else{
        name = "You";
        ai_name = "Neuro";
        persona = dotenv::getenv("DEFAULT_PERSONALITY");
    }
	std::string prompt, ai_answer; bool first = true;
	do{
        if(first){
            banner("\033[1;32m");
            first = false;
        }else{
            line(73, '-');
        }
        std::string display_name = "["+name+"]";
        std::string display_prompt = display_name +" (type 'exit' to go back): ";
		prompt = input(display_prompt);
        if (prompt == "exit" || prompt == "Exit") break;
        cursor_up; std::cout<<display_name<<": "<<prompt;
		std::cout<<std::endl<<std::endl;
        std::cout<<ai_name<<" is thinking...\n";
		
        if(neuro->id != -1){
            append_message(neuro, "user", prompt);
            auto &user = neuro->get_current_user();
            auto &current_chat = neuro->get_current_chat();
            if(current_chat.title.empty() || current_chat.title == "")
                current_chat.title = get_answer_unlogged(prompt, TITLE_PERSONALITY, TITLE_KEY);
            ai_answer = get_answer(neuro, &current_chat, persona, GEMINI_API_KEY);
        // Hit API: if user is not logged in, Neuro will have an amnesia
        }else{
            ai_answer = get_answer_unlogged(prompt, persona, GEMINI_API_KEY);
        }
		cursor_up;
        std::string display_ai_name = "["+ai_name+"]: ";
        std::cout <<display_ai_name;
        chat_limiter(ai_answer);
        //std::cout<<display_ai_name<<ai_answer<<std::endl<<std::endl;
		if(neuro->id != -1){
            append_message(neuro, "model", ai_answer);
        }
	}while(prompt != "exit" && prompt != "Exit");
    if(neuro->id != -1){
        auto &user = neuro->get_current_user();
        user.chats.push_back(Chat{});
        user.total_chat++;
    }
    std::cout<<"Returning to the previous menu..."<<std::endl;
    system("pause");
    return;
}


void banner(const std::string& color) {
	line(73, '=');
    std::cout << color <<
R"(███╗   ██╗███████╗██╗   ██╗██████╗  ██████╗        ██████╗██████╗ ██████╗ 
████╗  ██║██╔════╝██║   ██║██╔══██╗██╔═══██╗      ██╔════╝██╔══██╗██╔══██╗
██╔██╗ ██║█████╗  ██║   ██║██████╔╝██║   ██║█████╗██║     ██████╔╝██████╔╝
██║╚██╗██║██╔══╝  ██║   ██║██╔══██╗██║   ██║╚════╝██║     ██╔═══╝ ██╔═══╝ 
██║ ╚████║███████╗╚██████╔╝██║  ██║╚██████╔╝      ╚██████╗██║     ██║     
╚═╝  ╚═══╝╚══════╝ ╚═════╝ ╚═╝  ╚═╝ ╚═════╝        ╚═════╝╚═╝     ╚═╝
)" << "\033[0m";
	line(73, '=');
}

void center_text(std::string title, int length){
	int padding = (length - title.length())/2;
	int remain = (length - title.length())/2 % 2;
	std::cout << "|";
	for(int i = 0; i < padding; i++) std::cout << " ";
	std::cout << title;
	for(int i = 0; i < padding + remain; i++) std::cout << " ";
	std::cout << "|" << std::endl;
}

void progress_bar(int percent){
    int length = 50;
    int filled = percent * length / 100;
    std::cout << "\r        [";
    for (int i = 0; i < filled; ++i) std::cout << "\033[1;32m#";
    for (int i = filled; i < length; ++i) std::cout << "\033[0m-";
	std::cout << "\033[0m] " << percent << "%" << std::flush;
}

void center(std::string text, int length, std::string color = "", bool isOption = false) {
    int padding = (length - text.length()) / 2;
    int remain = (length - text.length()) % 2;
    int option_length = (length - 22) / 2; int option_padding = length - 22 - option_length;
    if (isOption) {
        const int wavy_length = 22;
        int wavy_padding = (length - wavy_length) / 2;
        int wavy_right_padding = length - wavy_length - wavy_padding;
        std::cout << "|";
        for (int i = 0; i < wavy_padding; i++) std::cout << " ";
        for (int i = 0; i < wavy_length; i++) std::cout << "~";
        for (int i = 0; i < wavy_right_padding; i++) std::cout << " ";
        std::cout << "|" << std::endl;
    }
    std::cout << "|";
    for (int i = 0; i < padding; i++) std::cout << " ";
    if (color != "") std::cout << color;
    std::cout << text;
    if (color != "") std::cout << "\033[0m";
    for (int i = 0; i < padding + remain; i++) std::cout << " ";
    std::cout << "|" << std::endl;
    if (isOption) {
        const int wavy_length = 22;
        int wavy_padding = (length - wavy_length) / 2;
        int wavy_right_padding = length - wavy_length - wavy_padding;
        std::cout << "|";
        for (int i = 0; i < wavy_padding; i++) std::cout << " ";
        for (int i = 0; i < wavy_length; i++) std::cout << "~";
        for (int i = 0; i < wavy_right_padding; i++) std::cout << " ";
        std::cout << "|" << std::endl;
    }
}

int show_menu(Neuro* neuro, std::string options[], int num_choice, std::string title){
    int choice = 0;
    while(1){
        system("cls");
        banner("\033[1;34m");
        line(73,'~');
        center_text(title, 71);
		line(73,'=');
        for(int i = 0; i < num_choice; i++){
            std::string color = (i == choice) ? "\033[1;34m" : "";
            center(options[i], 71, color, true);
        }
        line(73, '=');
        int key = getch();
        if (key == 224) {
            key = getch();
            if (key == UP) {
                choice--;
                if (choice < 0) choice = num_choice - 1;
            } else if (key == DOWN) {
                choice++;
                if (choice >= num_choice) choice = 0;
            }
        } else if (key == ENTER) {
            break;
        }
    }
    return choice;
}

int* get_paddings(const std::string &text, int width = 73) {
    int* pads = new int[2]; 
    int total = width - text.length();
    pads[0] = total / 2;
    pads[1] = total - pads[0];
    return pads;
}

int confirm(std::string options[], int num_choice, std::string text) {
    int choice = 0, key;
    const int box_width = 71;
    const int half = (box_width / 2) - 2;

    do {
        system("cls");
        banner("\033[1;34m");
        if (!text.empty()) {
            center_text(text, box_width);
            line(box_width + 2, '=');
        }

        int* pad_left = get_paddings(options[1], half);
        int* pad_right = get_paddings(options[0], half);

        std::cout << "|";
        std::cout << std::string(pad_left[0], ' ');
        if (choice == 0) std::cout << "[\033[1;34m" << options[1] << "\033[0m]";
        else std::cout << " " << options[1] << " ";
        std::cout << std::string(pad_left[1], ' ');
        std::cout << "|";
        std::cout << std::string(pad_right[0], ' ');
        if (choice == 1) std::cout << "[\033[1;34m" << options[0] << "\033[0m]";
        else std::cout << " " << options[0] << " ";
        std::cout << std::string(pad_right[1], ' ');
        std::cout << "|" << std::endl;

        line(box_width + 2, '=');

        delete[] pad_left;
        delete[] pad_right;

        key = getch();
        if (key == 224 || key == 0) {
            key = getch();
            if (key == LEFT)  choice = 0;
            if (key == RIGHT) choice = 1;
        } else if (key == ENTER) {
            return choice;
        }
    } while (true);
}

void help(){
    banner("\033[0;32m");
    center_text("💡 HELP CENTER 💡", 75);
    line(73, '=');
    std::cout<<R"(
 🧠 1. NEUROCHAT - Start a new chat
   ➤ Custom AI Name
     → Set your AI’s name or press enter to use default.
   ➤ Custom AI Personality
     → Choose traits or press enter to skip.
   ➤ Start Chat 💬
     → Begin chatting with Neuro.
   ➤ Exit 🚪
     → Type "exit" to end chat.

📁 2. CHAT HISTORY - Manage previous sessions
   ➤ Continue Chat 🔁
     → Type "1" then choose a chat title to continue.
   ➤ Delete Chat 🗑️
     → Type "2" then select a chat title to delete.
   ➤ Exit 🔙
     → Type "3" to return to main menu.

➭ Use ↑ / ↓ to navigate. Press Enter to select.
)";
    system("pause");
}

void user_interface(Neuro* neuro){
	banner("\033[0m");
    line(73, '=');
    const int num_choice = 4;
    std::string options[] = {"NeuroChat", "Chat History", "Guide", "Log Out"};
    int choice;
    do {
        auto& current_user = neuro->users[neuro->id].username;
    	std::string title = "H E L L O " + current_user + "!";
        choice = show_menu(neuro, options, num_choice, title);
        system("cls");
        if(choice == 0) {
            new_chat(neuro);//chat(); 
        } else if(choice == 1) {
            history(neuro);
        } else if(choice == 2) {
            help();
        } else if(choice == 3) {
            neuro->id = -1;
        }
    } while(choice != 3);
    system("cls");
}

void signup(Neuro* neuro){
	banner("\033[0m");
	std::string new_name, new_password;
	bool success;
	do{
		success = true;
		new_name = input("Enter Name: ");
		for(auto user : neuro->users){
			if(user.username == new_name){
				std::cout << "Username not available!" << std::endl;
				success = false; break;
			}
		}
		if (success){
			new_password = input("Enter Password: ");
            neuro->id = neuro->total_user; //noted
			neuro->users.push_back({new_name, new_password}); 
            neuro->total_user++;
			std::cout<<"Account created successfully\n";
		}
		std::string options[] = {"Back", "Continue"};
		int result = confirm(options, 2, "Proceed to user dashboard?");
		if (result == 0) {
    		user_interface(neuro);
    		break;
		} else if (result == 1) {
    		break;
		}
	}while(1);
}

void login(Neuro* neuro){
	bool success = false;
	banner("\033[0m");
	std::string name;
	char ch;
    char char_password[100];
    int len = 0;
	name = input("Enter your name: ");
	std::cout << "Enter Password: \n";
	while ((ch = getch()) != ENTER) {
        if (ch == BACKSPACE) { 
            if (len > 0 && len < 100) {
                len--;
                std::cout << "\b \b";
            }
        }else {
            char_password[len++] = ch;
            std::cout << '*';
        }
    }
    char_password[len] = '\0';
    std::string password(char_password);

    std::cout<<std::endl;
	for(int i=0; i<neuro->users.size(); i++){
		if(neuro->users[i].username == name){
			if(neuro->users[i].password == password){
				std::cout << "Login Successful!";
				neuro->id = i;
				success = true;
			}
		}
	}
	if (success){
		user_interface(neuro);
	}else{
		std::cout<<"username not found\n";
	}
	system("pause");
}

void main_menu(Neuro* neuro){
    banner("\033[0m");
    line(73, '=');
    const int num_choice = 4;
    std::string options[] = {"NeuroChat", "Register", "Connect", "Disconnect"};
    int choice;
    do {
    	std::string title = "S Y N T A X - S E M A N T I C - S E N T I E N C E  ";
        choice = show_menu(neuro, options, num_choice, title);
        system("cls");
        if(choice == 0) {
            new_chat(neuro);
        } else if(choice == 1) {
            signup(neuro);
        } else if(choice == 2) {
            login(neuro);
        }
    } while(choice != 3);
    system("cls");
}

void loading(Neuro* neuro){
    std::string warna[] = {
        "\033[91m", "\033[92m", "\033[93m",
        "\033[94m", "\033[95m", "\033[96m",
        "\033[1;33m", "\033[1;31m"
    };
    int color_idx = 0;
    for (int i = 0; i <= 100; ++i) {
        if (i % 10 == 0) {
            system("cls");
            banner(warna[color_idx]);
            color_idx = (color_idx + 1) % 8;
            if(i == 100) std::cout << std::setw(43) << "Hello World!" << std::endl;
            else std::cout << std::setw(43) << "Loading ..." << std::endl;
        }
        progress_bar(i);
        Sleep(30);
    }
    std::cout << std::endl;
    Sleep(1500);
    system("cls");
    main_menu(neuro);
}

int main(){
    dotenv::init(".env"); // DO NOT CHANGE THIS
    Neuro* neuro = new Neuro();
    try{
        curlpp::Cleanup cleanup;
        loading(neuro);
    }
    catch (const curlpp::RuntimeError & e) {
        std::cerr << "Runtime error: " << e.what() << "\n";
        system("pause");
        return 1;
    }
    catch (const curlpp::LogicError & e) {
        std::cerr << "Logic error: " << e.what() << "\n";
        system("pause");
        return 1;
    }
    catch (const nlohmann::json::exception & e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        system("pause");
        return 1;
    }
    catch (const std::exception & e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        system("pause");
        return 1;
    }
    delete neuro;
    neuro = nullptr;
    return 0;
}