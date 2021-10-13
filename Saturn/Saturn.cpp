// Saturn.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <dpp/dpp.h>
#include <dpp/fmt/printf.h>
#include <iostream>
#include <unordered_map>
#include <chrono>

#define TOKEN_LENGTH 59 
#define CONSOLE_TEXT_WHITE 15
#define CONSOLE_TEXT_RED 12

HANDLE ConsoleHandle;
static char CommandPrefix = ';';
std::unordered_map<std::string, std::function<void(struct MessageDetails, std::vector<std::string>)>> CommandFunctionMap;
bool InVoiceChat;
dpp::snowflake CurrentVoiceChannel;
dpp::discord_voice_client* VoiceClient;

struct MessageDetails
{
    //Do not use this if you are unsure. You probably want the bot.
    dpp::discord_client* messageClient;
    dpp::cluster* bot;
    dpp::user author;
    dpp::snowflake guild_id;
	dpp::snowflake channel_id;
    time_t sentTime;

    MessageDetails(dpp::cluster* _bot, dpp::discord_client* _client, dpp::user _author, dpp::snowflake _guild_id, dpp::snowflake _channel_id, time_t _sentTime)
	: bot(_bot), messageClient(_client), author(_author), guild_id(_guild_id), channel_id(_channel_id), sentTime(_sentTime) {};
};


void PrintError(std::string error)
{
    SetConsoleTextAttribute(ConsoleHandle, CONSOLE_TEXT_RED);
    std::cout << error << "\n";
    SetConsoleTextAttribute(ConsoleHandle, CONSOLE_TEXT_WHITE);
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

void PingPongReply(MessageDetails details, std::vector<std::string> parameters)
{
    details.bot->message_create(dpp::message(details.channel_id, "Pong!"));
}

void SendHelpMessage(MessageDetails details, std::vector<std::string> parameters)
{
    std::string help = "";
    help += "\n";
    help += "\n\t\t => " + details.bot->me.username + " <=\n\n";
    help += details.bot->me.username + " is maintained by Hal, (@HalWasTooShort), with the source available on Github(https://github.com/hallamrear/SaturnCPP)\n\n";
    help += "All the available commands include:\n";
    help += "> " + CommandPrefix; help += "help - Displays this message.\n";
    help += "> " + CommandPrefix; help += "list - Lists all servers this bot is in.\n";
    help += "> " + CommandPrefix; help += "SetCommandKey - Sets the character which comes before each command.\n";
    help += "> " + CommandPrefix; help += "GetCommandKey - Displays the current command key.\n";
    help += "> " + CommandPrefix; help += "Ping - Pong.\n";
    help += "> " + CommandPrefix; help += "Join - Joins the voice channel of whoever told it to.\n";
    help += "> " + CommandPrefix; help += "Leave - Leaves the voice channel if its in one.\n";
	help += "\n";

    details.bot->message_create(dpp::message(details.channel_id, help));
}

void ListCommands(MessageDetails details, std::vector<std::string> parameters)
{
    if(parameters.size() != 0)
    {
        details.bot->message_create(dpp::message(details.channel_id, "You don't need to pass anything into this function."));
    }

	dpp::cluster& bot = *details.bot;
    dpp::snowflake channel = details.channel_id;

    details.bot->current_user_get_guilds([&bot, &channel](const dpp::confirmation_callback_t& event)
    {
        if (event.is_error())
        {
            PrintError(event.get_error().message);
        }
        else
        {
            std::string guildList = "";
            dpp::guild_map guilds;
            guilds = std::get<dpp::guild_map>(event.value);

            if (guilds.size() > 1)
            {
                guildList += bot.me.username + " is currently in " + std::to_string(guilds.size()) + " servers.\n";

                for (auto& guild : guilds)
                {
                    guildList += "=> " + guild.second.name + '\n';
                }

                guildList += "\n";
            }
            else
                guildList += bot.me.username + " is currently only in this server.\n";

            bot.message_create(dpp::message(channel, guildList));
        }
    });
}

void LeaveVoice(MessageDetails details, std::vector<std::string> parameters)
{
    if(InVoiceChat)
    {
        details.messageClient->disconnect_voice(details.guild_id);

        InVoiceChat = false;
        CurrentVoiceChannel = 0;
        VoiceClient = nullptr;
    }
}

void JoinVoice(MessageDetails details, std::vector<std::string> parameters)
{
    if(parameters.size() != 0)
    {
        details.bot->message_create(dpp::message(details.channel_id, "I'm not a dog. Don't try and tell me where to go."));
    }

    dpp::user target = details.author;
	dpp::guild* guild = dpp::find_guild(details.guild_id);

    bool needToJoinChat = true;
    /* Check if we are currently on any vc */
    if (InVoiceChat)
    {
        /* Find the channel id  that the user is currently on */
        auto users_vc = guild->voice_members.find(details.author.id);

        if (users_vc != guild->voice_members.end() && CurrentVoiceChannel == users_vc->second.channel_id)
        {
            //We're already in the voice channel
            needToJoinChat = false;
            details.bot->message_create(dpp::message(details.channel_id, "I'm already here bozo."));
        }
        else 
        {
            //Leave and set us to rejoin
            LeaveVoice(details, parameters);
            needToJoinChat = true;
        }
    }

    //If we do need to join a channel
    if (needToJoinChat) 
    {
        //Make sure theyre in one.
    	//The actual joining bit is done here.
        //Itll fail if youre not in a channel
        if (!guild->connect_member_voice(details.author.id)) 
        {
            details.bot->message_create(dpp::message(details.channel_id, "Get in a channel so i can join."));
        }
        else 
        {
            //Get our voice connection data
            dpp::voiceconn* voiceConnection = details.messageClient->get_voice(details.guild_id);

            while(voiceConnection && voiceConnection->voiceclient && voiceConnection->voiceclient->is_ready() == false)
            {
                InVoiceChat = true;
                CurrentVoiceChannel = voiceConnection->channel_id;
            }

            //if (voiceConnection && voiceConnection->voiceclient)
            //{
            //    /* Stream the already decoded MP3 file. This passes the PCM data to the library to be encoded to OPUS */
            //    //voiceConnection->voiceclient->send_audio_raw((uint16_t*)pcmdata.data(), pcmdata.size());

            

            //
            //
            //}

            /* We are now connecting to a vc. Wait for on_voice_ready
             * event, and then send the audio within that event:
             *
             * event.voice_client->send_audio_raw(...);
             *
             * NOTE: We can't instantly send audio, as we have to wait for
             * the connection to the voice server to be established!
             */

            voiceConnection = nullptr;
        }
    }
}

void ParseCommand(MessageDetails details, std::string command, std::vector<std::string> arguments)
{
    std::cout << "Command: " << command << "; Parameters: ";

    for (size_t i = 0; i < arguments.size(); i++)
    {
        std::cout << arguments[i] << ", ";
    }
    std::cout << "\n";

    auto commandPair = CommandFunctionMap.find(command);

    if(commandPair != CommandFunctionMap.end())
    {
        commandPair->second(details, arguments);
    }
    else
    {
        std::cout << "Last Command was undefined in the function map." << std::endl;
    }
}

void SetCommandKey(MessageDetails details, std::vector<std::string> arguments)
{
    if(arguments.size() != 1)
    {
        details.bot->message_create(dpp::message(details.channel_id, "You didn't use the correct number of arguments. Command ignored."));
        return;
    }

    if(arguments[0].length() > 1)
    {
        details.bot->message_create(dpp::message(details.channel_id, "You passed in an argument that was more than a single character long. Command ignored."));
    }

    CommandPrefix = arguments[0].at(0);
    std::string s = "The new command prefix is ["; s += CommandPrefix; s += "]\n";
    details.bot->message_create(dpp::message(details.channel_id, s));
}

void GetCommandKey(MessageDetails details, std::vector<std::string> arguments)
{
    if (arguments.size() != 0)
    {
        details.bot->message_create(dpp::message(details.channel_id, "Useless... absolutely useless arguments passed in."));
    }

    std::string s = "The current command prefix is ["; s += CommandPrefix; s += "]\n";
    details.bot->message_create(dpp::message(details.channel_id, s));
}   

uint8_t* robot = nullptr;
size_t robot_size = 0;

void PlayTestAudio(MessageDetails details, std::vector<std::string> arguments)
{
    if(InVoiceChat)
		VoiceClient->send_audio_raw((uint16_t*)robot, robot_size);
}

void TestAudioStream()
{
    std::ifstream input("Robot.wav", std::ios::in | std::ios::binary | std::ios::ate);
    if (input.is_open()) {
        robot_size = input.tellg();
        robot = new uint8_t[robot_size];
        input.seekg(0, std::ios::beg);
        input.read((char*)robot, robot_size);
        input.close();
    }
}

int main()
{
    TestAudioStream();

    InVoiceChat = false;
    CurrentVoiceChannel = -1;
    VoiceClient = nullptr;

    std::string token;
    LoadToken(token, "token.txt");

    if (token.length() != TOKEN_LENGTH || token == "")
    {
        PrintError("Invalid Token read");
        std::cin.get();
        return 0;
    }

    dpp::cluster bot(token);

    CommandFunctionMap = std::unordered_map<std::string, std::function<void(MessageDetails, std::vector<std::string>)>>();
    //These all need to be in lower case.
    CommandFunctionMap.emplace(std::make_pair("help", SendHelpMessage));
    CommandFunctionMap.emplace(std::make_pair("setcommandkey", SetCommandKey));
    CommandFunctionMap.emplace(std::make_pair("getcommandkey", GetCommandKey));
    CommandFunctionMap.emplace(std::make_pair("join", JoinVoice));
    CommandFunctionMap.emplace(std::make_pair("leave", LeaveVoice));
    CommandFunctionMap.emplace(std::make_pair("ping", PingPongReply));
    CommandFunctionMap.emplace(std::make_pair("list", ListCommands));
    CommandFunctionMap.emplace(std::make_pair("playtest", PlayTestAudio));

    ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(ConsoleHandle, CONSOLE_TEXT_WHITE);

    bot.on_ready([&bot](const dpp::ready_t& event) 
    {
		std::cout << "Logged in as " << bot.me.username << "!\n";
    });

    bot.on_message_create([&bot](const dpp::message_create_t& event)
    {
        std::string content = event.msg->content;

		if(content[0] == CommandPrefix)
		{
            std::string command = "";
            std::vector<std::string> arguments;
            MessageDetails details(&bot, event.from, *event.msg->author, event.msg->guild_id, event.msg->channel_id, event.msg->sent);

            content = content.substr(1, content.length() - 1);

            int end = content.find(' ');
            if (end != -1)
            {
                command = content.substr(0, end);
                content.erase(0, end + 1);

                int start = 0;
                end = content.find(' ');
                while (end != -1) 
                {
                    arguments.push_back(content.substr(start, end - start));
                    start = end + 1;
                    end = content.find(' ', start);
                }
                arguments.push_back(content.substr(start, end - start));

            }
            else
                command = content;

            for (size_t i = 0; i < command.size(); i++)
                command[i] = std::tolower(command[i]);

            ParseCommand(details, command, arguments);
		}

        content.clear();
    });

    bot.on_voice_ready([&bot](const dpp::voice_ready_t& event)
    {
    	InVoiceChat = true;
        CurrentVoiceChannel = event.voice_channel_id;
        VoiceClient = event.voice_client;
    });
    
	/*
	 * The parameter which we set to false indicates if the function should return once all shards are created.
     * Passing false here tells the program you do not need to do anything once bot.start is called,
     * so the return statement directly afterwards is never reached
     */
    bot.start(false);
	return 0;
}