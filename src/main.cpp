#include <iostream>
#include <sstream>
#include <conio.h>
#include "dotenv.h"
#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"
#include "nlohmann/json.hpp"

const int UP = 72;
const int DOWN = 80;
const int RIGHT = 77;
const int LEFT = 75;
const int ENTER = 13;
const int BACKSPACE = 8;
const int max_chats = 500;
const int max_users = 100;
const int max_messages = 1000;

#define cursor_up std::cout<<"\x1b[A\x1b[2K"
#define flush_buffer std::cin.ignore(9999, '\n')

//structures
struct Message{
    std::string role; // Either "user" or "model" TOLONG JANGAN GONTA GANTI
	std::string content;
};

struct Chat{
    std::string ai_name = "Neuro";
    std::string personality = "";
	std::string title = "Untitled Topic\n";
	//std::vector<Message> messages = {}; //DO NOT INIT Message{}
    Message* messages = new Message[max_messages];
    int total_messages = 0;
};

struct User{
    std::string username;
    std::string password;
    //std::vector<Chat> chats = {Chat{}};
    Chat* chats = new Chat[max_chats];
    int total_chat;
    int user_id;
};

struct Neuro{
    User* users = new User[max_users];
	//std::vector<User> users;
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

//Global Variable
int last_top_choice = 0;

/*
Function prototypes below
Not necessary, but will help during presentation
Ctrl+Click is all we need
*/
//User Interfaces
inline void banner(const std::string& color);
inline void line(int length, char c);
inline void progress_bar(int percent);
inline void center(std::string text, int length, std::string color, bool isOption);
int show_menu(Neuro* neuro, std::string options[], int num_choice, std::string title);
inline int* get_paddings(const std::string &text, int width);
int confirm(std::string options[], int num_choice, std::string text);
inline void chat_limiter(std::string& x, bool typing);
inline void center_text(std::string title, int length, bool selected);
void display_history(std::string ai_name, std::string title, std::string color);
int navigate_history(Neuro *neuro);
inline void loading(Neuro* neuro);

//Helper Functions
inline std::string input(const std::string &prompt);
inline int input(const std::string &prompt, int min, int max);
inline int end_program(Neuro *neuro, int status_code);

//Core Formation
nlohmann::json prep_payload(const Chat *chat);
nlohmann::json post_request(curlpp::Easy &request, nlohmann::json payload);
nlohmann::json send_request_unlogged(std::string &input, std::string &persona, std::string &key);
nlohmann::json send_request(Neuro *neuro, const Chat *chat, std::string &persona, std::string &key);
std::string get_answer_unlogged(std::string &input, std::string &persona, std::string &key);
std::string get_answer(Neuro *neuro, const Chat *chat, std::string &persona, std::string &key);
inline void append_message(Neuro *neuro, std::string role, std::string content);
inline void continue_message(Neuro *neuro, int &pil, std::string role, std::string content);
void create_title(Neuro *neuro, Chat *chat);
void continue_chat(Neuro* neuro, int &pil);
void new_chat(Neuro* neuro);
void history(Neuro* neuro);
void init_ai(Neuro* neuro, Chat &current_chat);
int account(Neuro *neuro);
void signup(Neuro* neuro);
inline void help();
inline void credits();
void user_interface(Neuro* neuro);
void main_menu(Neuro* neuro);
void login(Neuro* neuro);

inline void chat_limiter(std::string& text, bool typing = true){
    int count = 0;
    for(size_t i = 0; i < text.size(); i++){
        std::cout << text[i];
        count++;
        if(count >= 70 && text[i] == ' '){
            std::cout << std::endl;
            count = 0;
        }
        if(typing) Sleep(30);
    }
    std::cout << std::endl;
}

inline void center_text(std::string title, int length = 73, bool selected = false){
	int padding = (length - title.length())/2;
	int remain = (length - title.length())/2 % 2;
	std::cout << "|";
	for(int i = 0; i < padding; i++) std::cout << " ";
    if(selected) std::cout<<"\033[1;34m";
	std::cout << title<<"\033[0m";
	for(int i = 0; i < padding + remain; i++) std::cout << " ";
	std::cout << "|" << std::endl;
}

inline std::string input(const std::string &prompt){
    std::string in; std::cout<<prompt;
    std::getline(std::cin, in);
    return in;
}

inline int input(const std::string &prompt, int min, int max){
    int x; while(true){
        std::cout<<prompt;
        std::cin>>x;
        if(std::cin.fail() || x < min || x > max){
            std::cin.clear(); flush_buffer;
            cursor_up;
            std::cout<<"\033[31mInvalid\033[0m: Please input between "<<min<<" and "<<max<<"!"<<std::endl;
        }else{
            flush_buffer; return x;
        }
    }
}

inline void line(int length, char c){
	for(int i = 0; i < length; i++) std::cout << c;
	std::cout << std::endl;
} 
/*
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
*/

//yg ga pake vector
nlohmann::json prep_payload(const Chat *chat){
    nlohmann::json contents = nlohmann::json::array(); // For Neuro's memory
    for(int i = 0; i<chat->total_messages; i++){
        nlohmann::json current_turn = {
            {"role", chat->messages[i].role},
            {"parts", {
                {{"text", chat->messages[i].content}}
            }}
        };
        contents.push_back(current_turn);

    }
    return contents;
}

// Non-async
nlohmann::json post_request(curlpp::Easy &request, nlohmann::json payload){
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

nlohmann::json send_request_unlogged(std::string &input, std::string &persona, std::string &key){
    curlpp::Easy request;
    const std::string url = dotenv::getenv("AI_BASE_URL") + key;
    request.setOpt<curlpp::options::Url>(url);
    int max_tokens = (key == dotenv::getenv("TITLE_KEY")) ? 70 : 256;
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
        }},
        {
            "generationConfig", {
                {"temperature", 1.0},
                {"maxOutputTokens", max_tokens}
            }
        }
    };
    return post_request(request, payload);
}

nlohmann::json send_request(Neuro *neuro, const Chat *chat, std::string &persona, std::string &key){
    curlpp::Easy request;
    const std::string url = dotenv::getenv("AI_BASE_URL") + key;
    request.setOpt<curlpp::options::Url>(url);
    nlohmann::json contents = prep_payload(chat);
    std::string add_sys_prompt = " You are talking to '" + neuro->get_current_user().username+"'";
    nlohmann::json payload = {
        {"system_instruction", {
            {"parts", {
                {{"text", (persona + add_sys_prompt)}}
            }}
        }},
        {"contents", contents},
        {
            "generationConfig", {
                {"temperature", 1.0},
                {"maxOutputTokens", 256}
            }
        }
    };
    return post_request(request, payload);
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
inline void append_message(Neuro *neuro, std::string role, std::string content){
    auto& current_user = neuro->users[neuro->id];
	//current_user.chats[current_user.total_chat].messages.push_back({role, content});
    current_user.chats[current_user.total_chat].messages[current_user.chats->total_messages] = {role, content};
    current_user.chats->total_messages++;
}

inline void continue_message(Neuro *neuro, int &pil, std::string role, std::string content){
    auto &current_user = neuro->users[neuro->id];
    //current_user.chats[pil].messages.push_back({role, content});
    current_user.chats[pil].messages[current_user.chats->total_messages] = {role, content};
    current_user.chats->total_messages++;
}

void create_title(Neuro *neuro, Chat *chat){
    auto TITLE_KEY = dotenv::getenv("TITLE_KEY");
    auto TITLE_PERSONALITY = dotenv::getenv("TITLE_PERSONALITY");
    if(chat->title == "Untitled Topic\n" || chat->title == "") {
        std::string message_content = neuro->get_current_user().username + ": " + chat->messages[0].content + " | "
        + chat->ai_name + ": " + chat->messages[1].content;
        chat->title = get_answer_unlogged(message_content, TITLE_PERSONALITY, TITLE_KEY);
        if(chat->title == ""){ // Sometimes it does this idk why
            chat->title = "Untitled Topic\n";
        }
    }
}

// continue chat
void continue_chat(Neuro* neuro, int &pil){
    auto GEMINI_API_KEY = dotenv::getenv("GEMINI_API_KEY");
    std::string name, ai_name, persona;
    auto &user = neuro->get_current_user();
    auto &current_chat = neuro->get_current_chat(pil);
    name = user.username;
    ai_name = current_chat.ai_name;
    persona = current_chat.personality;
	std::string prompt, ai_answer;
    for (int i = 0; i<current_chat.total_messages; i++){
        if(current_chat.messages[i].role == "user") std::cout << "[" << user.username << "]: ";
        else std::cout<<"["<<current_chat.ai_name<<"]: ";
        chat_limiter(current_chat.messages[i].content, false);
        std::cout<<std::endl;
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
        std::cout<<display_ai_name;
        chat_limiter(ai_answer);
        continue_message(neuro, pil, "model", ai_answer);
        create_title(neuro, &current_chat);
	}while(prompt != "exit" && prompt != "Exit");
    std::cout<<"Returning to the previous menu..."<<std::endl;
    system("pause");
    return;
}

void display_history(std::string ai_name, std::string title, std::string color = ""){
    bool selected = (color != "");
    // Wowie, banyak banget
    int name_box_left = (20 - ai_name.length()) / 2;
    int name_box_right = (20 - ai_name.length()) % 2;
    int total_name = name_box_left + name_box_right;
    int title_box_left = (53 - title.length()) / 2;
    int title_box_right = (53 - title.length()) % 2;
    int total_title = title_box_left + title_box_right;

    // History selection
    std::cout << " ";
    for (int i = 0; i<name_box_left; i++) std::cout << " ";
    if (selected) std::cout << color;
    std::cout << ai_name<<"\033[0m";
    for (int i = 0; i<total_name; i++) std::cout << " ";
    if (selected) std::cout << color;
    std::cout << "| " << title << "\033[0m";
}

int navigate_history(Neuro *neuro){
    int choice = 0;
    auto &total_chat = neuro->get_current_user().total_chat;
    int num_choice = total_chat + 1; // Add 1 for back
    while(1){
        system("cls");
        banner("\033[1;34m");
        center_text("Y O U R - H I S T O R Y", 71);
        std::string heading_ainame = "AI Name";
        int ainame_left = (20 - heading_ainame.length()) / 2;
        int ainame_right = (20 - heading_ainame.length()) % 2;
        std::string heading_title = "Title";
        int title_left = (50 - heading_title.length()) / 2;
        int title_right = (50 - heading_title.length()) % 2;

        // Table Heading
        line (73, '-');
        std::cout << "|";
        for (int i = 0; i<ainame_left; i++) std::cout << " ";
        std::cout << heading_ainame;
        for (int i = 0; i<ainame_left+ainame_right; i++) std::cout << " ";
        std::cout << "|";
        for (int i = 0; i<title_left; i++) std::cout << " ";
        std::cout << heading_title;
        for (int i = 0; i<title_left+title_right; i++) std::cout << " ";
        std::cout << "|" << std::endl;
        line (73, '-');

        for(int i = 0; i < total_chat; i++){
            auto& current_chat = neuro->get_current_user().chats[i];
            std::string color = (i == choice) ? "\033[1;34m" : "";
            display_history(current_chat.ai_name, current_chat.title, color);
        }
        line(73, '=');
        bool back_selected = (choice == num_choice - 1);
        center_text ("Back", 71, back_selected);
        
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

void history(Neuro* neuro){
    int pil, ans;
    auto& current_user = neuro->users[neuro->id];
    if (current_user.total_chat == 0){
        std::string options[] = {"Back", "Start New Chat"};
        int choice_empty = confirm(options, 2, "No Chat History Yet!");
        system("cls");
        if(choice_empty == 0)
            new_chat(neuro);
        return;
    }
    int choice = 0;
    do{
        int result = navigate_history(neuro);
        if (result != current_user.total_chat){
            std::string options[] = {"Delete", "Continue", "Back"};
            choice = confirm(options, 3, " Manage History");
            if (choice == 0) {
                system("cls"); 
                banner ("\033[1;34m");
                continue_chat(neuro, result);
                break;
            }
            else if(choice == 1){
                //current_user.chats.erase(current_user.chats.begin() + pil);
                current_user.total_chat--;
                for (int i = pil-1; i<current_user.total_chat; i++){
                    current_user.chats[i] = current_user.chats[i+1];
                }
            }
            else break;
        }else return;
    }while(choice != 2);
}

void init_ai(Neuro* neuro, Chat &current_chat){
    banner("\033[1;35m");
    std::cout<<"Before starting your new chat,\n"
    <<"We would like you to customize your chatbot!\n";
    current_chat.ai_name = input("AI name (defaults to Neuro): ");
    current_chat.personality = input("AI personality (defaults to system prompt): ");
    if(current_chat.ai_name == "" || current_chat.ai_name.empty())
        current_chat.ai_name = "Neuro";
    if(current_chat.personality == "" || current_chat.personality.empty())
        current_chat.personality = dotenv::getenv("DEFAULT_PERSONALITY");
    std::cout<<"Initialization complete, you may now begin your chat with "<<current_chat.ai_name<<".\n";
    system("pause"); system("cls");
}

// First chat
void new_chat(Neuro* neuro){
    auto GEMINI_API_KEY = dotenv::getenv("GEMINI_API_KEY");
    auto DEFAULT_PERSONALITY = dotenv::getenv("DEFAULT_PERSONALITY");
    std::string name, ai_name, persona;
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
        persona = DEFAULT_PERSONALITY;
    }
	std::string prompt, ai_answer; bool first = true;
	do{
        if(first){
            banner("\033[1;32m");
            first = false;
        }else line(73, '-');
        std::string display_name = "["+name+"]";
        std::string display_prompt = display_name +" (type 'exit' to go back): ";
		prompt = input(display_prompt);
        if (prompt == "exit" || prompt == "Exit") break;
        cursor_up; std::cout<<display_name<<": "<<prompt;
		std::cout<<std::endl<<std::endl;
        std::cout<<ai_name<<" is thinking...\n";
		
        if(neuro->id != -1){
            append_message(neuro, "user", prompt);
            auto &current_chat = neuro->get_current_chat();
            ai_answer = get_answer(neuro, &current_chat, persona, GEMINI_API_KEY);
            append_message(neuro, "model", ai_answer);
            // Create title here for more context
            create_title(neuro, &current_chat);
        // API: if user is not logged in, Neuro will have an amnesia
        }else
            ai_answer = get_answer_unlogged(prompt, persona, GEMINI_API_KEY);
		cursor_up;
        std::string display_ai_name = "["+ai_name+"]: ";
        std::cout <<display_ai_name;
        chat_limiter(ai_answer);
	}while(prompt != "exit" && prompt != "Exit");
    if(neuro->id != -1){
        auto &user = neuro->get_current_user();
        //user.chats.push_back(Chat{});
        user.total_chat++;
    }
    std::cout<<"\nReturning to the previous menu..."<<std::endl;
    system("pause");
}

inline void banner(const std::string& color) {
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

inline void progress_bar(int percent){
    int length = 50;
    int filled = percent * length / 100;
    std::cout << "\r        [";
    for (int i = 0; i < filled; ++i) std::cout << "\033[1;32m#";
    for (int i = filled; i < length; ++i) std::cout << "\033[0m-";
	std::cout << "\033[0m] " << percent << "%";
}

inline void center(std::string text, int length, std::string color = "", bool isOption = false) {
    int padding = (length - text.length()) / 2;
    int remain = (length - text.length()) % 2;
    int option_length = (length - 22) / 2; 
    int option_padding = length - 22 - option_length;
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

inline int* get_paddings(const std::string &text, int width = 73) {
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
        if (num_choice == 3) {
            if (choice == 2) center_text ("[\033[1;34m" + options[2] + "\033[0m] ", 82);
            else center_text(options[2], 71);
            line(73, '=');
        }
        
        delete[] pad_left;
        delete[] pad_right;
        pad_left = nullptr;
        pad_right = nullptr;

        key = getch();
        if (key == 224 || key == 0) {
            key = getch();
            if (key == LEFT)  {
                if(choice == 2){
                    choice = last_top_choice;
                }else{
                    choice = (choice - 1 + 2) % 2; // Fix modulo at two, prevent getting stuck
                    last_top_choice = choice;
                }
            }
            if (key == RIGHT) {
                if(choice == 2){
                    choice = last_top_choice;
                }else{
                    choice = (choice + 1) % 2;
                    last_top_choice = choice;
                }
            }
            if(key == DOWN && num_choice == 3){
                if(choice == 0 || choice == 1){
                    last_top_choice = choice;
                }
                choice = 2;
            }
            if(key == UP && num_choice == 3){
                if(choice == 2){
                    choice = last_top_choice;
                }
            }
        } else if (key == ENTER) {
            return choice;
        }
    } while (true);
}

inline void help(){
    banner("\033[0;32m");
    center_text(" 💡 HELP CENTER 💡", 75);
    line(73, '=');
    std::cout<<R"(
1. NEUROCHAT - Start a new chat
    By logging in, you will get three of the following benefits:
    ➤ Custom AI Name 
     → Set your AI's name or press enter to use default.

    ➤ Custom AI Personality
     → Choose traits or press enter to skip.

    ➤ AI Memory
     → Your AI will remember previous messages.

    Otherwise, you will only gain access to the following features:
    ➤ Start Chat 
     → Begin chatting with the AI.

    ➤ Exit 
     → Type "exit" to end chat.

2. CHAT HISTORY - Manage previous sessions
    Select one of the previously created chat sessions.
    You'll get to do the following:
    ➤ Continue Chat 

    ➤ Delete Chat 

    ➤ Return to Previous Menu

3. ACCOUNT - Manage account

    ➤ Update Profile 
     → Choose data to update (username or password).

    ➤ Delete Account 
     → Delete your account and erase all data.

➭ Use ↑ / ↓ / ← / → to navigate. Press Enter to select.

)";
    system("pause");
}

int account(Neuro *neuro){
    auto& current_user = neuro->users[neuro->id];
    int result;
    //check most chatted
    std::string title_biggest;
    int biggest = 0;
    for (int i=0; i<current_user.total_chat; i++){
        if (current_user.chats[i].total_messages > biggest){
            biggest = current_user.chats[i].total_messages;
            title_biggest = "[" + current_user.chats[i].ai_name + "] - " + current_user.chats[i].title;
        }
    }
    title_biggest = (!(title_biggest.empty())) ? title_biggest : "None, yet.";
    do {
        system("cls"); banner("\033[1;32m");
        std::cout << "Username\t: " << current_user.username << std::endl
        << "Password\t: " << current_user.password << std::endl
        << "Total Chat\t: " << current_user.total_chat << " chat(s)" << std::endl
        << "Most Frequently Chatted: " << title_biggest << std::endl;
        system("pause");
        std::string options[] = {"Delete Account", "Update Profile", "Back"};
        result = confirm(options, 3, "Manage your Account");
        if (result == 0){
            std::string options[] = {"Change Password", "Change Username", "Back"};
            int ans = confirm(options, 3, "Update Profile ");
            if (ans == 0){
                do{
                    bool success = true;
                    std:: string new_name = input("Enter new username: "); 
                    for(int i=0; i<neuro->total_user; i++){
                        if(neuro->users[i].username == new_name){
                            std::cout << "Username not available!" << std::endl;
                            success = false; break;
                        }
                    }
                    if (success) {
                        current_user.username = new_name;
                        std::cout<<"Your username has been updated!\n";
                        system("pause");
                        break;
                    }
                }while (true);
            } else if (ans == 1){
                bool success = true;
                current_user.password = input("Enter new password: ");
                std::cout<<"Your password has been updated!\n";
                system("pause");
            } else if (ans == 2) continue;
        } else if (result == 1){
            //neuro->users.erase(neuro->users.begin() + current_user.user_id);
            for (int i = current_user.user_id; i<neuro->total_user; i++){
                neuro->users[i] = neuro->users[i+1];
            }
            neuro->total_user--;
            system("cls"); banner("\033[0m");
            center_text("Account Deleted Successfully", 71);
            line(73, '='); system("pause"); 
            return result;
        } else if (result == 2) return 0;
    }while (result != 2);
    return 0; // Safe guard for the compiler
}

void user_interface(Neuro* neuro){
	banner("\033[0m");
    line(73, '=');
    const int num_choice = 5;
    std::string options[] = {"NeuroChat", "Chat History", "Guide", "Account", "Log Out"};
    int choice;
    do {
        auto& current_user = neuro->users[neuro->id];
    	std::string title = "H E L L O " + current_user.username + "!";
        choice = show_menu(neuro, options, num_choice, title);
        system("cls");
        if(choice == 0)
            new_chat(neuro); //chat(); 
        else if(choice == 1)
            history(neuro);
        else if(choice == 2)
            help();
        else if (choice == 3){
            int result = account(neuro); 
            if (result == 1) choice = 4; //kondisi delete account           
        }else if(choice == 4)
            neuro->id = -1;
    } while(choice != 4);
    system("cls");
}

void signup(Neuro* neuro){
    system("cls");
	std::string new_name, new_password;
	bool success;
	do{
        banner("\033[1;35m");
        center_text("REGISTER IDENTITY", 71);
        line(73, '=');
        do{
            new_name = input("Enter Name: ");
            if (new_name.empty() || new_name == "") {
                std::cout << "Username cannot be empty!\n";
                success = false; continue;
            }
            break;
        }while(true);
        success = true;
/*		for(auto user : neuro->users){
			if(user.username == new_name){
				std::cout << "Username not available!" << std::endl;
                system("pause"); return;
			}
		}
*/
        for (int i = 0; i<neuro->total_user; i++){
            if (new_name == neuro->users[i].username){
                std::cout << "Username not available!" << std::endl;
                system("pause"); return;
            }
        }
		if (success){
            do{
                new_password = input("Enter Password: ");
                if(new_password.empty() || new_password == ""){
                    std::cout << "Password cannot be empty!\n";
                    continue;
                }
                break;
            }while(true);
            neuro->id = neuro->total_user; //noted
//			neuro->users.push_back({new_name, new_password}); 
            neuro->users[neuro->id] = {new_name, new_password};
            neuro->total_user++;
			std::cout<<"Account created successfully!\n";
            system("pause");
		}
		std::string options[] = {"Back", "Continue"};
		int result = confirm(options, 2, "Proceed to user dashboard? ");
		if (result == 0)
    		user_interface(neuro);
    	break;
	}while(1);
}

inline void credits(){
    banner("\033[1;35m");
    std::cout<<
R"(
                             ++=--+=--------=++-===                             
                        -=+==++=-+=----------=+=========                        
                    +====+=====-=+------------=+=========*%%                    
                 %=+=-=-==-===-===------=------=+===---=+++%%*#                 
               #%@#=-===+-----===-------=-----====-=--=++*++@%##%               
             *#@%*+#%*===-----=----------=-=--::----*===++###@###%+             
           +*%@%++%%%%%#==--=-=-::-------=:--::::-==++=+++###@%#*#%==           
         ++#@@#=+%%%#%%*=======-::-:::--:=-----:-====*+=+++*#@@#*#%#=           
        ==*%@#=+*%%%###*=+++++=----::-=--=--===-=====+*=+++++%@%*##%#-*#        
      #+++%@%+++#%%%##*+=++++++===---==+-++=========+=+==+++++#%#*%#%+-*+       
     ###+#@%#+*+#%%%#%%*=++++*+=======+==++=========+++===++#*++**%#%#+*+-      
    #####%@%#+#*%%%%#%%#+=+*#+++======+==+=+==+=====++=..-==+###**%#%%*+=       
   %####%@@##+#*%%%%%%%%#*###+++=====++=+=-===+=====++-::=-=+++######%*+=       
  %#%%##@@%####*#%%%%#%@@###*++==+===*+=+-:-+++=====:=-=#@@%####%%#%%%#+        
  %##%#%@@%##%##%@%%%@@@@%*#*++=+===++=++=--=++====-:=+@#==*%%#*##%%##+=        
 %%#%%#%%%%######%%%@@@@@@@#+*++===+#*+*+=-:-++=-==::=@%%=:+=%#+=*###%#====     
 *#%%%#%%@@%#####%@@@@@@@%%+++====+%%##%@@%%%#+===-..+%%#=:-+%#==#%#%#%+=       
 *##%%#%%%@@#####%@@@%%*#*+**===+#%#++*****#%@%=-:.:.-++=-::*%*=*%%#++*++++=-   
 **#*%#%%%%@@%##%@@%#++##+#*==+++%%+*********%#-..::..-==---+#=*####*=-+=       
 *##*+#%%%#%@@@%###+=*%%##++*#*-#%#*********#%-.::...:::--===#%#+++#*==*=      %
 *#%#**#%@%##%@@#*#%%###*###*=+*%%+*********%+:::...:--:--==+#%#==+#*==+=    =*%
  %%#%###########%%@#*+*##+=+++#%#+***#****#%-:::..:.:::-----%%*++#+++++    =+%%
   #####**######%@@@#:-=++#++==+%%%#+++***%#=--::.::...:::::=%%++#+++*+= +==*%#=
   +++#*++###%####%@@#--=+*#+===--=*###%%#=---:::..::=+:.::=%%#+**=+++==-=*%%*= 
        +*#%########%@%#+-+##+=-=----===-----::--==++#+:::=++**+#++++++#%%#*+   
        ++  #########%%%%%%##*+*+--------::::::+*+====:::+###**#+=====--==:     
 %%%@%%###############%###*###+=+#+--:::::::::.:=====::-*%%%%#####++-           
 @@%%%%############%##++**#*####-+###=-:::::::::::::::-%%%%%%%#%%@@+##+=        
 @@%%#########*===*###*=:=*#*####*=**#+=*#*+=-:::::::+@%%%%@@@%%%%@#####*    =  
  #*++++++---+#*=---+##*=::-***###*#**#--=####%%#*#%%%%%%%%@@@%#%@@%*####--==+  
  +==++==++----+*=--:=*##+-:--+*###*++++-:-=*###***####*==+#%%@##########+%%%%  
   ===+++=++=---=#=--:-+###+----+*###*+=-=-::=#***####+:--:+%%%%###+######%%%   
    ===+++==+++=-=#+=+++*###*+=---+**++==----::-+***##*===-=%%%%##+#####*%%%    
     ++==++===+**##############*++=-=+*+=-:::---::-=*#=-=--+%%#%%+#######%%     
      %##+++*+#%%%%%#############==----==+=-:::---::----=--+%%%%#######%%%      
        %%##%%%%%%#*############*++=-:::::-----====--::-=+=-=%%%######%%        
         %@%%%%%%%%%#+#%#######**--=*=--===--::::-----:::--=-=##+#####%         
           #*#%%%%%%%#+*##%%%##%%#+--=**+-::--=+--::::::::::---+#*###           
             ==-=#%#++++++-*%%%%%%%%#+--+##*=-::----::::::-------+#             
               ....:=+++*##+#%%%%%%%%%%%####++=-::::::::---------               
                 ....-=###%%%%%%%%%%%%%%%%%#+------::----------                 
                    .:--=#%%%%%#%%%%%%%%%%%%#*+*##*+=-------                    
                        --+%%%%%%##%%%%%%%%%%%#**+**%#+=                        
                             %%%%%%%%###%%%%%%%%%%%                             

MIT License

Copyright (c) 2025 I Nyoman Widiyasa Jayananda and Co.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
)";line(73, '-');std::cout<<
R"(Made with ❤️ by:
I Nyoman Widiyasa Jayananda (Schryzon) - F1D02410053
I Kadek Mahesa Permana Putra (Vuxyn) - F1D02410052
Samara Wardasadiya (samarawards) - F1D02410023

Thanks to:
Muhammad Rendi Maulana (YNK) as our group's assistant
Christian Hadi Chandra (CHIZ) as the practicum coordinator

Special thanks to:
Senior Muhammad Kholilluloh for bringing upon solution to approve our project idea
Dzakanov Inshoofi (SEA) for the insights on low-level programming and libraries

Sincerely,
Team Neuro.

)";
    system("pause");
}

void login(Neuro* neuro){
	bool success = false;
	banner("\033[1;32m");
    center_text("CONNECT TO YOUR ACCOUNT", 71);
    line(73, '=');
	std::string name;
	char ch;
    char char_password[100];
    int len = 0;
    if(neuro->total_user == 0){
        center_text("No Account Registered Yet! ", 71);
        line(73, '=');
        system("pause");
        std::string options[] = {"Back", "Register"};
		int result = confirm(options, 2, "Proceed to Register?");
		if (result == 0)
    		signup(neuro);
        return;
    }
	name = input("Enter your name: ");
	std::cout << "Enter Password: ";
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
	for(int i=0; i<neuro->total_user; i++){
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
		std::cout<<"Login failed!\n";
        system("pause");
	}
}

void main_menu(Neuro* neuro){
    banner("\033[0m");
    line(73, '=');
    const int num_choice = 6;
    std::string options[] = {"NeuroChat", "Register", "Guide", "Connect", "Disconnect", "Credits & License"};
    int choice;
    do {
    	std::string title = "S Y N T A X - S E M A N T I C - S E N T I E N C E  ";
        choice = show_menu(neuro, options, num_choice, title);
        system("cls");
        if(choice == 0)
            new_chat(neuro);
        else if(choice == 1)
            signup(neuro);
        else if(choice ==2)
            help();
        else if(choice == 3)
            login(neuro);
        else if(choice == 5)
            credits();
    } while(choice != 4);
    system("cls");
}

inline void loading(Neuro* neuro){
    std::string warna[] = {
        "\033[91m", "\033[92m", "\033[93m",
        "\033[94m", "\033[95m", "\033[1;34m",
        "\033[1;33m", "\033[1;35m"
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

inline int end_program(Neuro* neuro, int status_code = 0){
    system("pause");
    delete[] neuro->users->chats->messages;
    neuro->users->chats->messages = nullptr;
    delete[] neuro->users->chats;
    neuro->users->chats = nullptr;
    delete[] neuro->users;
    neuro->users = nullptr;
    delete neuro;
    neuro = nullptr;
    return status_code;
}

int main(){
    dotenv::init(".env"); // DO NOT CHANGE THIS
    Neuro* neuro = new Neuro();
    try{
        curlpp::Cleanup cleanup; // RAII
        loading(neuro);
    }
    catch (const curlpp::RuntimeError & e) {
        std::cerr << "cURLpp runtime error: " << e.what() << "\n";
        return end_program(neuro, 1);
    }
    catch (const curlpp::LogicError & e) {
        std::cerr << "cURLpp logic error: " << e.what() << "\n";
        return end_program(neuro, 1);
    }
    catch (const nlohmann::json::exception & e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return end_program(neuro, 1);
    }
    catch (const std::exception & e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return end_program(neuro, 1);
    }
    return end_program(neuro, 0);
}