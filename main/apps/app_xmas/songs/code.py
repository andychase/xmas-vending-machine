import re
import sys
import math

name_pattern = r"//\s*(?P<name>.*)"
melody_pattern = r"int\s+melody\[\]\s*=\s*\{\s*(?P<melody>[\d,\s]+)\s*\};"
note_durations_pattern = r"int\s+noteDurations\[\]\s*=\s*\{\s*(?P<duration>[\d,\s]+)\s*\};"
constants_pattern = r"noteDurations\[thisNote\]\s*\*\s*(?P<constants>[.\d]+)"

regex = re.compile(f"{name_pattern}|{melody_pattern}|{note_durations_pattern}|{constants_pattern}")


# Data storage
songs = []
song_constants = []

current_song = {}
current_constants = []

for match in regex.finditer(sys.stdin.read()):
    if match.group("name"):
        if current_song:
            songs.append(current_song)
            song_constants.append(current_constants)
        current_song = {"name": match.group("name"), "melody": [], "duration": []}
        current_constants = []
    elif match.group("melody"):
        current_song["melody"] = [int(x.strip()) for x in match.group("melody").split(",") if x.strip()]
    elif match.group("duration"):
        current_song["duration"] = [int(x.strip()) for x in match.group("duration").split(",") if x.strip()]
    elif match.group("constants"):
        current_constants.append(float(match.group("constants")))

# Add the last song
if current_song:
    songs.append(current_song)
    song_constants.append(current_constants)

# Format the output
MAX_NOTES = max(max(len(_['melody']) for _ in songs), max(len(_['duration']) for _ in songs))
output_songs = f"inline static const int songs[{len(songs)}][2][{MAX_NOTES}] = {{\n"
output_constants = f"inline static const int songConstants[{len(songs)}][2] = {{\n"


for i, song in enumerate(songs):
    output_songs += f"  {{ // {song['name']}\n"
    output_songs += f"    {{{', '.join(map(str, song['melody']))}}},\n"
    output_songs += f"    {{{', '.join(map(str, song['duration']))}}},\n"
    output_songs += f"  }},\n"

    output_constants += f"  // {song['name']}\n"
    if len(song_constants[i]) < 2:
        song_constants[i].append(1)
    if len(song_constants[i]) < 2:
        song_constants[i].append(1)
    song_constants[i] = [round(_*10) for _ in song_constants[i]]
    output_constants += f"  {{{', '.join(map(str, song_constants[i]))}}},\n"

output_songs += "};\n"
output_constants += "};\n"

print("""namespace MOONCAKE { namespace USER_APP { namespace XMAS {    """)
print(output_songs)
print(output_constants)
print(""" } } } """)

