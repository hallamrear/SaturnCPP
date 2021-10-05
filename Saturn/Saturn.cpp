// Saturn.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <dpp/dpp.h>
#include <iostream>

#define TOKEN_LENGTH 61

void LoadToken(std::string& token, const char* location_path)
{
    std::fstream stream(location_path);
    if (stream.good())
    {
        char* buffer = new char[TOKEN_LENGTH];
        stream.read(buffer, TOKEN_LENGTH);
        token = buffer;

        delete buffer;
        buffer = nullptr;
    }
    else
        token = "";
}

int main()
{
    std::string token;
    LoadToken(token, "token.txt");
    if (token == "")
    {
        std::cout << "Invalid Token read\n";
        std::cin.get();
        return 0;
    }

    dpp::cluster bot(token);

    bot.on_ready([&bot](const dpp::ready_t& event) 
        {
			std::cout << "Logged in as " << bot.me.username << "!\n";
        });

    bot.on_message_create([&bot](const dpp::message_create_t& event)
        {
	        if (event.msg->content == "!ping") 
	        {
	            bot.message_create(dpp::message(event.msg->channel_id, "Pong!"));
	        }
        });

    
	/*
	 * The parameter which we set to false indicates if the function should return once all shards are created.
     * Passing false here tells the program you do not need to do anything once bot.start is called,
     * so the return statement directly afterwards is never reached
     */
    bot.start(false);
	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
