// Saturn.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <dpp/dpp.h>
#include <iostream>

#define TOKEN_LENGTH 59
#define CONSOLE_TEXT_WHITE 15
#define CONSOLE_TEXT_RED 12
HANDLE consoleHandle;

void PrintError(std::string error)
{
    SetConsoleTextAttribute(consoleHandle, CONSOLE_TEXT_RED);
    std::cout << error << "\n";
    SetConsoleTextAttribute(consoleHandle, CONSOLE_TEXT_WHITE);
}

void LoadToken(std::string& token, const char* location_path)
{
    std::fstream stream(location_path);
    if (stream.good())
    {
        //Create buffer to exact size of expected token.
        char* buffer = new char[TOKEN_LENGTH];

        //Read the expected number of characters from the file
        //This should exclude the endline and eof.
        stream.read(buffer, TOKEN_LENGTH);

        //Set token to the buffer.
        //This fixes the error presented in line 27 in which the buffer is created 4 characters larger in debug mode.
        token.assign(&buffer[0], &buffer[0] + TOKEN_LENGTH);

        //Cleanup
        delete [] buffer;
        buffer = nullptr;
    }
    else
        token = "";

    stream.close();
}

int main()
{
    consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(consoleHandle, CONSOLE_TEXT_WHITE);

    std::string token;
    LoadToken(token, "token.txt");


    if (token.length() != TOKEN_LENGTH || token == "")
    {
        PrintError("Invalid Token read");
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
            std::string content = event.msg->content;
          
	        if (event.msg->content == "!ping") 
	        {
	            bot.message_create(dpp::message(event.msg->channel_id, "Pong!"));
	        }

            if (event.msg->content == "!list")
            {
                dpp::snowflake channel = event.msg->channel_id;

                bot.current_user_get_guilds([&bot, &channel](const dpp::confirmation_callback_t& event)
                {
						if (event.is_error())
                        {
                            std::cout << event.get_error().message << std::endl;
                            return;
                        }
                        else
                        {
                            std::string guildList = "";
                            dpp::guild_map guilds;
                            guilds = std::get<dpp::guild_map>(event.value);

                            if (guilds.size() > 1)
                            {
                                guildList += "```" + bot.me.username + " is currently in " + std::to_string(guilds.size()) + " servers.\n";

                                for (auto& guild : guilds)
                                {
                                    guildList += "=> " + guild.second.name + '\n';
                                }

                                guildList += "```\n";
                            }
                            else
                                guildList += "```" + bot.me.username + " is currently only in this server.```\n";

                            bot.message_create(dpp::message(channel, guildList));
                        }
                });

               
            }


            if(event.msg->content == "!help")
            {
                std::string help = "";
                help += "```\n";
            	help += "\n\t\t => " + bot.me.username + " <=\n\n";
            	help += bot.me.username + " is maintained by Hal, (@HalWasTooShort), with the source available on Github(https://github.com/hallamrear/SaturnCPP)\n";
                help += "All the available commands include:\n";
                help += "> !list - Lists all servers this bot is in.";
                help += "```\n";
                
                bot.message_create(dpp::message(event.msg->channel_id, help));
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
