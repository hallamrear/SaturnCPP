import os
from os import walk
import ctypes
import discord
from discord.ext import commands
import asyncio
import youtube_dl

ytdl_format_options = {
    'format': 'bestaudio/best',
    'outtmpl': '%(extractor)s-%(id)s-%(title)s.%(ext)s',
    'restrictfilenames': True,
    'noplaylist': True,
    'nocheckcertificate': True,
    'ignoreerrors': False,
    'logtostderr': False,
    'quiet': True,
    'no_warnings': True,
    'default_search': 'auto',
    'source_address': '0.0.0.0' # bind to ipv4 since ipv6 addresses cause issues sometimes
}

ffmpeg_options = {
    'options': '-vn'
}

ytdl = youtube_dl.YoutubeDL(ytdl_format_options)

#discord.py api documentation
#https://discordpy.readthedocs.io/en/latest/index.html
os.system('CLS')

#getting token
fileHandler = open("token.txt")
listOfLines = fileHandler.readlines()
token = listOfLines[1]
owner_id = listOfLines[3]
fileHandler.close()

#create client
#saturn = discord.Client()#
saturn = commands.Bot(command_prefix='-')
saturn.owner_id = 226051785830170625
opus = ctypes.util.find_library("opus")

#join voice
async def join_voice(channel):
    if(len(saturn.voice_clients) != 0):

        if(saturn.voice_clients[0].channel == channel):
            return
        await leave_voice(saturn.voice_clients[0])

    print("Joining voice channel <" + channel.name +  ">")
    await channel.connect()

#leave voice
async def leave_voice(voice_client):
    print("Leaving voice channel <" + voice_client.channel.name +  ">")
    await voice_client.disconnect()

@saturn.event
#on bot ready
async def on_ready():
    await saturn.change_presence(status=discord.Status.online, activity=discord.Streaming(name="Chuck :)", url = "https://www.twitch.tv/chuckecheese"))
    print('We have logged in as {0.user}'.format(saturn))
    await check_for_owner_in_voice()

async def check_for_owner_in_voice():
    targetVC = None
    for guild in saturn.guilds:
        for voice_channel in guild.voice_channels:
            for member_in_channel in voice_channel.members:                
                if(member_in_channel.id == owner_id):
                    targetVC = voice_channel

    if(targetVC != None):
        await join_voice(targetVC)

#join/leave voice event
@saturn.event
async def on_voice_state_update(member, before, after):
    #ignore self
    if(member.id != saturn.user.id):
        #ignore disconnects or self controls (mute/deafen)
        if(before.channel == after.channel or after.channel == None):
            return   

        await join_voice(after.channel)

@saturn.event
async def on_message(message):
    if message.author == saturn.user:
        return
    if(message.content[0] == '-'):
        message.content = str(message.content.split('-', 1)[1])
        await Parse_Command(message)
                           
async def Parse_Command(message):
    requested_command = message.content.split(' ', 1)[0]

    try:
        command_data = message.content.split(' ', 1)[1]
    except:
        command_data = ""
   
    action = command_to_function(requested_command)
    
    print("\ncommand: " + str(action))
    print("   args: " + str(command_data) + '\n')

    try:
        await action(message.channel, command_data)
    except Exception as e:
        print(e)
        await message.channel.send("yea no thats not gonna work (reason: " + e)

#custom command -list
async def list_commands_in_channel(requested_in_channel, command_data):
    f = []
    for (dirpath, dirnames, filenames) in walk("data/"):
        f.extend(filenames)
        break

    print(f)
    path_string = ""
    for i in f:
        path_string = path_string + "\t" + i.split(".mp3", 1)[0] + "\n"
    await requested_in_channel.send("```List\n-play\n" + path_string + "\n-when\n```")                              

#play music
async def play_sound(requested_in_channel, sound_name):
    if(len(saturn.voice_clients) == 0):
        await requested_in_channel.send('youre not in a voice channel')
        return
   
    if(len(saturn.voice_clients) != 0):
        if(saturn.voice_clients[0].is_playing()):
            saturn.voice_clients[0].stop()

    if "yt" in sound_name:
        url = sound_name
        loop = None
        loop = loop or asyncio.get_event_loop()
        stream = True
        data = await loop.run_in_executor(None, lambda: ytdl.extract_info(url, download=not stream))

        if 'entries' in data:
            # take first item from a playlist
            data = data['entries'][0]

        filename = data['url']# if stream else ytdl.prepare_filename(data)
        title = data['title']
        print(title)
        duration = data['duration']
        print(duration)
        player = discord.FFmpegPCMAudio(filename, **ffmpeg_options)
        saturn.voice_clients[0].play(player, after=lambda e: print('Player error: %s' % e) if e else None)

    else:
        dir = os.path.dirname(__file__)
        filename = os.path.join(dir, 'data/' + sound_name + '.mp3')
        audio_source = discord.FFmpegPCMAudio(filename)
        saturn.voice_clients[0].play(audio_source)

#custom command -ping-
async def Pong(channel, command_data):
    await channel.send("pong")

#custom command -when"
async def Did_I_Ask(channel, command_data):
    await channel.send("did i ask? bc even i dont remember it")

#custom command -leave
async def leave_voice_command(channel, command_data):
    if(len(saturn.voice_clients) != 0):
        if(saturn.voice_clients[0].channel == channel):
            return
        await leave_voice(saturn.voice_clients[0])

async def join_voice_command(channel, command_data):
    author = channel.last_message.author.id
    targetVC = None
    for voice_channel in channel.guild.voice_channels:
        print(voice_channel.name)
        print(len(voice_channel.members))
        for member_in_channel in voice_channel.members:
            print(member_in_channel.name)
            if(member_in_channel.id == author):
                targetVC = voice_channel
                print("Target found.")

    if(targetVC != None):
        await join_voice(targetVC)
        
#custom commands switcher
switcher = {
        "play": play_sound,
        "ping": Pong,
        "when": Did_I_Ask,
        "list": list_commands_in_channel,
        "leave": leave_voice_command,
        "join": join_voice_command
    }

def command_to_function(requested_command):
    # Get the function from switcher dictionary
    out = switcher.get(requested_command, "INVALID_COMMAND")
    print(out)
    return out

saturn.run(token)
