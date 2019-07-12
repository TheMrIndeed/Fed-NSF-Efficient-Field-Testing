# Fed-NSF-Efficient-Field-Testing
All of the files in the project will need to be edited to include information for api keys and ThingSpeak channel ID.

The only changes that need to be made to the Temperature_Control_Panel are changeing the ****** to the ThingSpeak channel ID.

There are multiple changes that need to be made to the Arduino_Field_Testing file. All of the changes that need to be made are before the setup.
- **ThingSpeakChannelIDTemp** Temperature ThingSpeak Channel ID
- **ThingSpeakChannelIDHigh** HighTemperature ThingSpeak Channel ID
- **ThingSpeakChannelIDAvg** AverageDifference ThingSpeak Channel ID
- **ThingSpeakWriteKeyTemp** Temperature Channel Write API Key
- **ThingSpeakWriteKeyTemp** HighTemperature Read API Key
- **ThingSpeakWriteKeyTemp** AvgerageDifference Channel Write API Key
- **location** needs to be updated for the latatude and longitude
- **IFTTT_Key** IFTTT API Key
